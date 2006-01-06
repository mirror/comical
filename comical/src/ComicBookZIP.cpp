/***************************************************************************
                              ComicBookZIP.cpp
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

#include "ComicBookZIP.h"
#include <wx/mstream.h>
#include "unzip.h"
#include "Exceptions.h"

ComicBookZIP::ComicBookZIP(wxString file) : ComicBook(file)
{
	wxString page;
	static char namebuf[1024];
	unzFile ZipFile;
	unz_file_info *fileInfo;
	int retcode;
#ifdef wxUSE_UNICODE
	ZipFile = unzOpen(filename.mb_str(wxConvUTF8));
#else // ANSI
	ZipFile = unzOpen(filename.c_str());
#endif
	fileInfo = (unz_file_info*) malloc(sizeof(unz_file_info_s));

	if (ZipFile) {
		if ((retcode = unzGoToFirstFile(ZipFile)) != UNZ_OK) {
			unzClose(ZipFile);
			throw ArchiveException(filename, OpenArchiveError(retcode));
		}
	} else {
		throw ArchiveException(filename, wxT("Could not open the file."));
	}

	do {
		unzGetCurrentFileInfo(ZipFile, fileInfo, namebuf, 1024, NULL, 0, NULL, 0);
		page = wxString::FromAscii(namebuf);
		if(page.Right(5).Upper() == wxT(".JPEG") || page.Right(4).Upper() == wxT(".JPG") ||
		page.Right(5).Upper() == wxT(".TIFF") || page.Right(4).Upper() == wxT(".TIF") ||
		page.Right(4).Upper() == wxT(".GIF") ||
		page.Right(4).Upper() == wxT(".PNG"))
			Filenames->Add(page);
	} while (unzGoToNextFile(ZipFile) == UNZ_OK);

	unzClose(ZipFile);	

	Filenames->Sort();
	Filenames->Shrink();
	pageCount = Filenames->GetCount();
	
	originals = new wxImage[pageCount];
	resamples = new wxImage[pageCount];
	thumbnails = new wxImage[pageCount];
	resampleLockers = new wxMutex[pageCount];
	thumbnailLockers = new wxMutex[pageCount];
	Orientations = new COMICAL_ROTATE[pageCount];
	for (wxUint32 i = 0; i < pageCount; i++)
		Orientations[i] = NORTH;
	
	Create(); // create the wxThread
}

wxInputStream * ComicBookZIP::ExtractStream(wxUint32 pageindex)
{
	unzFile ZipFile;
	unz_file_info *fileInfo;
	wxUint8 *buffer, *inc_buffer;
	int retcode;
	uLong length;
	
#ifdef wxUSE_UNICODE
	ZipFile = unzOpen(filename.mb_str(wxConvUTF8));
#else // ANSI
	ZipFile = unzOpen(filename.c_str());
#endif
	fileInfo = (unz_file_info*) malloc(sizeof(unz_file_info_s));

	if (!ZipFile) {
		throw ArchiveException(filename, wxT("Could not open the file."));
	}
#ifdef wxUSE_UNICODE	
	if((retcode = unzLocateFile(ZipFile, Filenames->Item(pageindex).mb_str(wxConvUTF8), 0)) != UNZ_OK) {
#else
	if((retcode = unzLocateFile(ZipFile, Filenames->Item(pageindex), 0)) != UNZ_OK) {
#endif
		unzClose(ZipFile);
		throw ArchiveException(filename, OpenArchiveError(retcode));
	}
	
	if ((retcode = unzGetCurrentFileInfo(ZipFile, fileInfo, NULL, 0, NULL, 0, NULL, 0)) != UNZ_OK) {
		unzClose(ZipFile);
		throw ArchiveException(filename, OpenArchiveError(retcode));
	}
	
	length = fileInfo->uncompressed_size;
	buffer = (wxUint8*) malloc(length);
	inc_buffer = buffer;

	if (password)
		retcode = unzOpenCurrentFilePassword(ZipFile, password);
	else
		retcode = unzOpenCurrentFile(ZipFile);
		
	if (retcode != UNZ_OK) {
		unzClose(ZipFile);
		throw ArchiveException(filename, OpenArchiveError(retcode));
	}
	
	while((retcode = unzReadCurrentFile(ZipFile, inc_buffer, length)) > 0) {
		inc_buffer += retcode;
		length -= retcode;
	}
	
	if (retcode < 0) {
		unzClose(ZipFile);
		throw ArchiveException(filename, OpenArchiveError(retcode));
	}
	
	if ((retcode = unzCloseCurrentFile(ZipFile)) != UNZ_OK) {
		unzClose(ZipFile);
		throw ArchiveException(filename, OpenArchiveError(retcode));
	}
	
	unzClose(ZipFile);

	return new wxMemoryInputStream(buffer, fileInfo->uncompressed_size);
}

wxString ComicBookZIP::OpenArchiveError(int Error)
{
	wxString prefix = wxT("Could not open ") + filename;
	switch(Error) {
		case UNZ_END_OF_LIST_OF_FILE:
			return wxString(prefix + wxT(": file not found in ZIP file"));
		case UNZ_ERRNO:
			return wxString(prefix);
		case UNZ_EOF:
			return wxString(prefix + wxT(": reached the end of the file"));
		case UNZ_PARAMERROR:
			return wxString(prefix + wxT(": invalid parameter"));
		case UNZ_BADZIPFILE:
			return wxString(prefix + wxT(": invalid or corrupted ZIP file"));
		case UNZ_INTERNALERROR:
			return wxString(prefix + wxT(": internal error"));
		case UNZ_CRCERROR:
			return wxString(prefix + wxT(": CRC error"));
		default:
			return wxString(prefix);
	}
}
