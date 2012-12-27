/*
 * ComicPage.cpp
 * Copyright (c) 2006-2011, James Athey. 2012, John Peterson.
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   In addition, as a special exception, the author gives permission to   *
 *   link the code of his release of Comical with Rarlabs' "unrar"         *
 *   library (or with modified versions of it that use the same license    *
 *   as the "unrar" library), and distribute the linked executables. You   *
 *   must obey the GNU General Public License in all respects for all of   *
 *   the code used other than "unrar". If you modify this file, you may    *
 *   extend this exception to your version of the file, but you are not    *
 *   obligated to do so. If you do not wish to do so, delete this          *
 *   exception statement from your version.                                *
 *                                                                         *
 ***************************************************************************/

#include <cstring>
#include "ComicPage.h"
#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
#include "Exceptions.h"
#include "JpegWxInputStreamSrc.h"
#include <wx/imagjpeg.h>

ComicPage::ComicPage(wxString &filename, wxInputStream *headerStream):
Filename(filename),
orientation(NORTH),
m_uiWidth(0),
m_uiHeight(0),
m_bitmapType(wxBITMAP_TYPE_INVALID),
m_bitmapLeft(),
m_bitmapRight(),
m_bitmapFull(),
m_bitmapThumb()
{
	extractDimensions(headerStream);
}


ComicPage::~ComicPage()
{
	DestroyAll();
}

void ComicPage::Rotate(COMICAL_ROTATE direction)
{
	if (orientation != direction) {
		wxMutexLocker rlock(ResampleLock);
		wxMutexLocker tlock(ThumbnailLock);
		orientation = direction;
		DestroyResample();
		DestroyThumbnail();
	}
}

#define STREAM_READ(stream, buffer, length) if(!(stream->Read(buffer, length).LastRead())) return
#define STREAM_SEEK(stream, pos, mode) if(stream->SeekI(pos, mode) == wxInvalidOffset) return

#define TIFF_IMAGE_LENGTH 257
#define TIFF_IMAGE_WIDTH 256

enum TIFF_TAG_DATATYPE
{
	TIFF_TAG_DATATYPE_BYTE = 1,
	TIFF_TAG_DATATYPE_ASCII,
	TIFF_TAG_DATATYPE_SHORT,
	TIFF_TAG_DATATYPE_LONG,
	TIFF_TAG_DATATYPE_RATIONAL,
};


/*
 * A replacement for the jpeg library's default error_exit callback to keep
 * the program from exiting on a jpeg decode error.
 */
METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
	throw JpegException(cinfo);
}


