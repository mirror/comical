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
#include <wx/textdlg.h>
#include "Exceptions.h"

#ifdef wxUSE_UNICODE
#include <wchar.h>
#else
#include <cstring>
#endif

ComicBookRAR::ComicBookRAR(wxString file) : ComicBook(file)
{
	HANDLE RarFile;
	int RHCode,PFCode;
	struct RARHeaderDataEx HeaderData;
	struct RAROpenArchiveDataEx OpenArchiveData;
	wxString page, new_password;
	
	memset(&OpenArchiveData, 0, sizeof(OpenArchiveData));
#ifdef wxUSE_UNICODE
#ifdef __WXOSX__
	OpenArchiveData.ArcName = new char[filename.Length() + 1];
	strcpy(OpenArchiveData.ArcName, filename.fn_str().data());
#else
	OpenArchiveData.ArcNameW = new wchar_t[filename.Length() + 1];
	wcscpy(OpenArchiveData.ArcNameW, filename.c_str());
#endif
#else // ANSI
	OpenArchiveData.ArcName = new char[filename.Length() + 1];
	strcpy(OpenArchiveData.ArcName, filename.c_str());
#endif
	open_rar:
	OpenArchiveData.OpenResult = 0;
	OpenArchiveData.CmtBuf = NULL;
	OpenArchiveData.OpenMode = RAR_OM_LIST;

	RarFile = RAROpenArchiveEx(&OpenArchiveData);
	if (OpenArchiveData.OpenResult != 0)
		throw ArchiveException(filename, OpenArchiveError(OpenArchiveData.OpenResult));

	if (password)
		RARSetPassword(RarFile, password);

	HeaderData.CmtBuf = NULL;

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
		
		if ((PFCode = RARProcessFile(RarFile, RAR_SKIP, NULL, NULL)) != 0) {
			RARCloseArchive(RarFile);
			throw ArchiveException(filename, ProcessFileError(PFCode, page));
		}
	}

	RARCloseArchive(RarFile);
	
	// Determine whether the files AND the headers in this RAR are password-protected
	if (RHCode == ERAR_UNKNOWN || RHCode == ERAR_BAD_DATA) { // ERAR_UNKNOWN means no password specified, ERAR_BAD_DATA means wrong password
		new_password = wxGetPasswordFromUser(
				wxT("This archive is password-protected.  Please enter the password."),
				wxT("Enter Password"));
		if (new_password.IsEmpty()) // the dialog was cancelled, and the archive cannot be opened
			throw ArchiveException(filename, wxT("Could not open the file, because it is password-protected."));
		SetPassword(new_password.ToAscii());
		goto open_rar;
	}

#ifdef wxUSE_UNICODE
#ifdef __WXOSX__
	delete[] OpenArchiveData.ArcName;
#else
	delete[] OpenArchiveData.ArcNameW;
#endif
#else
	delete[] OpenArchiveData.ArcName;
#endif
	
	postCtor();
	Create(); // create the wxThread
}

wxInputStream * ComicBookRAR::ExtractStream(wxUint32 pageindex)
{
	HANDLE RarFile;
	int RHCode, PFCode;
	struct RARHeaderDataEx HeaderData;
	struct RAROpenArchiveDataEx OpenArchiveData;
	
	wxString page = Filenames->Item(pageindex);
	size_t length = 0;
	
	memset(&OpenArchiveData, 0, sizeof(OpenArchiveData));
#ifdef wxUSE_UNICODE
#ifdef __WXOSX__
	OpenArchiveData.ArcName = new char[filename.Length() + 1];
	strcpy(OpenArchiveData.ArcName, filename.fn_str().data());
#else
	OpenArchiveData.ArcNameW = new wchar_t[filename.Length() + 1];
	wcscpy(OpenArchiveData.ArcNameW, filename.c_str());
#endif
#else // ANSI
	OpenArchiveData.ArcName = new char[filename.Length() + 1];
	strcpy(OpenArchiveData.ArcName, filename.c_str());
#endif
	OpenArchiveData.CmtBuf = NULL;
	OpenArchiveData.OpenMode = RAR_OM_EXTRACT;
	RarFile = RAROpenArchiveEx(&OpenArchiveData);

	if ((RHCode = OpenArchiveData.OpenResult) != 0) {
		throw new ArchiveException(filename, OpenArchiveError(RHCode));
	}

	if (password)
		RARSetPassword(RarFile, password);

	HeaderData.CmtBuf=NULL;

	while ((RHCode = RARReadHeaderEx(RarFile, &HeaderData)) == 0) {
#ifdef wxUSE_UNICODE
		if (page.IsSameAs(wxString(HeaderData.FileNameW))) {
#else // ANSI
		if (page.IsSameAs(HeaderData.FileName)) {
#endif
			length = HeaderData.UnpSize;
			break;	
		} else {
			if ((PFCode = RARProcessFile(RarFile, RAR_SKIP, NULL, NULL)) != 0)
				throw ArchiveException(filename, ProcessFileError(PFCode, page));
		}
	}
	
	if (length == 0) { // archived file not found
		throw new ArchiveException(filename, page + wxT(" not found in this archive."));
	}

	wxUint8 *buffer = new wxUint8[length];
	wxUint8 *callBackBuffer = buffer;

	RARSetCallback(RarFile, CallbackProc, (long) &callBackBuffer);

	PFCode = RARProcessFile(RarFile, RAR_TEST, NULL, NULL);

	if (PFCode != 0) {
		throw new ArchiveException(filename, ProcessFileError(PFCode, page));	
	}

	if (RarFile)  
		RARCloseArchive(RarFile);

#ifdef wxUSE_UNICODE
#ifdef __WXOSX__
	delete[] OpenArchiveData.ArcName;
#else
	delete[] OpenArchiveData.ArcNameW;
#endif
#else
	delete[] OpenArchiveData.ArcName;
#endif
	return new wxMemoryInputStream(buffer, length);
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

bool ComicBookRAR::TestPassword()
{
	return true;
}

int CALLBACK CallbackProc(wxUint32 msg, long UserData, long P1, long P2)
{
	wxUint8 **buffer;
	switch(msg) {

	case UCM_CHANGEVOLUME:
		break;
	case UCM_PROCESSDATA:
		buffer = (wxUint8 **) UserData;
		memcpy(*buffer, (wxUint8 *)P1, P2);
		// advance the buffer ptr, original m_buffer ptr is untouched
		*buffer += P2;
		break;
	case UCM_NEEDPASSWORD:
		break;
	}
	return(0);
}
