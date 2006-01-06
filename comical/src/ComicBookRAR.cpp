/***************************************************************************
                              ComicBookRAR.cpp
                             -------------------
    begin                : Wed Oct 29 2003
    copyright            : (C) 2005 by James Athey
    email                : jathey@comcast.net
 ***************************************************************************/

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

#include "ComicBookRAR.h"
#include <wx/mstream.h>
#include "dll.hpp"
#include "Exceptions.h"

ComicBookRAR::ComicBookRAR(wxString file) : ComicBook(file)
{
	HANDLE RarFile;
	int RHCode,PFCode;
	char CmtBuf[16384];
	struct RARHeaderDataEx HeaderData;
	struct RAROpenArchiveDataEx OpenArchiveData;
	wxString page;
	
	memset(&OpenArchiveData, 0, sizeof(OpenArchiveData));
#ifdef wxUSE_UNICODE
#ifdef __WXOSX__
	OpenArchiveData.ArcName = filename.fn_str().data();
#else
	OpenArchiveData.ArcNameW = (wchar_t *) filename.wc_str(wxConvLocal);
#endif
#else // ANSI
	OpenArchiveData.ArcName = (char *) filename.c_str();
#endif
	OpenArchiveData.CmtBuf=CmtBuf;
	OpenArchiveData.CmtBufSize=sizeof(CmtBuf);
	OpenArchiveData.OpenMode=RAR_OM_LIST;
	RarFile=RAROpenArchiveEx(&OpenArchiveData);

	if (OpenArchiveData.OpenResult != 0)
		throw ArchiveException(filename, OpenArchiveError(OpenArchiveData.OpenResult));

	HeaderData.CmtBuf=CmtBuf;
	HeaderData.CmtBufSize=sizeof(CmtBuf);
	
	while ((RHCode = RARReadHeaderEx(RarFile, &HeaderData)) == 0) {
#ifdef wxUSE_UNICODE
		page = wxString(HeaderData.FileNameW);
#else
		page = wxString(HeaderData.FileName);
#endif
		if(page.Right(5).Upper() == wxT(".JPEG") || page.Right(4).Upper() == wxT(".JPG") ||
		page.Right(5).Upper() == wxT(".TIFF") || page.Right(4).Upper() == wxT(".TIF") ||
		page.Right(4).Upper() == wxT(".GIF") ||
		page.Right(4).Upper() == wxT(".PNG"))
			Filenames->Add(page);
		
		if ((PFCode = RARProcessFile(RarFile, RAR_SKIP, NULL, NULL)) != 0)
			throw ArchiveException(filename, ProcessFileError(PFCode, page));
	}

	if (RHCode == ERAR_BAD_DATA)
		throw ArchiveException(filename, OpenArchiveError(RHCode));

	RARCloseArchive(RarFile);
	
	Filenames->Sort();
	Filenames->Shrink();
	
	pageCount = Filenames->GetCount();
	
	originals = new wxImage[pageCount];
	resamples = new wxImage[pageCount];
	thumbnails = new wxImage[pageCount];
	resampleLockers = new wxMutex[pageCount];
	thumbnailLockers = new wxMutex[pageCount];
	Orientations = new COMICAL_ROTATE[pageCount]; // NORTH == 0
	for (wxUint32 i = 0; i < pageCount; i++)
		Orientations[i] = NORTH;
	
	Create(); // create the wxThread
}

wxInputStream * ComicBookRAR::ExtractStream(wxUint32 pageindex)
{
	HANDLE RarFile;
	int RHCode, PFCode;
	char CmtBuf[16384];
	struct RARHeaderDataEx HeaderData;
	struct RAROpenArchiveDataEx OpenArchiveData;
	char * CallBackBuffer;
	
	wxString file = Filenames->Item(pageindex);
	
	m_Pos = 0;
	m_Size = 0;
	m_ArcName = archive;
	
	memset(&OpenArchiveData, 0, sizeof(OpenArchiveData));
#ifdef wxUSE_UNICODE
#ifdef __WXOSX__
	OpenArchiveData.ArcName = filename.fn_str().data();	
#else
	OpenArchiveData.ArcNameW = (wchar_t *) filename.wc_str(wxConvLocal);
#endif
#else
	OpenArchiveData.ArcName = (char *) filename.c_str();
#endif
	OpenArchiveData.CmtBuf = CmtBuf;
	OpenArchiveData.CmtBufSize = sizeof(CmtBuf);
	OpenArchiveData.OpenMode = RAR_OM_EXTRACT;
	RarFile = RAROpenArchiveEx(&OpenArchiveData);

	if ((RHCode = OpenArchiveData.OpenResult) != 0) {
		m_lasterror = wxSTREAM_READ_ERROR;
		throw new ArchiveException(m_ArcName, OpenArchiveError(RHCode));
	}

	HeaderData.CmtBuf=NULL;

	while ((RHCode = RARReadHeaderEx(RarFile, &HeaderData)) == 0) {
#ifdef wxUSE_UNICODE
		if (file.IsSameAs(wxString(HeaderData.FileNameW))) {
#else // ANSI
		if (file.IsSameAs(HeaderData.FileName)) {
#endif
			m_Size = HeaderData.UnpSize;
			break;	
		} else {
			if ((PFCode = RARProcessFile(RarFile, RAR_SKIP, NULL, NULL)) != 0)
				throw ArchiveException(archive, ProcessFileError(PFCode, file));
		}
	}
	
	if (m_Size == 0) { // archived file not found
		m_lasterror = wxSTREAM_READ_ERROR;
		throw new ArchiveException(m_ArcName, file + wxT(" not found in ") + archive + wxT("."));
	}

	m_Buffer = new char[m_Size];
	CallBackBuffer = m_Buffer;

	RARSetCallback(RarFile, CallbackProc, (long) &CallBackBuffer);

	PFCode = RARProcessFile(RarFile, RAR_TEST, NULL, NULL);

	if (PFCode != 0) {
		m_lasterror = wxSTREAM_READ_ERROR;
		throw new ArchiveException(m_ArcName, ProcessFileError(PFCode, file));	
	}

	if (RarFile)  
		RARCloseArchive(RarFile);

	return new wxMemoryInputStream(m_Buffer, m_Size);
}

wxString ComicBookRAR::OpenArchiveError(int Error)
{
	wxString prefix = wxT("Could not open ") + filename;
	switch(Error) {
		case ERAR_NO_MEMORY:
			return wxString(prefix + wxT(": out of memory"));
		case ERAR_EOPEN:
			return prefix;
		case ERAR_BAD_ARCHIVE:
			return wxString(prefix + wxT(": it is not a valid RAR archive"));
		case ERAR_BAD_DATA:
			return wxString(prefix + wxT(": archive header broken"));
		case ERAR_UNKNOWN:
			return wxString(prefix + wxT(": unknown error"));
		default:
			return prefix;
	}
}

wxString ComicBookRAR::ProcessFileError(int Error, wxString compressedFile)
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
		default:
			return prefix;
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
		// implementation needed!
		break;
	}
	return(0);
}
