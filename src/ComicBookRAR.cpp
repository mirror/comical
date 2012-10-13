/*
 * ComicBookRAR.cpp
 * Copyright (c) 2003-2011, James Athey. 2012, John Peterson.
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
#include "wxRarInputStream.h"
#include "Exceptions.h"

#include <wx/mstream.h>

extern "C" int CALLBACK TestPasswordCallbackProc(wxUint32 msg, long UserData, long P1, long P2);

wxThread::ExitCode ComicBookRAROpen::Entry() {
	HANDLE rarFile;
	int RHCode = 0, PFCode = 0;
	bool passwordSet = false;
	struct RARHeaderDataEx header;
	struct RAROpenArchiveDataEx flags;
	wxString path;
	wxInputStream *stream;
	ComicPage *page;

	rarFile = p->openRar(&flags, &header, RAR_OM_LIST);
	if (!rarFile)
		goto end;

	if (flags.Flags&0x0080) {
		p->SendCustomEvent(ID_SetPassword);
		Pause();
		if (p->password.IsEmpty()) goto end;
	}
	
	if (!p->password.IsEmpty())
		RARSetPassword(rarFile, p->password.char_str());
	
	while ((RHCode = RARReadHeaderEx(rarFile, &header)) == 0) {
		if ((header.Flags&0x00e0) == 0x00e0) {
			if ((PFCode = RARProcessFileW(rarFile, RAR_SKIP, NULL, NULL)) != 0) break;
			continue;
		}
		
		if (header.Flags&0x04 && p->password.IsEmpty() && !passwordSet) {
			p->SendCustomEvent(ID_SetPassword);
			Pause();
			passwordSet = true;
		}
		
		if ((PFCode = RARProcessFileW(rarFile, RAR_SKIP, NULL, NULL)) != 0) break;
				
		if (!(header.Flags&0x04 && p->password.IsEmpty())) {		
			path = header.FileNameW;
			stream = p->ExtractStream(path);
			page = new ComicPage(path, stream);
			if (page->GetBitmapType() == wxBITMAP_TYPE_INVALID) {
				delete page;
			} else {
				// p->Pages.push_back(page);
				p->AddPage(page);
			}
			wxDELETE(stream);
		}
		
		if (TestDestroy()) {
			RHCode = 0;
			break;
		}
	}

	if (RHCode != ERAR_END_ARCHIVE) {
		p->SendCustomEvent(ID_Error, p->OpenArchiveError(RHCode));
	}
	
end:
	p->closeRar(rarFile, &flags);
	p->postCtor();
	p->SendCustomEvent(ID_Opened);
	return (wxThread::ExitCode)0;
}

ComicBookRAR::ComicBookRAR(wxString file, wxUint32 cacheLen, COMICAL_ZOOM zoom, long zoomLevel, bool fitOnlyOversize, COMICAL_MODE mode, FREE_IMAGE_FILTER filter, COMICAL_DIRECTION direction) : ComicBook(file, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction)
{
	Open = new ComicBookRAROpen(this);
	Create(); // create the wxThread
}

ComicBookRAR::~ComicBookRAR()
{
	Open->Delete();
};

void ComicBookRAR::ResumeOpen()
{
	Open->Resume();
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

	rarFile = openRar(&flags, &header, RAR_OM_EXTRACT);

	if (!password.IsEmpty())
		RARSetPassword(rarFile, password.char_str());
		
	while ((RHCode = RARReadHeaderEx(rarFile, &header)) == 0) {
		if (header.Flags&0x04)
			break;
		else {
			if ((PFCode = RARProcessFile(rarFile, RAR_SKIP, NULL, NULL)) != 0) {
				closeRar(rarFile, &flags);
				return false;
			}
		}
	}
	
	if (RHCode == ERAR_END_ARCHIVE) return true;
	else if (RHCode == ERAR_UNKNOWN || RHCode == ERAR_BAD_DATA) return false;
	else if (RHCode != 0) {
		closeRar(rarFile, &flags);
		SendCustomEvent(ID_Error, OpenArchiveError(RHCode));
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
		SendCustomEvent(ID_Error, OpenArchiveError(flags->OpenResult));
		return NULL;
	}

	header->CmtBuf = NULL;

	return rarFile;
}

void ComicBookRAR::closeRar(HANDLE rarFile, RAROpenArchiveDataEx *flags)
{
	if (rarFile)
		RARCloseArchive(rarFile);
}