void ComicPage::extractDimensions(wxInputStream *stream)
{
	const wxUint8 jpegHeader[] = { 0xFF, 0xD8 };
	const wxUint8 tiffBigHeader[] = { 0x4d, 0x4d, 0x00, 0x2a };
	const wxUint8 tiffLittleHeader[] = { 0x49, 0x49, 0x2a, 0x00 };
	const wxUint8 gif87Header[] = { 'G', 'I', 'F', '8', '7', 'a' };
	const wxUint8 gif89Header[] = { 'G', 'I', 'F', '8', '9', 'a' };
	const wxUint8 pngHeader[] = { '\211', 'P', 'N', 'G', '\r', '\n', '\032', '\n' };

	wxUint16 width16, height16;
	wxUint16 count, tag, type;
	wxUint32 offset;

	wxUint8 header[8];

	if (!stream->IsOk())
		return;

	// try and determine the filetype not from the filename, but
	// from the file's header
	STREAM_READ(stream, header, sizeof(pngHeader)); // pngHeader is the longest

	if (memcmp(header, jpegHeader, sizeof(jpegHeader)) == 0) {
		// JPEG file

		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;

		// rewind to BEFORE the header, so jpeglib does not get confused
		stream->Ungetch(header, sizeof(pngHeader));

		jpeg_create_decompress(&cinfo);
		cinfo.err = jpeg_std_error(&jerr);
		cinfo.err->error_exit = my_error_exit;

		try {
			jpeg_wx_input_stream_src(&cinfo, stream);
			jpeg_read_header(&cinfo, TRUE);
			jpeg_calc_output_dimensions(&cinfo);
			m_uiWidth = cinfo.output_width;
			m_uiHeight = cinfo.output_height;
		} catch (JpegException &je) {
			cinfo.err->output_message((j_common_ptr)&cinfo);
		}

		jpeg_destroy_decompress(&cinfo);

		m_bitmapType = wxBITMAP_TYPE_JPEG;

	} else if (memcmp(header, gif87Header, sizeof(gif87Header)) == 0 || memcmp(header, gif89Header, sizeof(gif89Header)) == 0) {
		// GIF file

		// The width has already been read
		memcpy(&width16, header+sizeof(gif87Header), sizeof(width16));
		STREAM_READ(stream, &height16, sizeof(height16));
		m_uiWidth = wxUINT16_SWAP_ON_BE(width16);
		m_uiHeight = wxUINT16_SWAP_ON_BE(height16);

		m_bitmapType = wxBITMAP_TYPE_GIF;
	} else if (memcmp(header, pngHeader, sizeof(pngHeader)) == 0) {
		// PNG file
		STREAM_SEEK(stream, 8, wxFromCurrent);
		STREAM_READ(stream, &m_uiWidth, sizeof(m_uiWidth));
		STREAM_READ(stream, &m_uiHeight, sizeof(m_uiHeight));
		m_uiWidth = wxUINT32_SWAP_ON_LE(m_uiWidth);
		m_uiHeight = wxUINT32_SWAP_ON_LE(m_uiHeight);

		m_bitmapType = wxBITMAP_TYPE_PNG;
	} else if (memcmp(header, tiffBigHeader, sizeof(tiffBigHeader)) == 0) {
		// Big Endian TIFF
		// Since TIFF supports both byte orders, there's a lot of code duplication below.

		// The offset has already been read
		memcpy(&offset, header+sizeof(tiffBigHeader), sizeof(offset));
		offset = wxUINT32_SWAP_ON_LE(offset);
		STREAM_SEEK(stream, offset, wxFromStart);
		STREAM_READ(stream, &count, sizeof(count));
		count = wxUINT16_SWAP_ON_LE(count);
		for (wxUint16 i = 0; i < count; i++) {
			STREAM_READ(stream, &tag, sizeof(tag));
			tag = wxUINT16_SWAP_ON_LE(tag);
			if (tag == TIFF_IMAGE_WIDTH) {
				STREAM_READ(stream, &type, sizeof(type));
				type = wxUINT16_SWAP_ON_LE(type);
				STREAM_SEEK(stream, 4, wxFromCurrent); // there's only going to be 1 of these
				if (type == 3) { // SHORT
					STREAM_READ(stream, &width16, sizeof(width16));
					m_uiWidth = wxUINT16_SWAP_ON_LE(width16);
				} else if (type == 4) { // LONG
					STREAM_READ(stream, &m_uiWidth, sizeof(m_uiWidth));
					m_uiWidth = wxUINT32_SWAP_ON_LE(m_uiWidth);
				}
			} else if (tag == TIFF_IMAGE_LENGTH) {
				STREAM_READ(stream, &type, sizeof(type));
				type = wxUINT16_SWAP_ON_LE(type);
				STREAM_SEEK(stream, 4, wxFromCurrent); // there's only going to be 1 of these
				if (type == 3) { // SHORT
					STREAM_READ(stream, &height16, sizeof(height16));
					m_uiHeight = wxUINT16_SWAP_ON_LE(height16);
				} else if (type == 4) { // LONG
					STREAM_READ(stream, &m_uiHeight, sizeof(m_uiHeight));
					m_uiHeight = wxUINT32_SWAP_ON_LE(m_uiHeight);
				}
			}
			if (m_uiWidth && m_uiHeight) // i.e., if m_uiWidth and m_uiHeight have both been set
				break;
		}
		m_bitmapType = wxBITMAP_TYPE_TIF;
	} else if (memcmp(header, tiffLittleHeader, sizeof(tiffLittleHeader)) == 0) {
		// Little Endian TIFF

		// The offset has already been read
		memcpy(&offset, header+sizeof(tiffLittleHeader), sizeof(offset));
		offset = wxUINT32_SWAP_ON_BE(offset);
		STREAM_SEEK(stream, offset, wxFromStart);
		STREAM_READ(stream, &count, sizeof(count));
		count = wxUINT16_SWAP_ON_BE(count);
		for (wxUint16 i = 0; i < count; i++) {
			STREAM_READ(stream, &tag, sizeof(tag));
			tag = wxUINT16_SWAP_ON_BE(tag);
			if (tag == TIFF_IMAGE_WIDTH) {
				STREAM_READ(stream, &type, sizeof(type));
				type = wxUINT16_SWAP_ON_BE(type);
				STREAM_SEEK(stream, 4, wxFromCurrent); // there's only going to be 1 of these
				if (type == 3) { // SHORT
					STREAM_READ(stream, &width16, sizeof(width16));
					m_uiWidth = wxUINT16_SWAP_ON_BE(width16);
				} else if (type == 4) { // LONG
					STREAM_READ(stream, &m_uiWidth, sizeof(m_uiWidth));
					m_uiWidth = wxUINT32_SWAP_ON_BE(m_uiWidth);
				}
			} else if (tag == TIFF_IMAGE_LENGTH) {
				STREAM_READ(stream, &type, sizeof(type));
				type = wxUINT16_SWAP_ON_BE(type);
				STREAM_SEEK(stream, 4, wxFromCurrent); // there's only going to be 1 of these
				if (type == 3) { // SHORT
					STREAM_READ(stream, &height16, sizeof(height16));
					m_uiHeight = wxUINT16_SWAP_ON_BE(height16);
				} else if (type == 4) { // LONG
					STREAM_READ(stream, &m_uiHeight, sizeof(m_uiHeight));
					m_uiHeight = wxUINT32_SWAP_ON_BE(m_uiHeight);
				}
			}
			if (m_uiWidth && m_uiHeight) // i.e., if m_uiWidth and m_uiHeight have both been set
				break;
		}
		m_bitmapType = wxBITMAP_TYPE_TIF;
	}
}


