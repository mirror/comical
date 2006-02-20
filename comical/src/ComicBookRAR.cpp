/***************************************************************************
                              ComicBookRAR.cpp
                             -------------------
    begin                : Wed Oct 29 2003
    copyright            : (C) 2003-2006 by James Athey
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

ComicBookRAR::ComicBookRAR(wxString file, wxUint32 cacheLen, COMICAL_ZOOM zoom, long zoomLevel, bool fitOnlyOversize, COMICAL_MODE mode, FREE_IMAGE_FILTER filter, COMICAL_DIRECTION direction, wxInt32 scrollbarThickness) : ComicBook(file, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction, scrollbarThickness)
{
	HANDLE rarFile;
	int RHCode = 0, PFCode = 0;
	struct RARHeaderDataEx header;
	struct RAROpenArchiveDataEx flags;
	wxString page, new_password;
	
	open_rar:
	
	rarFile = openRar(&flags, &header, RAR_OM_LIST);

	if (flags.Flags & 0x0080) { // if the headers are encrypted
		new_password = wxGetPasswordFromUser(
				wxT("This archive is password-protected.  Please enter the password."),
				wxT("Enter Password"));
		if (new_password.IsEmpty()) { // the dialog was cancelled, and the archive cannot be opened
			closeRar(rarFile, &flags);
			throw ArchiveException(filename, wxT("Comical could not open this file because it is password-protected."));
		}
		SetPassword(new_password.ToAscii());
	}
	if (password)
		RARSetPassword(rarFile, password);

	while ((RHCode = RARReadHeaderEx(rarFile, &header)) == 0) {
#ifdef wxUSE_UNICODE
		page = wxString(header.FileNameW);
#else
		page = wxString(header.FileName);
#endif
		if(page.Right(5).Upper() == wxT(".JPEG") || page.Right(4).Upper() == wxT(".JPG") ||
		page.Right(4).Upper() == wxT(".GIF") ||
		page.Right(4).Upper() == wxT(".PNG"))
			Filenames->Add(page);
		
		if ((PFCode = RARProcessFile(rarFile, RAR_SKIP, NULL, NULL)) != 0) {
			closeRar(rarFile, &flags);
			throw ArchiveException(filename, ProcessFileError(PFCode, page));
		}
	}

	closeRar(rarFile, &flags);
	
	// Wrong return code + needs password = wrong password given
	if (RHCode != ERAR_END_ARCHIVE && flags.Flags & 0x0080) 
		goto open_rar;

	postCtor();
	Create(); // create the wxThread
}

wxInputStream * ComicBookRAR::ExtractStream(wxUint32 pageindex)
{
	HANDLE rarFile;
	int RHCode = 0, PFCode = 0;
	struct RARHeaderDataEx header;
	struct RAROpenArchiveDataEx flags;
	
	wxString page = Filenames->Item(pageindex);
	size_t length = 0;
	
	rarFile = openRar(&flags, &header, RAR_OM_EXTRACT);

	if (password)
		RARSetPassword(rarFile, password);

	while ((RHCode = RARReadHeaderEx(rarFile, &header)) == 0) {
#ifdef wxUSE_UNICODE
		if (page.IsSameAs(wxString(header.FileNameW))) {
#else // ASCII
		if (page.IsSameAs(header.FileName)) {
#endif
			length = header.UnpSize;
			break;	
		} else {
			if ((PFCode = RARProcessFile(rarFile, RAR_SKIP, NULL, NULL)) != 0) {
				closeRar(rarFile, &flags);
				throw ArchiveException(filename, ProcessFileError(PFCode, page));
			}
		}
	}
	
	if (length == 0) { // archived file not found
		closeRar(rarFile, &flags);
		throw new ArchiveException(filename, page + wxT(" not found in this archive."));
	}
	if (RHCode) {
		closeRar(rarFile, &flags);
		throw new ArchiveException(filename, OpenArchiveError(PFCode));
	}

	wxUint8 *buffer = new wxUint8[length];
	wxUint8 *callBackBuffer = buffer;

	RARSetCallback(rarFile, CallbackProc, (long) &callBackBuffer);

	PFCode = RARProcessFile(rarFile, RAR_TEST, NULL, NULL);

	closeRar(rarFile, &flags);
	
	if (PFCode != 0)
		throw new ArchiveException(filename, ProcessFileError(PFCode, page));

	return new wxMemoryInputStream(buffer, length);
}

bool ComicBookRAR::TestPassword()
{
	HANDLE rarFile;
	int RHCode = 0, PFCode = 0;
	struct RARHeaderDataEx header;
	struct RAROpenArchiveDataEx flags;
	bool passwordCorrect = true;
	
	wxString page = Filenames->Item(0); // test using the first page
	
	rarFile = openRar(&flags, &header, RAR_OM_EXTRACT);

	if (password)
		RARSetPassword(rarFile, password);

	while ((RHCode = RARReadHeaderEx(rarFile, &header)) == 0) {
#ifdef wxUSE_UNICODE
		if (page.IsSameAs(wxString(header.FileNameW))) {
#else // ASCII
		if (page.IsSameAs(header.FileName)) {
#endif
			break;	
		} else {
			if ((PFCode = RARProcessFile(rarFile, RAR_SKIP, NULL, NULL)) != 0) {
				closeRar(rarFile, &flags);
				throw ArchiveException(filename, ProcessFileError(PFCode, page));
			}
		}
	}
	
	if (RHCode != 0) {
		closeRar(rarFile, &flags);
		throw new ArchiveException(filename, OpenArchiveError(RHCode));
	}

	RARSetCallback(rarFile, TestPasswordCallbackProc, (long) &passwordCorrect);
	PFCode = RARProcessFile(rarFile, RAR_TEST, NULL, NULL);

	closeRar(rarFile, &flags);

	// If the password is wrong, RARProcessFile will return ERAR_BAD_DATA.  Of
	// course, it will also return ERAR_BAD_DATA when the first file in the archive
	// is corrupted.  How does one tell the difference?
	if (PFCode == ERAR_BAD_DATA)
		return false;

	return passwordCorrect;
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

int CALLBACK TestPasswordCallbackProc(wxUint32 msg, long UserData, long P1, long P2)
{
	bool *passwordCorrect;
	switch(msg) {

	case UCM_CHANGEVOLUME:
		break;
	case UCM_PROCESSDATA:
		break;
	case UCM_NEEDPASSWORD:
		passwordCorrect = (bool*) UserData;
		*passwordCorrect = false;
		break;
	}
	return(0);
}

HANDLE ComicBookRAR::openRar(RAROpenArchiveDataEx *flags, RARHeaderDataEx *header, wxUint8 mode)
{
	HANDLE rarFile;

	memset(flags, 0, sizeof(*flags));
#ifdef wxUSE_UNICODE
#ifdef __WXOSX__
	const char *filenameData = filename.fn_str().data();
	flags->ArcName = new char[strlen(filenameData) + 1];
	strcpy(flags->ArcName, filenameData);
#else
	flags->ArcNameW = new wchar_t[filename.Length() + 1];
	wcscpy(flags->ArcNameW, filename.c_str());
#endif
#else // ASCII
	flags->ArcName = new char[filename.Length() + 1];
	strcpy(flags->ArcName, filename.c_str());
#endif
	flags->CmtBuf = NULL;
	flags->OpenMode = mode;

	rarFile = RAROpenArchiveEx(flags);
	if (flags->OpenResult != 0) {
		closeRar(rarFile, flags);
		throw ArchiveException(filename, OpenArchiveError(flags->OpenResult));
	}

	header->CmtBuf = NULL;

	return rarFile;
}

void ComicBookRAR::closeRar(HANDLE rarFile, RAROpenArchiveDataEx *flags)
{
	if (rarFile)
		RARCloseArchive(rarFile);

#if defined(wxUSE_UNICODE) && !defined(__WXOSX__)
	delete[] flags->ArcNameW;
#else
	delete[] flags->ArcName;
#endif
}
