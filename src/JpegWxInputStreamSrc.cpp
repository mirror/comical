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

/* Expanded data source object for stdio input */

class JpegWxInputStreamSrc
{
public:
	JpegWxInputStreamSrc(wxInputStream* stream) : stream(stream), start_of_file(true) {}

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


/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */

static void init_wx_source(j_decompress_ptr cinfo)
{
	JpegWxInputStreamSrc *wsrc = static_cast<JpegWxInputStreamSrc*>(cinfo->client_data);

	/* We reset the empty-input-file flag for each image,
	 * but we don't clear the input buffer.
	 * This is correct behavior for reading a series of images from one source.
	 */
	wsrc->start_of_file = true;
}

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In typical applications, this should read fresh data into the buffer
 * (ignoring the current state of next_input_byte & bytes_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been reloaded.  It is not necessary to
 * fill the buffer entirely, only to obtain at least one more byte.
 *
 * There is no such thing as an EOF return.  If the end of the file has been
 * reached, the routine has a choice of ERREXIT() or inserting fake data into
 * the buffer.  In most cases, generating a warning message and inserting a
 * fake EOI marker is the best course of action --- this will allow the
 * decompressor to output however much of the image is there.  However,
 * the resulting error message is misleading if the real problem is an empty
 * input file, so we handle that case specially.
 */

static boolean fill_wx_input_buffer (j_decompress_ptr cinfo)
{
	struct jpeg_source_mgr * src = cinfo->src;
	JpegWxInputStreamSrc *wsrc = static_cast<JpegWxInputStreamSrc*>(cinfo->client_data);

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

	src->next_input_byte = wsrc->buffer;
	src->bytes_in_buffer = nbytes;
	wsrc->start_of_file = false;

	return TRUE;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 */

static void skip_wx_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	struct jpeg_source_mgr * src = cinfo->src;
	JpegWxInputStreamSrc *wsrc = static_cast<JpegWxInputStreamSrc*>(cinfo->client_data);

	wsrc->stream->SeekI((off_t)num_bytes, wxFromCurrent);
	if (num_bytes < (long) src->bytes_in_buffer) {
		src->next_input_byte += (size_t) num_bytes;
		src->bytes_in_buffer -= (size_t) num_bytes;
	} else {
		src->next_input_byte = wsrc->buffer;
		src->bytes_in_buffer = 0;
	}
}


static void term_wx_source (j_decompress_ptr cinfo)
{
	delete static_cast<JpegWxInputStreamSrc*>(cinfo->client_data);
	cinfo->client_data = NULL;
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
	    				  sizeof(struct jpeg_source_mgr));
		cinfo->client_data = new JpegWxInputStreamSrc(stream);
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
