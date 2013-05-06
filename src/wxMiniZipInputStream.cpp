/*
 * wxMiniZipInputStream.cpp
 * Copyright (c) 2011 James Athey. 2012, John Peterson.
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
		const wxString password):
wxInputStream(),
m_zipFile(NULL),
m_iMiniZipError(UNZ_OK),
m_filename(filename),
m_entry(entry),
password(password)
{
	Open();
}

wxMiniZipInputStream::~wxMiniZipInputStream()
{
	Close();
}

bool wxMiniZipInputStream::Open()
{
	memset(&m_fileInfo, 0, sizeof(m_fileInfo));

	// TODO what minizip / zlib errors set errno?

	m_zipFile = unzOpen64(m_filename.fn_str());

	if (!m_zipFile) {
		m_iMiniZipError = UNZ_BADZIPFILE;
		Close();
		return false;
	}

	if((m_iMiniZipError = unzLocateFile(m_zipFile, m_entry.ToUTF8(), 0)) != UNZ_OK) {
		Close();
		return false;
	}

	if ((m_iMiniZipError = unzGetCurrentFileInfo64(m_zipFile, &m_fileInfo, NULL, 0, NULL, 0, NULL, 0)) != UNZ_OK) {
		Close();
		return false;
	}
	
	if (!password.IsEmpty())
		m_iMiniZipError = unzOpenCurrentFilePassword(m_zipFile, password.char_str());
	else
		m_iMiniZipError = unzOpenCurrentFile(m_zipFile);

	if (m_iMiniZipError != UNZ_OK) {
		Close();
		return false;
	}
	return true;
}

void wxMiniZipInputStream::Close()
{
	m_lasterror = convertMiniZipError(m_iMiniZipError);
	if (m_zipFile) {
		if (m_fileInfo.version)
			unzCloseCurrentFile(m_zipFile);
		unzClose(m_zipFile);
		m_zipFile = NULL;
	}
}


wxFileOffset wxMiniZipInputStream::GetLength() const
{
	if (m_fileInfo.version)
		return m_fileInfo.uncompressed_size;
	else
		return wxInvalidOffset;
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

wxFileOffset wxMiniZipInputStream::OnSysSeek(wxFileOffset pos, wxSeekMode mode)
{
	wxFileOffset pos_arg = pos, read;

	switch (mode)
	{
		case wxFromStart:
			read = pos;
			break;
			
		case wxFromCurrent:
			read = pos < 0 ? TellI() + pos : pos;
			pos = TellI() + pos;
			break;
			
		case wxFromEnd:
			pos = GetLength() + pos;
			read = pos;
			break;

		default:
			wxFAIL_MSG( wxT("invalid seek mode") );
			return wxInvalidOffset;
	}
	
	wxCHECK_MSG(pos >= 0 && pos <= GetLength(), wxInvalidOffset, _("cannot seek outside file"));
	
	if (mode == wxFromStart && TellI() > 0 || mode == wxFromCurrent && pos_arg < 0 || mode == wxFromEnd) {
		Close();
		if (!Open()) return wxInvalidOffset;
	}
	
	// the temporary buffer size used when copying from stream to stream
	const wxUint16 BUF_TEMP_SIZE = 4096;
	
	// rather than seeking, we can just read data and discard it;
	// this allows to forward-seek also non-seekable streams!
	char buf[BUF_TEMP_SIZE];
	size_t bytes_read;

	// read chunks of BUF_TEMP_SIZE bytes until we reach the new position
	for ( ; read >= BUF_TEMP_SIZE; read -= bytes_read)
	{
		bytes_read = Read(buf, WXSIZEOF(buf)).LastRead();
		if ( m_lasterror != wxSTREAM_NO_ERROR )
			return wxInvalidOffset;

		wxASSERT(bytes_read == WXSIZEOF(buf));
	}

	// read the last 'pos' bytes
	bytes_read = Read(buf, (size_t)read).LastRead();
	if ( m_lasterror != wxSTREAM_NO_ERROR )
		return wxInvalidOffset;

	wxASSERT(bytes_read == (size_t)read);

	// we should now have sought to the right position...
	return TellI();
}


wxFileOffset wxMiniZipInputStream::OnSysTell() const
{
	if (m_zipFile)
		return unztell64(m_zipFile);
	else
		return wxInvalidOffset;
}

size_t wxMiniZipInputStream::OnSysRead(void* buffer, size_t size)
{
	int bytesRead;
	if ((bytesRead = unzReadCurrentFile(m_zipFile, buffer, size)) < 0) {
		m_iMiniZipError = bytesRead;
		m_lasterror = convertMiniZipError(m_iMiniZipError);
		bytesRead = 0;
	}
}