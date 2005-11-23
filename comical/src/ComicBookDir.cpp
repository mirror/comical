/***************************************************************************
                              ComicBookDir.cpp
                             -------------------
    begin                : Sun Nov 6 2005
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

#include "ComicBookDir.h"

ComicBookDir::ComicBookDir(wxString dir) : ComicBook(dir)
{
	wxArrayString *allFiles = new wxArrayString();
	size_t count = wxDir::GetAllFiles(dir, allFiles, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
	wxString page;
	
	for (wxUint32 i = 0; i < count; i++) {
		page = allFiles->Item(i);
		if(page.Right(5).Upper() == wxT(".JPEG") || page.Right(4).Upper() == wxT(".JPG") ||
				page.Right(5).Upper() == wxT(".TIFF") || page.Right(4).Upper() == wxT(".TIF") ||
				page.Right(4).Upper() == wxT(".GIF") ||
				page.Right(4).Upper() == wxT(".PNG"))
			Filenames->Add(page);
	}
	
	delete allFiles;
	
	Filenames->Sort();
	Filenames->Shrink();
	pageCount = Filenames->GetCount();
	
	originals = new wxImage[pageCount];
	resamples = new wxImage[pageCount];
	imageLockers = new wxMutex[pageCount];
	Orientations = new COMICAL_ROTATE[pageCount]; // NORTH == 0
	for (wxUint32 i = 0; i < pageCount; i++)
		Orientations[i] = NORTH;
	
	Create(); // create the wxThread
}

wxInputStream * ComicBookDir::ExtractStream(wxUint32 pageindex)
{
	return new wxFileInputStream(Filenames->Item(pageindex));
}
