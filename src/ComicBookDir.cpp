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

#include <wx/file.h>
#include <wx/log.h>

ComicBookDir::ComicBookDir(wxString dir, wxUint32 cacheLen, COMICAL_ZOOM zoom, long zoomLevel, bool fitOnlyOversize, COMICAL_MODE mode, FREE_IMAGE_FILTER filter, COMICAL_DIRECTION direction, wxInt32 scrollbarThickness) : ComicBook(dir, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction, scrollbarThickness)
{
	wxArrayString *allFiles = new wxArrayString();
	wxString path;
	size_t count = wxDir::GetAllFiles(dir, allFiles, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
	ComicPage *page;
	wxInputStream *stream;
	
	for (wxUint32 i = 0; i < count; i++) {
		path = allFiles->Item(i);
		page = new ComicPage(path);
		stream = ExtractStream(path);
		if (page->ExtractDimensions(stream))
			Pages->Add(page);
		else
			delete page;
		wxDELETE(stream);
	}
	
	delete allFiles;
	
	postCtor();
	Create(); // create the wxThread
}

wxInputStream * ComicBookDir::ExtractStream(wxUint32 pageindex)
{
	return new wxFileInputStream(Pages->Item(pageindex).Filename);
}

wxInputStream * ComicBookDir::ExtractStream(wxString path)
{
	return new wxFileInputStream(path);
}
