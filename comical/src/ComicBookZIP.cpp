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
#include <cstring>
#include <errno.h>

ComicBookZIP::ComicBookZIP(wxString file, wxUint32 cacheLen, COMICAL_ZOOM zoom, long zoomLevel, bool fitOnlyOversize, COMICAL_MODE mode, FREE_IMAGE_FILTER filter, COMICAL_DIRECTION direction, wxInt32 scrollbarThickness) : ComicBook(file, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction, scrollbarThickness)
{
	wxString page;
	static char namebuf[1024];
	unzFile ZipFile;
	unz_file_info *fileInfo;
	int retcode;
	ZipFile = unzOpen(filename.ToAscii());
	fileInfo = (unz_file_info*) malloc(sizeof(unz_file_info_s));

	if (ZipFile) {
		if ((retcode = unzGoToFirstFile(ZipFile)) != UNZ_OK) {
			unzClose(ZipFile);
			throw ArchiveException(filename, ArchiveError(retcode));
		}
	} else {
		throw ArchiveException(filename, wxT("Could not open the file."));
	}

	do {
		unzGetCurrentFileInfo(ZipFile, fileInfo, namebuf, 1024, NULL, 0, NULL, 0);
		page = wxString::FromAscii(namebuf);
		if(page.Right(5).Upper() == wxT(".JPEG") || page.Right(4).Upper() == wxT(".JPG") ||
		page.Right(4).Upper() == wxT(".GIF") ||
		page.Right(4).Upper() == wxT(".PNG"))
			Filenames->Add(page);
	} while (unzGoToNextFile(ZipFile) == UNZ_OK);

	free(fileInfo);
	unzClose(ZipFile);	

	postCtor();
	Create(); // create the wxThread
}

wxInputStream * ComicBookZIP::ExtractStream(wxUint32 pageindex)
{
	unzFile ZipFile;
	unz_file_info *fileInfo;
	wxUint8 *buffer, *inc_buffer;
	int retcode;
	uLong length;
	
	ZipFile = unzOpen(filename.ToAscii());
	fileInfo = (unz_file_info*) malloc(sizeof(unz_file_info_s));

	if (!ZipFile) {
		throw ArchiveException(filename, wxT("Could not open the file."));
	}
	if((retcode = unzLocateFile(ZipFile, Filenames->Item(pageindex).ToAscii(), 0)) != UNZ_OK) {
		unzClose(ZipFile);
		throw ArchiveException(filename, ArchiveError(retcode));
	}
	
	if ((retcode = unzGetCurrentFileInfo(ZipFile, fileInfo, NULL, 0, NULL, 0, NULL, 0)) != UNZ_OK) {
		unzClose(ZipFile);
		throw ArchiveException(filename, ArchiveError(retcode));
	}
	
	length = fileInfo->uncompressed_size;
	buffer = new wxUint8[length];
	inc_buffer = buffer;

	if (password)
		retcode = unzOpenCurrentFilePassword(ZipFile, password);
	else
		retcode = unzOpenCurrentFile(ZipFile);
		
	if (retcode != UNZ_OK) {
		unzClose(ZipFile);
		throw ArchiveException(filename, ArchiveError(retcode));
	}
	
	while((retcode = unzReadCurrentFile(ZipFile, inc_buffer, length)) > 0) {
		inc_buffer += retcode;
		length -= retcode;
	}
	
	if (retcode < 0) {
		unzClose(ZipFile);
		throw ArchiveException(filename, ArchiveError(retcode));
	}
	
	if ((retcode = unzCloseCurrentFile(ZipFile)) != UNZ_OK) {
		unzClose(ZipFile);
		throw ArchiveException(filename, ArchiveError(retcode));
	}
	
	free(fileInfo);
	unzClose(ZipFile);

	return new wxMemoryInputStream(buffer, fileInfo->uncompressed_size);
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
	ZipFile = unzOpen(filename.ToAscii());
	bool retval;
	
	if (!ZipFile) {
		throw ArchiveException(filename, wxT("Could not open the file."));
	}
	if((retcode = unzLocateFile(ZipFile, Filenames->Item(0).ToAscii(), 0)) != UNZ_OK) {
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
