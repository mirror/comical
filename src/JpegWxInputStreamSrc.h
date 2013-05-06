/*
 * JpegWxInputStreamSrc.h
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

#ifndef JPEGWXINPUTSTREAMSRC_H_
#define JPEGWXINPUTSTREAMSRC_H_

#include <wx/stream.h>
#define XMD_H
#include <jpeglib.h>
#ifndef __MINGW32__
typedef int jpeg_boolean;
#endif

void jpeg_wx_input_stream_src (j_decompress_ptr cinfo, wxInputStream *stream);

#endif /* JPEGWXINPUTSTREAMSRC_H_ */
