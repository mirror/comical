/*
 * wxRarInputStream.cpp
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

#include "wxRarInputStream.h"
#include <iostream>

#ifndef LPARAM
#define LPARAM long
#endif

extern "C" int CALLBACK unrarThreadCallback(wxUint32 msg, LPARAM UserData, LPARAM P1, LPARAM P2);


wxUnrarThread::wxUnrarThread(wxRarInputStream *stream):
wxThread(wxTHREAD_JOINABLE),
m_stream(stream)
{
}


void* wxUnrarThread::Entry()
{
	RARSetCallback(m_stream->m_rarFile, unrarThreadCallback, (LPARAM)m_stream);
	if (RARProcessFile(m_stream->m_rarFile, RAR_TEST, NULL, NULL) != 0)
		m_stream->m_lasterror = wxSTREAM_READ_ERROR;

	return NULL;
}


wxRarInputStream::wxRarInputStream(const wxString& filename, const wxString& entry, const char* password, bool throttle):
wxInputStream(),
m_unrarThread(this),
m_rarFile(NULL),
m_bThrottle(throttle),
m_mutexThrottle(wxMUTEX_DEFAULT),
m_condThrottle(m_mutexThrottle),
m_iRetval(0),
m_iBytesAvailable(0),
m_iBytesRead(0),
m_iLength(wxInvalidOffset)
{
	struct RAROpenArchiveDataEx flags;
	int RHCode, PFCode;
	wxFileOffset length = 0;

	std::cerr << "Opening page " << entry.ToAscii() << " from " << filename.ToAscii() << std::endl;

	memset(&flags, 0, sizeof(flags));
	memset(&m_rarHeader, 0, sizeof(m_rarHeader));

#ifdef wxUSE_UNICODE
#ifdef __WXOSX__
	const char *filenameData = filename.fn_str().data();
	flags.rcName = new char[strlen(filenameData) + 1];
	strcpy(flags.ArcName, filenameData);
#else
	flags.ArcNameW = const_cast<wchar_t*>(filename.c_str());
#endif
#else // ASCII
	flags.ArcName = const_cast<char*>(filename.c_str());
#endif
	flags.CmtBuf = NULL;
	flags.OpenMode = RAR_OM_EXTRACT;

	m_rarFile = RAROpenArchiveEx(&flags);
	if (flags.OpenResult != 0) {
		m_lasterror = wxSTREAM_READ_ERROR;
		return;
	}

	if (password)
		RARSetPassword(m_rarFile, const_cast<char*>(password));

	while ((RHCode = RARReadHeaderEx(m_rarFile, &m_rarHeader)) == 0) {
#ifdef wxUSE_UNICODE
		if (entry.IsSameAs(wxString(m_rarHeader.FileNameW))) {
#else // ASCII
		if (entry.IsSameAs(m_rarHeader.FileName)) {
#endif
			// Handle archive members bigger than 2 GiB
			if (sizeof(wxFileOffset) == sizeof(unsigned int)) {
				// uh oh, 2 gig file limit...
				if (m_rarHeader.UnpSizeHigh > 0 || m_rarHeader.UnpSize > INT_MAX)
					m_iLength = INT_MAX;
				else
					m_iLength = m_rarHeader.UnpSize;
			} else
				m_iLength = (((wxFileOffset)m_rarHeader.UnpSizeHigh) << 32) +
						(wxFileOffset)m_rarHeader.UnpSize;
			break;
		} else {
			if ((PFCode = RARProcessFile(m_rarFile, RAR_SKIP, NULL, NULL)) != 0) {
				m_lasterror = wxSTREAM_READ_ERROR;
				return;
			}
		}
	}

	if (m_iLength == 0 || m_iLength == wxInvalidOffset || RHCode != 0) {
		m_lasterror = wxSTREAM_READ_ERROR;
		return;
	}

	m_unrarThread.Create();
	m_unrarThread.Run();
}


wxRarInputStream::~wxRarInputStream()
{
	m_iRetval = 1; // stop unrar'ing
	if (m_unrarThread.GetId())
		m_unrarThread.Wait();	// wait for unrar thread to finish

	if (m_rarFile)
		RARCloseArchive(m_rarFile);

	while (!m_queueBuffers.empty()) {
		std::pair<wxUint8*,size_t>& bufpair = m_queueBuffers.front();
		delete[] bufpair.first;
		m_queueBuffers.pop();
	}
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
	return m_iBytesAvailable > 0;
}


wxFileOffset wxRarInputStream::OnSysTell() const
{
	return m_iBytesRead;
}


size_t wxRarInputStream::OnSysRead(void *buffer, size_t size)
{
	while (m_iBytesAvailable <= 0);

	wxMutexLocker lock(m_mutexQueue);
	size_t bytesRead = 0;

	while (size && !m_queueBuffers.empty()) {
		std::pair<wxUint8*,size_t>& bufpair = m_queueBuffers.front();
		if (bufpair.second <= size) {
			memcpy(buffer, bufpair.first, bufpair.second);
			bytesRead += bufpair.second;
			size -= bufpair.second;
			delete[] bufpair.first;
			m_queueBuffers.pop();
		} else { // buffer's length > size
			memcpy(buffer, bufpair.first, size);
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
