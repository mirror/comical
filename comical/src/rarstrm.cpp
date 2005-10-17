/***************************************************************************
 *   Copyright (C) 2005 by James Leighton Athey                            *
 *   jathey@comcast.net                                                    *
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
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef __GNUG__
#pragma implementation "rarstrm.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_STREAMS

#include "rarstrm.h"

/* This implementation is far from ideal.  This carpenter blames his tool.  The
 * unrar dll interface is awful, and the source code of the library should be
 * used as an example of how NOT to write C++ code.
 * So anyway, ideally this class would only extract data from the RAR when
 * OnSysRead is called, but instead I have to extract the whole file first
 * and then pretend that it's a stream.
 */
wxRarInputStream::wxRarInputStream(const wxString& archive, const wxString& file) : wxInputStream()
{
	HANDLE RarFile;
	int RHCode,PFCode;
	char CmtBuf[16384];
	struct RARHeaderDataEx HeaderData;
	struct RAROpenArchiveDataEx OpenArchiveData;
	char * CallBackBuffer;
	
	m_Pos = 0;
	m_Size = 0;
	m_ArcName = archive;
	
	memset(&OpenArchiveData,0,sizeof(OpenArchiveData));
#ifdef wxUSE_UNICODE
	OpenArchiveData.ArcNameW = (wchar_t *) archive.c_str();
#else
	OpenArchiveData.ArcName = (char *) archive.c_str();
#endif
	OpenArchiveData.CmtBuf = CmtBuf;
	OpenArchiveData.CmtBufSize = sizeof(CmtBuf);
	OpenArchiveData.OpenMode = RAR_OM_EXTRACT;
	RarFile=RAROpenArchiveEx(&OpenArchiveData);

	if ((RHCode = OpenArchiveData.OpenResult) != 0) {
		m_lasterror = wxSTREAM_READ_ERROR;
		throw new ArchiveException(m_ArcName, OpenArchiveError(RHCode));
	}

	HeaderData.CmtBuf=NULL;

	while ((RHCode=RARReadHeaderEx(RarFile,&HeaderData))==0) {
#ifdef wxUSE_UNICODE
		if (file.IsSameAs(wxString(HeaderData.FileNameW))) {
#else // ANSI
		if (file.IsSameAs(HeaderData.FileName)) {
#endif
			m_Size = HeaderData.UnpSize;
			break;	
		}
		else
			RARProcessFile(RarFile,RAR_SKIP,NULL,NULL);
	}
	
	if (m_Size == 0) { // archived file not found
		m_lasterror = wxSTREAM_READ_ERROR;
		throw new ArchiveException(m_ArcName, file + wxT(" not found in ") + archive + wxT("."));
	}

	m_Buffer = new char[m_Size];
	CallBackBuffer = m_Buffer;

	RARSetCallback(RarFile, CallbackProc, (long) &CallBackBuffer);

	PFCode = RARProcessFile(RarFile, RAR_TEST, NULL, NULL);

	if (PFCode!=0) {
		m_lasterror = wxSTREAM_READ_ERROR;
		throw new ArchiveException(m_ArcName, ProcessFileError(PFCode, file));	
	}

	if (RarFile)  
		RARCloseArchive(RarFile);

}

wxRarInputStream::~wxRarInputStream()
{
	delete[] m_Buffer;
}

bool wxRarInputStream::Eof() const
{
	wxASSERT_MSG( m_Pos <= (off_t)m_Size, wxT("wxRarInputStream: invalid current position") );
	return m_Pos >= (off_t)m_Size;
}

size_t wxRarInputStream::OnSysRead(void *buffer, size_t bufsize)
{
	wxASSERT_MSG( m_Pos <= (off_t)m_Size, wxT("wxRarInputStream: invalid current position") );
	if (m_Pos >= (off_t)m_Size) {
		m_lasterror = wxSTREAM_EOF;
		return 0;
	}
    
	if (m_Pos + bufsize > m_Size)
		bufsize = m_Size - m_Pos;

	memcpy(buffer, m_Buffer + m_Pos, bufsize);
	
	m_Pos += bufsize;

	return bufsize;
}

off_t wxRarInputStream::OnSysSeek(off_t seek, wxSeekMode mode)
{
    off_t nextpos;

    switch (mode) {
        case wxFromCurrent : nextpos = seek + m_Pos; break;
        case wxFromStart : nextpos = seek; break;
        case wxFromEnd : nextpos = m_Size - 1 + seek; break;
        default : nextpos = m_Pos; break; /* just to fool compiler, never happens */
    }

    m_Pos = nextpos;
    return m_Pos;
}

wxString wxRarInputStream::OpenArchiveError(int Error)
{
	wxString prefix = wxT("Could not open ") + m_ArcName;
	switch(Error) {
		case ERAR_NO_MEMORY:
			return wxString(prefix + wxT(": out of memory"));
		case ERAR_EOPEN:
			return wxString(prefix);
		case ERAR_BAD_ARCHIVE:
			return wxString(prefix + wxT(": it is not a valid RAR archive"));
		case ERAR_BAD_DATA:
			return wxString(prefix + wxT(": archive header broken"));
		case ERAR_UNKNOWN:
			return wxString(prefix + wxT(": unknown error"));
	}
}

wxString wxRarInputStream::ProcessFileError(int Error, wxString compressedFile)
{
	wxString prefix = wxT("Error processing ") + compressedFile;
	switch(Error) {
		case ERAR_UNKNOWN_FORMAT:
			return wxString(prefix + wxT(": unknown archive format"));
		case ERAR_BAD_ARCHIVE:
			return wxString(prefix + wxT(": invalid or corrupted volume"));
		case ERAR_ECREATE:
			return wxString(prefix + wxT(": could not create the file"));
		case ERAR_EOPEN:
			return wxString(prefix + wxT(": could not open the file"));
		case ERAR_ECLOSE:
			return wxString(prefix + wxT(": could not close the file"));
		case ERAR_EREAD:
			return wxString(prefix + wxT(": could not read the file"));
		case ERAR_EWRITE:
			return wxString(prefix + wxT(": could not write the file"));
		case ERAR_BAD_DATA:
			return wxString(prefix + wxT(": CRC error"));
		case ERAR_UNKNOWN:
			return wxString(prefix + wxT(": Unknown error"));
	}
}

int CALLBACK CallbackProc(wxUint32 msg, long UserData, long P1, long P2)
{
	char **buffer;
	switch(msg) {

	case UCM_CHANGEVOLUME:
		break;
	case UCM_PROCESSDATA:
		buffer = (char **) UserData;
		memcpy(*buffer, (char *)P1, P2);
		// advance the buffer ptr, original m_buffer ptr is untouched
		*buffer += P2;
		break;
	case UCM_NEEDPASSWORD:
		break;
	}
	return(0);
}

#endif
