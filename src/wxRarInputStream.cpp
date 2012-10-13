/*
 * wxRarInputStream.cpp
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

#include "wxRarInputStream.h"
#include <iostream>

extern "C" int CALLBACK unrarThreadCallback(wxUint32 msg, LPARAM UserData, LPARAM P1, LPARAM P2);

wxUnrarThread::wxUnrarThread(wxRarInputStream *stream):
wxThread(wxTHREAD_JOINABLE),
m_stream(stream)
{
}


void* wxUnrarThread::Entry()
{
	RARSetCallback(m_stream->m_file, unrarThreadCallback, (LPARAM)m_stream);
	if (RARProcessFile(m_stream->m_file, RAR_TEST, NULL, NULL) != 0)
		m_stream->m_lasterror = wxSTREAM_READ_ERROR;

	return NULL;
}

wxRarInputStream::wxRarInputStream(const wxString& filename, const wxString& entry, const wxString password, bool throttle):
wxInputStream(),
m_file(NULL),
m_filename(filename),
m_entry(entry),
password(password),
m_bThrottle(throttle),
m_mutexThrottle(wxMUTEX_DEFAULT),
m_condThrottle(m_mutexThrottle),
m_iRetval(0),
m_iBytesAvailable(0),
m_iBytesRead(0),
m_iLength(wxInvalidOffset)
{
	Open();
}

wxRarInputStream::~wxRarInputStream()
{
	Close();
}

bool wxRarInputStream::Open()
{
	struct RAROpenArchiveDataEx flags;
	int RHCode, PFCode;

	memset(&flags, 0, sizeof(flags));
	memset(&m_Header, 0, sizeof(m_Header));

	flags.ArcNameW = const_cast<wchar_t*>(m_filename.wc_str());
	flags.CmtBuf = NULL;
	flags.OpenMode = RAR_OM_EXTRACT;

	m_file = RAROpenArchiveEx(&flags);
	if (flags.OpenResult != 0) {
		m_lasterror = wxSTREAM_READ_ERROR;
		return false;
	}

	if (!password.IsEmpty())
		RARSetPassword(m_file, password.char_str());

	while ((RHCode = RARReadHeaderEx(m_file, &m_Header)) == 0) {
		if (m_entry.Cmp(m_Header.FileNameW) == 0) {
			// Handle archive members bigger than 2 GiB
			if (sizeof(wxFileOffset) == sizeof(unsigned int)) {
				// uh oh, 2 gig file limit...
				if (m_Header.UnpSizeHigh > 0 || m_Header.UnpSize > INT_MAX)
					m_iLength = INT_MAX;
				else
					m_iLength = m_Header.UnpSize;
			} else
				m_iLength = (((wxFileOffset)m_Header.UnpSizeHigh) << 32) +
						(wxFileOffset)m_Header.UnpSize;
			break;
		} else {
			if ((PFCode = RARProcessFileW(m_file, RAR_SKIP, NULL, NULL)) != 0) {
				m_lasterror = wxSTREAM_READ_ERROR;
				return false;
			}
		}
	}

	if (m_iLength == 0 || m_iLength == wxInvalidOffset || RHCode != 0) {
		m_lasterror = wxSTREAM_READ_ERROR;
		return false;
	}
	
	m_unrarThread = new wxUnrarThread(this);
	m_unrarThread->Create();
	m_unrarThread->Run();
	
	return true;
}

void wxRarInputStream::Close()
{
	m_iRetval = 1; // stop unrar'ing
	if (m_unrarThread->GetId())
		m_unrarThread->Wait();	// wait for unrar thread to finish

	if (m_file)
		RARCloseArchive(m_file);

	while (!m_queueBuffers.empty()) {
		std::pair<wxUint8*,size_t>& bufpair = m_queueBuffers.front();
		delete[] bufpair.first;
		m_queueBuffers.pop();
	}
	
	m_iRetval = 0;
	m_iBytesAvailable = 0;
	m_iBytesRead = 0;
}


wxFileOffset wxRarInputStream::GetLength() const
{
	return m_iLength;
}


void wxRarInputStream::PushNewBuffer(wxUint8* buffer, size_t size)
{
	if (size <= 0)
		return;

	wxUint8* copy = new wxUint8[size];
	memcpy(copy, buffer, size);

	wxMutexLocker lock(m_mutexQueue); // lock the buffers until return
	m_queueBuffers.push(std::pair<wxUint8*,size_t>(copy, size));

	m_iBytesAvailable += size;
}


void wxRarInputStream::CheckThrottle()
{
	if (!m_bThrottle)
		return;

	m_mutexThrottle.Lock();
	if (m_iBytesAvailable > READAHEAD_BYTES_THROTTLE_THRESHOLD)
		m_condThrottle.Wait();

	m_mutexThrottle.Unlock();
}


bool wxRarInputStream::CanRead() const
{
	return m_iBytesAvailable > 0 && m_iBytesRead < m_iLength;
}

wxFileOffset wxRarInputStream::OnSysSeek(wxFileOffset pos, wxSeekMode mode)
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

wxFileOffset wxRarInputStream::OnSysTell() const
{
	return m_iBytesRead;
}


size_t wxRarInputStream::OnSysRead(void *buffer, size_t size)
{
	while (m_iBytesAvailable <= 0 && m_iBytesRead < m_iLength) wxThread::This()->Sleep(1);

	wxMutexLocker lock(m_mutexQueue);
	size_t bytesRead = 0;

	while (size && !m_queueBuffers.empty()) {
		std::pair<wxUint8*,size_t>& bufpair = m_queueBuffers.front();
		if (bufpair.second <= size) {
			memcpy(buffer + bytesRead, bufpair.first, bufpair.second);
			bytesRead += bufpair.second;
			size -= bufpair.second;
			delete[] bufpair.first;
			m_queueBuffers.pop();
		} else { // buffer's length > size
			memcpy(buffer + bytesRead, bufpair.first, size);
			bytesRead += size;

			// move the remaining bytes to a new buffer so that the original
			// buffer can be deleted additional extra bookkeeping
			bufpair.second -= size;
			wxUint8 *leftover = new wxUint8[bufpair.second];
			memcpy(leftover, bufpair.first + size, bufpair.second);
			delete[] bufpair.first;
			bufpair.first = leftover;

			size = 0;
		}
	}

	m_iBytesRead += bytesRead;

	if (m_bThrottle) {
		m_mutexThrottle.Lock();
		m_iBytesAvailable -= bytesRead;
		m_mutexThrottle.Unlock();
		if (m_iBytesAvailable < READAHEAD_BYTES_THROTTLE_THRESHOLD)
			m_condThrottle.Signal();
	} else
		m_iBytesAvailable -= bytesRead;

	return bytesRead;
}


int CALLBACK unrarThreadCallback(wxUint32 msg, LPARAM userData, LPARAM addr, LPARAM length)
{
	wxRarInputStream *stream = reinterpret_cast<wxRarInputStream*>(userData);
	wxUint8* buffer = reinterpret_cast<wxUint8*>(addr);

	switch(msg) {

	case UCM_CHANGEVOLUME:
		// No support for multi-volume RARs yet
		return -1;
	case UCM_PROCESSDATA:
		stream->PushNewBuffer(buffer, length);
		break;
	case UCM_NEEDPASSWORD:
		// The password should already have been set
		return -1;
	}

	// If throttling is on and the stream has read enough data, the unrar
	// thread will pause until the bytes available passes below the threshold.
	stream->CheckThrottle();

	return stream->GetCallbackRetval();
}