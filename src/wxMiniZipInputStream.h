/*
 * wxMiniZipInputStream.h
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

#ifndef _WX_MINI_ZIP_INPUT_STREAM_H_
#define _WX_MINI_ZIP_INPUT_STREAM_H_

#include <wx/stream.h>
#include "unzip.h"

/**
 * The built-in wxZipInputStream does not support password-protected ZIP
 * files, but wxMiniZipInputStream does!
 */
class wxMiniZipInputStream : public wxInputStream
{
public:
	wxMiniZipInputStream(const wxString& filename, const wxString& entry, const char* password=NULL);
	~wxMiniZipInputStream();

	int GetMiniZipError() const { return m_iMiniZipError; }

	virtual wxFileOffset GetLength() const;

protected:
	virtual wxFileOffset OnSysTell() const;
	virtual size_t OnSysRead(void *buffer, size_t size);

private:
	unzFile m_zipFile;
	unz_file_info64 m_fileInfo;
	int m_iMiniZipError;

	void cleanup();

	static wxStreamError convertMiniZipError(int error);
};

#endif /* _WX_MINI_ZIP_INPUT_STREAM_H_ */
