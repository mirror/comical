/*
 * JpegWxInputStreamSrc.cpp
 * Copyright (c) 2011 James Athey
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

#include "JpegWxInputStreamSrc.h"

#include <jerror.h>

struct JpegWxInputStreamSrc : public jpeg_source_mgr
{
	wxInputStream* stream;
	JOCTET buffer[4096];
	bool start_of_file;
};


extern "C" {
	static void init_wx_source(j_decompress_ptr cinfo);
	static boolean fill_wx_input_buffer (j_decompress_ptr cinfo);
	static void skip_wx_input_data (j_decompress_ptr cinfo, long num_bytes);
	static void term_wx_source (j_decompress_ptr cinfo);
}


static void init_wx_source(j_decompress_ptr cinfo)
{
	JpegWxInputStreamSrc *wsrc = static_cast<JpegWxInputStreamSrc*>(cinfo->src);

	/* We reset the empty-input-file flag for each image,
	 * but we don't clear the input buffer.
	 * This is correct behavior for reading a series of images from one source.
	 */
	wsrc->start_of_file = true;
}


static boolean fill_wx_input_buffer (j_decompress_ptr cinfo)
{
	JpegWxInputStreamSrc *wsrc = static_cast<JpegWxInputStreamSrc*>(cinfo->src);

	wsrc->stream->Read(wsrc->buffer, sizeof(wsrc->buffer));
	size_t nbytes = wsrc->stream->LastRead();

	if (nbytes <= 0) {
		if (wsrc->start_of_file)	/* Treat empty input file as fatal error */
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		WARNMS(cinfo, JWRN_JPEG_EOF);
		/* Insert a fake EOI marker */
		wsrc->buffer[0] = (JOCTET) 0xFF;
		wsrc->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
	}

	wsrc->next_input_byte = wsrc->buffer;
	wsrc->bytes_in_buffer = nbytes;
	wsrc->start_of_file = false;

	return TRUE;
}


static void skip_wx_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	if (num_bytes <= 0)
		return;

	struct jpeg_source_mgr * src = cinfo->src;

	while (num_bytes > (long)src->bytes_in_buffer)
	{
		num_bytes -= (long) src->bytes_in_buffer;
		src->fill_input_buffer(cinfo);
	}
	src->next_input_byte += (size_t) num_bytes;
	src->bytes_in_buffer -= (size_t) num_bytes;
}


static void term_wx_source (j_decompress_ptr cinfo)
{
	JpegWxInputStreamSrc *wsrc = static_cast<JpegWxInputStreamSrc*>(cinfo->src);

	if (wsrc->bytes_in_buffer > 0)
		wsrc->stream->SeekI(-(long)wsrc->bytes_in_buffer, wxFromCurrent);
}


void jpeg_wx_input_stream_src (j_decompress_ptr cinfo, wxInputStream *stream)
{
	/* The source object and input buffer are made permanent so that a series
	 * of JPEG images can be read from the same file by calling
	 * jpeg_wx_input_stream_src only before the first one.
	 */
	if (cinfo->src == NULL) {	/* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr*)
				(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
	    				  sizeof(struct JpegWxInputStreamSrc));
		static_cast<JpegWxInputStreamSrc*>(cinfo->src)->stream = stream;
	}

	struct jpeg_source_mgr *src = cinfo->src;

	src->init_source = init_wx_source;
	src->fill_input_buffer = fill_wx_input_buffer;
	src->skip_input_data = skip_wx_input_data;
	src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->term_source = term_wx_source;
	src->bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->next_input_byte = NULL; /* until buffer loaded */
}
