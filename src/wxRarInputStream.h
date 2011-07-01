/*
 * wxRarInputStream.h
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

#ifndef _WX_RAR_INPUT_STREAM_H_
#define _WX_RAR_INPUT_STREAM_H_

#include <wx/stream.h>
#include <wx/thread.h>

#include <queue>
#include <utility>

#ifdef _WIN32
#include <windef.h>
#endif
#include "dll.hpp"

class wxRarInputStream;

class wxUnrarThread : public wxThread
{
public:
	wxUnrarThread(wxRarInputStream* stream);

protected:
	virtual void* Entry();

private:
	wxRarInputStream *m_stream;
};


class wxRarInputStream : public wxInputStream
{
	friend class wxUnrarThread;

public:
	wxRarInputStream(const wxString& filename, const wxString& entry, const char* password=NULL, bool throttle=false);
	~wxRarInputStream();

	// from wxStreamBase
	virtual wxFileOffset GetLength() const;

	// Interface for unrar callback function
	void PushNewBuffer(wxUint8* buffer, size_t size);
	void CheckThrottle();
	int GetCallbackRetval() const { return m_iRetval; }

	virtual bool CanRead() const;


protected:
	// from wxStreamBase
	virtual wxFileOffset OnSysTell() const;

	// from wxInputStream
	virtual size_t OnSysRead(void *buffer, size_t size);

private:
	wxUnrarThread m_unrarThread;

	HANDLE m_rarFile;
	struct RARHeaderDataEx m_rarHeader;

	const bool m_bThrottle;
	/**
	 * If throttling is on, then the decompression thread will pause when more
	 * than this threshold is available for reading.
	 */
	static const size_t READAHEAD_BYTES_THROTTLE_THRESHOLD = 8092;

	wxMutex m_mutexThrottle;
	wxCondition m_condThrottle;

	/**
	 * If the stream is closed before decompression has finished, this value
	 * will change to 1 to tell unrar to stop after the next decompression
	 * callback.
	 */
	int m_iRetval;

	volatile wxFileOffset m_iBytesAvailable;
	volatile wxFileOffset m_iBytesRead;
	wxFileOffset m_iLength;

	/**
	 * A queue of buffers, where each buffer is a pair containing a new[]
	 * allocated array of bytes and a length.
	 */
	std::queue< std::pair<wxUint8*,size_t> > m_queueBuffers;

	/**
	 * Regulates access to the buffer queue as a shared resource.  Both
	 * PushNewBuffer() and OnSysRead() grab the mutex, since the push and the
	 * read are done in different threads.
	 */
	wxMutex m_mutexQueue;
};

#endif /* _WX_RAR_INPUT_STREAM_H_ */
