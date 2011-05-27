/*
 * wxMiniZipInputStream.cpp
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

#include "wxMiniZipInputStream.h"

wxMiniZipInputStream::wxMiniZipInputStream(
		const wxString& filename,
		const wxString& entry,
		const char* password):
wxInputStream(),
m_zipFile(NULL),
m_iMiniZipError(UNZ_OK)
{
	wxUint8 *buffer, *inc_buffer;
	uLong length;

	memset(&m_fileInfo, 0, sizeof(m_fileInfo));

	// TODO what minizip / zlib errors set errno?

	m_zipFile = unzOpen64(filename.fn_str());

	if (!m_zipFile) {
		m_iMiniZipError = UNZ_BADZIPFILE;
		cleanup();
		return;
	}

	if((m_iMiniZipError = unzLocateFile(m_zipFile, entry.ToUTF8(), 0)) != UNZ_OK) {
		cleanup();
		return;
	}

	if ((m_iMiniZipError = unzGetCurrentFileInfo64(m_zipFile, &m_fileInfo, NULL, 0, NULL, 0, NULL, 0)) != UNZ_OK) {
		cleanup();
		return;
	}

	if (password)
		m_iMiniZipError = unzOpenCurrentFilePassword(m_zipFile, password);
	else
		m_iMiniZipError = unzOpenCurrentFile(m_zipFile);

	if (m_iMiniZipError != UNZ_OK) {
		cleanup();
		return;
	}
}


wxMiniZipInputStream::~wxMiniZipInputStream()
{
	cleanup();
}


wxFileOffset wxMiniZipInputStream::GetLength() const
{
	if (m_fileInfo.version)
		return m_fileInfo.uncompressed_size;
	else
		return wxInvalidOffset;
}


void wxMiniZipInputStream::cleanup()
{
	m_lasterror = convertMiniZipError(m_iMiniZipError);
	if (m_zipFile) {
		if (m_fileInfo.version)
			unzCloseCurrentFile(m_zipFile);
		unzClose(m_zipFile);
		m_zipFile = NULL;
	}
}


wxStreamError wxMiniZipInputStream::convertMiniZipError(int error)
{
	switch(error) {
	case UNZ_OK:
		return wxSTREAM_NO_ERROR;
	default:
		return wxSTREAM_READ_ERROR;
	}
}


size_t wxMiniZipInputStream::OnSysRead(void* buffer, size_t size)
{
	int bytesRead;
	if ((bytesRead = unzReadCurrentFile(m_zipFile, buffer, size)) < 0) {
		m_iMiniZipError = bytesRead;
		m_lasterror = convertMiniZipError(m_iMiniZipError);
		bytesRead = 0;
	}

	return bytesRead;
}


wxFileOffset wxMiniZipInputStream::OnSysTell() const
{
	if (m_zipFile)
		return unztell64(m_zipFile);
	else
		return wxInvalidOffset;
}