void ComicPage::DestroyAll()
{
	DestroyOriginal();
	DestroyResample();
	DestroyThumbnail();
}


void ComicPage::DestroyOriginal()
{
	wxMutexLocker lock(OriginalLock);
	if (Original.Ok())
		Original.Destroy();
}


void ComicPage::DestroyResample()
{
	wxMutexLocker lock(ResampleLock);
	if (Resample.Ok())
		Resample.Destroy();
	m_bitmapFull = wxBitmap();
	m_bitmapLeft = wxBitmap();
	m_bitmapRight = wxBitmap();
}


void ComicPage::DestroyThumbnail()
{
	wxMutexLocker lock(ThumbnailLock);
	if (Thumbnail.Ok())
		Thumbnail.Destroy();
	m_bitmapThumb = wxBitmap();
}


wxBitmap& ComicPage::GetPage()
{
	wxMutexLocker lock(ResampleLock);
	if (!m_bitmapFull.Ok())
		if (Resample.Ok())
			m_bitmapFull = wxBitmap(Resample);

	return m_bitmapFull;
}


wxBitmap& ComicPage::GetPageLeftHalf()
{
	wxMutexLocker lock(ResampleLock);
	if (!m_bitmapLeft.Ok())
		if (Resample.Ok())
			m_bitmapLeft = wxBitmap(Resample.GetSubImage(wxRect(0, 0, Resample.GetWidth() / 2, Resample.GetHeight())));

	return m_bitmapLeft;
}


wxBitmap& ComicPage::GetPageRightHalf()
{
	wxMutexLocker lock(ResampleLock);
	if (!m_bitmapRight.Ok()) {
		if (Resample.Ok()) {
			wxInt32 rWidth = Resample.GetWidth();
			wxInt32 offset = rWidth / 2;
			m_bitmapRight = wxBitmap(Resample.GetSubImage(wxRect(offset, 0, rWidth - offset, Resample.GetHeight())));
		}
	}

	return m_bitmapRight;
}


wxBitmap& ComicPage::GetThumbnail()
{
	wxMutexLocker lock(ThumbnailLock);
	if (!m_bitmapThumb.Ok())
		if (Thumbnail.Ok())
			m_bitmapThumb = wxBitmap(Thumbnail);

	return m_bitmapThumb;
}


bool ComicPage::IsLandscape()
{
	float rWidth, rHeight;
	if (orientation == NORTH || orientation == SOUTH) {
		rWidth = m_uiWidth;
		rHeight = m_uiHeight;
	} else {
		rHeight = m_uiWidth;
		rWidth = m_uiHeight;
	}
	
	if ((rWidth / rHeight) > 1.0f)
		return true;
	else
		return false;
}
