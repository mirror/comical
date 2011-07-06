/*
 * Exceptions.h
 * Copyright (c) 2005-2011 James Athey
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

#ifndef _Exceptions_h_
#define _Exceptions_h_

#include <cstddef>
#include <cstdio>
#include <jpeglib.h>
#include <stdexcept>
#include <wx/defs.h>
#include <wx/string.h>

class PageOutOfRangeException
{
	public:
		PageOutOfRangeException(wxUint32 pagenumber, wxUint32 pagecount) { PageNumber = pagenumber; PageCount = pagecount; }
		wxUint32 PageNumber, PageCount;
};

class ArchiveException
{
	public:
		ArchiveException(wxString filename, wxString message) { Filename = filename; Message = message; }
		wxString Filename, Message;
};

class JpegException : public std::runtime_error
{
public:
	JpegException(j_common_ptr cinfo);

	const j_common_ptr cinfo;

private:
	char errMsg[JMSG_LENGTH_MAX];
	const char* formatJpegErrorMsg(j_common_ptr cinfo);
};

#endif
