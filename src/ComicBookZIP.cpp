/*
 * ComicBookZIP.cpp
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

#include "ComicBookZIP.h"
#include "wxMiniZipInputStream.h"
#include "Exceptions.h"
#include <errno.h>

wxThread::ExitCode ComicBookZIPOpen::Entry() {
	unzFile ZipFile;	
	static char namebuf[1024];
	unz_file_info64 fileInfo;
	unz_global_info64 globalInfo;
	wxString path;
	wxInputStream *stream;
	ComicPage *page;
	bool passwordSet = false;
	int progress = 0;

	ZipFile = unzOpen64(p->filename.fn_str());
	if (!ZipFile) {
		p->SendCustomEvent(ID_Error, wxT("Could not open the file."));
		goto end;
	}

	unzGetGlobalInfo64(ZipFile, &globalInfo);

	for (int retcode = unzGoToFirstFile(ZipFile); retcode == UNZ_OK; retcode = unzGoToNextFile(ZipFile)) {
		unzGetCurrentFileInfo64(ZipFile, &fileInfo, namebuf, 1024, NULL, 0, NULL, 0);
		path = wxString::FromUTF8(namebuf);

		if (fileInfo.flag&0x01 && p->password.IsEmpty() && !passwordSet) {
			p->SendCustomEvent(ID_SetPassword);
			Pause();
			passwordSet = true;
		}
		
		if (!(fileInfo.flag&0x01 && p->password.IsEmpty())) {
			stream = p->ExtractStream(path);			
			page = new ComicPage(path, stream);
			if (page->GetBitmapType() == wxBITMAP_TYPE_INVALID)
				delete page;
			else
				p->AddPage(page);
			delete stream;
		}
		
		if (TestDestroy()) break;
	}
	
end:
	unzClose(ZipFile);
	p->postCtor();
	p->SendCustomEvent(ID_Opened);
	return (wxThread::ExitCode)0;
}

ComicBookZIP::ComicBookZIP(ComicalFrame *parent, wxString file, wxUint32 cacheLen, COMICAL_ZOOM zoom, long zoomLevel, bool fitOnlyOversize, COMICAL_MODE mode, FREE_IMAGE_FILTER filter, COMICAL_DIRECTION direction):
ComicBook(parent, file, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction)
{
	Open = new ComicBookZIPOpen(this);
	Create(); // create the wxThread
}
	
ComicBookZIP::~ComicBookZIP()
{
	Open->Delete();
};

void ComicBookZIP::ResumeOpen()
{
	Open->Resume();
}

wxInputStream * ComicBookZIP::ExtractStream(wxUint32 pageindex)
{
	return new wxMiniZipInputStream(filename, Pages.at(pageindex)->Filename, password);
}

wxInputStream * ComicBookZIP::ExtractStream(wxString path)
{
	return new wxMiniZipInputStream(filename, path, password);
}

wxString ComicBookZIP::ArchiveError(int error)
{
	wxString prefix = wxT("Could not open ") + filename;
	switch(error) {
		case UNZ_END_OF_LIST_OF_FILE:
			return prefix + wxT(": file not found in ZIP file");
		case UNZ_ERRNO:
			return prefix + wxString::FromAscii(strerror(errno));
		case UNZ_EOF:
			return prefix + wxT(": reached the end of the file");
		case UNZ_PARAMERROR:
			return prefix + wxT(": invalid parameter");
		case UNZ_BADZIPFILE:
			return prefix + wxT(": invalid or corrupted ZIP file");
		case UNZ_INTERNALERROR:
			return prefix + wxT(": internal error");
		case UNZ_CRCERROR:
			return prefix + wxT(": CRC error");
		default:
			return prefix + wxT(": unknown error ") + wxString::Format(wxT("%d"), error);
	}
}

bool ComicBookZIP::TestPassword()
{
	int retcode;
	wxString path;
	static char namebuf[1024];
	unz_file_info64 fileInfo;
	bool retval;

	unzFile ZipFile = unzOpen64(filename.fn_str());
	if (!ZipFile) {
		SendCustomEvent(ID_Error, wxT("Could not open the file."));
		return true;
	}
	
	for (retcode = unzGoToFirstFile(ZipFile); retcode == UNZ_OK; retcode = unzGoToNextFile(ZipFile)) {
		unzGetCurrentFileInfo64(ZipFile, &fileInfo, namebuf, 1024, NULL, 0, NULL, 0);
		path = wxString::FromUTF8(namebuf);
		if (fileInfo.flag&0x01) break;
	};
	if (retcode == UNZ_END_OF_LIST_OF_FILE) return true;

	retcode = unzOpenCurrentFilePassword(ZipFile, password.char_str());
	if (retcode != UNZ_OK) {
		unzClose(ZipFile);
		SendCustomEvent(ID_Error, ArchiveError(retcode));
		return false;
	}
	
	wxUint8 buf[0x100];
	retcode = unzReadCurrentFile(ZipFile, &buf, 0x100);
	unzCloseCurrentFile(ZipFile);
	unzClose(ZipFile);
	return retcode != Z_DATA_ERROR; // if the password is wrong
}
