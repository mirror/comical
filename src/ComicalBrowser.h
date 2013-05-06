/*
 * ComicalBrowser.h
 * Copyright (c) 2005-2011 James Athey
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

#ifndef _ComicalBrowser_h_
#define _ComicalBrowser_h_

#include <wx/bitmap.h>
#include <wx/vlbox.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/stream.h>
#include <wx/event.h>
#include <wx/pen.h>
#include <wx/brush.h>
#include <wx/utils.h>

#include "ComicBook.h"
#include "ComicalCanvas.h"
#include "Common.h"

class ComicalBrowser : public wxVListBox
{
public:
	ComicalBrowser(wxWindow *parent, wxInt32 startingWidth);
	~ComicalBrowser() {}

	void SetComicBook(ComicBook *book);
	void SetComicalCanvas(ComicalCanvas *canvas);
	wxInt32 GetThumbnailMaxWidth();
	void UpdateItemCount();
	void ClearBrowser();

	static const unsigned int MARGINS = 5;
	static const unsigned int SPACING = 3;

private:
	virtual void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const;
	virtual wxCoord OnMeasureItem(size_t n) const;
	void OnItemSelected(wxCommandEvent &event);
	void OnCurrentPageChanged(wxCommandEvent &event);
	wxSize GetThumbSize(wxInt32 pagenumber) const;
	void OnThumbnailReady(wxCommandEvent &event);
	void OnSize(wxSizeEvent &event);

	ComicBook *theBook;
	ComicalCanvas *theCanvas;
	wxWindow *parent;

	DECLARE_EVENT_TABLE()
};

#endif
