/***************************************************************************
                              ComicBookZIP.cpp
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

#include "ComicBookZIP.h"

#if wxCHECK_VERSION(2, 5, 0)
#include "wx/wfstream.h"
#else
#include <cstdlib>
#endif

ComicBookZIP::ComicBookZIP(wxString file, uint cachelen) : ComicBook()
{
	wxString page;
	filename = file;
	cacheLen = cachelen;
	Current = 0;

#if wxCHECK_VERSION(2, 5, 0)
	wxFFileInputStream *fs = new wxFFileInputStream(filename);
	if (fs->Ok())
	{
		wxZipInputStream *zipFile = new wxZipInputStream(*fs);
		wxZipEntry *entry;
		while ((entry = zipFile->GetNextEntry()) != NULL)
		{
			page = entry->GetName();
			if(	page.Right(5).Upper() == wxT(".JPEG") || page.Right(4).Upper() == wxT(".JPG") ||
				page.Right(5).Upper() == wxT(".TIFF") || page.Right(4).Upper() == wxT(".TIF") ||
				page.Right(4).Upper() == wxT(".GIF") ||
				page.Right(4).Upper() == wxT(".PNG"))
				Filenames.push_back(page);
		}
		delete zipFile;
	}
	delete fs;
#else
	static char namebuf[1024];
	unzFile ZipFile;
	unz_file_info *fileInfo;
#ifdef wxUSE_UNICODE
	ZipFile = unzOpen(filename.mb_str(wxConvUTF8));
#else // ANSI
	ZipFile = unzOpen(filename.c_str());
#endif
	fileInfo = (unz_file_info*) malloc(sizeof(unz_file_info_s));

	if (ZipFile) {
		if (unzGoToFirstFile(ZipFile) != UNZ_OK) {
			unzClose(ZipFile);
			ZipFile = NULL;
			return;
		}
	} else {
		return;
	}

	do {
		unzGetCurrentFileInfo(ZipFile, fileInfo, namebuf, 1024, NULL, 0, NULL, 0);
		page = wxString::FromAscii(namebuf);
		if(page.Right(5).Upper() == wxT(".JPEG") || page.Right(4).Upper() == wxT(".JPG") ||
		page.Right(5).Upper() == wxT(".TIFF") || page.Right(4).Upper() == wxT(".TIF") ||
		page.Right(4).Upper() == wxT(".GIF") ||
		page.Right(4).Upper() == wxT(".PNG"))
			Filenames.push_back(page);
	} while (unzGoToNextFile(ZipFile) == UNZ_OK);

	unzClose(ZipFile);	
#endif	
	vector<wxString>::iterator begin = Filenames.begin();
	vector<wxString>::iterator end = Filenames.end();
	sort(begin, end);  // I love the STL!

	pageCount = Filenames.size();
	
	originals = new wxImage[pageCount];
	resamples = new wxImage[pageCount];
	Orientations = new COMICAL_ROTATE[pageCount];
	for (uint i = 0; i < pageCount; i++)
		Orientations[i] = NORTH;
	
	Create(); // create the wxThread
}

wxInputStream * ComicBookZIP::ExtractStream(unsigned int pageindex)
{
	return new wxZipInputStream(filename, Filenames[pageindex]);
}
