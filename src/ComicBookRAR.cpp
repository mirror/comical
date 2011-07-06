/*
 * ComicBookRAR.cpp
 * Copyright (c) 2003-2011, James Athey
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

#include "ComicBookRAR.h"
#include <wx/mstream.h>
#include <wx/textdlg.h>
#include "Exceptions.h"
#include "wxRarInputStream.h"

#include <wx/progdlg.h>

extern "C" int CALLBACK TestPasswordCallbackProc(wxUint32 msg, long UserData, long P1, long P2);

ComicBookRAR::ComicBookRAR(wxString file, wxUint32 cacheLen, COMICAL_ZOOM zoom, long zoomLevel, bool fitOnlyOversize, COMICAL_MODE mode, FREE_IMAGE_FILTER filter, COMICAL_DIRECTION direction, wxInt32 scrollbarThickness) : ComicBook(file, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction, scrollbarThickness)
{
	HANDLE rarFile;
	int RHCode = 0, PFCode = 0;
	struct RARHeaderDataEx header;
	struct RAROpenArchiveDataEx flags;
	wxString path, new_password;
	wxInputStream *stream;
	ComicPage *page;
	int numEntries, progress=0;
	
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

	// skip through the entire archive to count the number of entries
	for (numEntries = 0; RARReadHeaderEx(rarFile, &header) == 0; numEntries++) {
		if ((PFCode = RARProcessFile(rarFile, RAR_SKIP, NULL, NULL)) != 0) {
			closeRar(rarFile, &flags);
			throw ArchiveException(filename, ProcessFileError(PFCode, header.FileNameW));
		}
	}

	wxProgressDialog progressDlg(wxString(wxT("Opening ")) + file, wxString(), numEntries);
	progressDlg.SetMinSize(wxSize(400, -1));

	// close and re-open the archive to restart at the first header
	closeRar(rarFile, &flags);
	rarFile = openRar(&flags, &header, RAR_OM_LIST);

	while ((RHCode = RARReadHeaderEx(rarFile, &header)) == 0) {
		path = header.FileNameW;

		progressDlg.Update(progress, wxString(wxT("Scanning: ")) + path);
		progressDlg.CentreOnParent(wxHORIZONTAL);

		stream = ExtractStream(path);
		page = new ComicPage(path, stream);
		if (page->GetBitmapType() == wxBITMAP_TYPE_INVALID)
			delete page;
		else
			Pages.push_back(page);

		wxDELETE(stream);

		if ((PFCode = RARProcessFileW(rarFile, RAR_SKIP, NULL, NULL)) != 0) {
			closeRar(rarFile, &flags);
			throw ArchiveException(filename, ProcessFileError(PFCode, path));
		}

		progressDlg.Update(++progress);
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
	return new wxRarInputStream(filename, Pages.at(pageindex)->Filename, password);
}

wxInputStream * ComicBookRAR::ExtractStream(wxString page)
{
	return new wxRarInputStream(filename, page, password);
}

bool ComicBookRAR::TestPassword()
{
	HANDLE rarFile;
	int RHCode = 0, PFCode = 0;
	struct RARHeaderDataEx header;
	struct RAROpenArchiveDataEx flags;
	bool passwordCorrect = true;
	
	if (Pages.size() == 0)
		// nothing in the archive to open
		return true;

	wxString page = Pages.at(0)->Filename; // test using the first page
	
	rarFile = openRar(&flags, &header, RAR_OM_EXTRACT);

	if (password)
		RARSetPassword(rarFile, password);

	while ((RHCode = RARReadHeaderEx(rarFile, &header)) == 0) {
		if (page.Cmp(header.FileNameW) == 0) {
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
	flags->ArcNameW = const_cast<wchar_t*>(filename.wc_str());
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
}
