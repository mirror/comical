/*
 * ComicBookZIP.cpp
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

#include "ComicBookZIP.h"
#include "wxMiniZipInputStream.h"
#include "Exceptions.h"
#include <errno.h>

#include <wx/progdlg.h>

ComicBookZIP::ComicBookZIP(wxString file, wxUint32 cacheLen, COMICAL_ZOOM zoom, long zoomLevel, bool fitOnlyOversize, COMICAL_MODE mode, FREE_IMAGE_FILTER filter, COMICAL_DIRECTION direction, wxInt32 scrollbarThickness):
ComicBook(file, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction, scrollbarThickness)
{
	wxString path;
	static char namebuf[1024];
	unz_file_info64 fileInfo;
	unz_global_info64 globalInfo;
	unzFile ZipFile = unzOpen64(filename.fn_str());
	wxInputStream *stream;
	ComicPage *page;
	int progress = 0;

	if (!ZipFile)
		throw ArchiveException(filename, wxT("Could not open the file."));

	unzGetGlobalInfo64(ZipFile, &globalInfo);

	wxProgressDialog progressDlg(wxString(wxT("Opening ")) + file, wxString(), globalInfo.number_entry);
	progressDlg.SetMinSize(wxSize(400, -1));

	for (int retcode = unzGoToFirstFile(ZipFile); retcode == UNZ_OK; retcode = unzGoToNextFile(ZipFile)) {
		unzGetCurrentFileInfo64(ZipFile, &fileInfo, namebuf, 1024, NULL, 0, NULL, 0);
		path = wxString::FromUTF8(namebuf);

		progressDlg.Update(progress, wxString(wxT("Scanning: ")) + path);
		progressDlg.CentreOnParent(wxHORIZONTAL);

		stream = ExtractStream(path);
		page = new ComicPage(path, stream);
		if (page->GetBitmapType() == wxBITMAP_TYPE_INVALID)
			delete page;
		else
			Pages.push_back(page);
		delete stream;

		progressDlg.Update(++progress);
	};

	unzClose(ZipFile);	

	postCtor();
	Create(); // create the wxThread
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
	unzFile ZipFile;
	ZipFile = unzOpen64(filename.fn_str());
	bool retval;
	
	if (!ZipFile) {
		throw ArchiveException(filename, wxT("Could not open the file."));
	}
	if((retcode = unzLocateFile(ZipFile, Pages.at(0)->Filename.ToUTF8(), 0)) != UNZ_OK) {
		unzClose(ZipFile);
		throw ArchiveException(filename, ArchiveError(retcode));
	}
	
	if (password)
		retcode = unzOpenCurrentFilePassword(ZipFile, password);
	else
		retcode = unzOpenCurrentFile(ZipFile);
	if (retcode != UNZ_OK) {
		unzClose(ZipFile);
		throw ArchiveException(filename, ArchiveError(retcode));
	}
	
	char testByte;
	if(unzReadCurrentFile(ZipFile, &testByte, 1) == Z_DATA_ERROR) // if the password is wrong
		retval = false;
	else
		retval = true;
	unzCloseCurrentFile(ZipFile);
	unzClose(ZipFile);	
	return retval;
}
