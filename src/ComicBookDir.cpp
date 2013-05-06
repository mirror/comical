/*
 * ComicBookDir.cpp
 * Copyright (c) 2005-2011, James Athey
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

#include "ComicBookDir.h"

#include <wx/file.h>
#include <wx/log.h>
#include <wx/progdlg.h>

ComicBookDir::ComicBookDir(ComicalFrame *_parent, wxString dir, wxUint32 cacheLen, COMICAL_ZOOM zoom, long zoomLevel, bool fitOnlyOversize, COMICAL_MODE mode, FREE_IMAGE_FILTER filter, COMICAL_DIRECTION direction) : ComicBook(_parent, dir, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction)
{
	wxArrayString allFiles;
	wxString path;
	size_t count = wxDir::GetAllFiles(dir, &allFiles, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
	wxInputStream *stream;
	ComicPage *page;
	
	wxProgressDialog progressDlg(wxString(wxT("Opening ")) + dir, wxString(), count);
	progressDlg.SetMinSize(wxSize(400, -1));

	for (wxUint32 i = 0; i < count; i++) {
		path = allFiles.Item(i);

		progressDlg.Update(i, wxString(wxT("Scanning: ")) + path);
		progressDlg.CentreOnParent(wxHORIZONTAL);

		stream = ExtractStream(path);
		page = new ComicPage(path, stream);
		if (page->GetBitmapType() == wxBITMAP_TYPE_INVALID)
			delete page;
		else
			Pages.push_back(page);

		wxDELETE(stream);

		progressDlg.Update(i + 1);
	}
	
	postCtor();
	Create(); // create the wxThread
}

wxInputStream * ComicBookDir::ExtractStream(wxUint32 pageindex)
{
	return new wxFileInputStream(Pages.at(pageindex)->Filename);
}


wxInputStream * ComicBookDir::ExtractStream(wxString path)
{
	return new wxFileInputStream(path);
}
