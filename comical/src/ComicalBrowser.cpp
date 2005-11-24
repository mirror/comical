/***************************************************************************
              ComicalBrowser.cpp - ComicalBrowser implementation
                             -------------------
    begin                : Tue Nov 22 2005
    copyright            : (C) 2005 James Athey
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

#include "ComicalBrowser.h"
#include "ComicalCanvas.h"

#if wxCHECK_VERSION(2, 5, 1)
ComicalBrowser::ComicalBrowser(wxWindow *prnt, const wxPoint &pos, const wxSize &size) : wxVListBox(prnt, -1, pos, size, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE | wxLB_SINGLE)
#else
ComicalBrowser::ComicalBrowser(wxWindow *prnt, const wxPoint &pos, const wxSize &size) : wxVListBox(prnt, -1, pos, size, wxNO_BORDER | wxLB_SINGLE)
#endif
{
	parent = prnt;
	SetBackgroundColour(* wxWHITE);
	theBook = NULL;
	theCanvas = NULL;
	SetMargins(5, 3);
}

BEGIN_EVENT_TABLE(ComicalBrowser, wxVListBox)
	EVT_LISTBOX(ItemSelected, ComicalBrowser::OnItemSelected)
END_EVENT_TABLE()

wxCoord ComicalBrowser::OnMeasureItem(size_t n) const
{
	return wxCoord(62);
}

void ComicalBrowser::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const
{
	wxBitmap *thumbnail;
	
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	if (theBook) {
		thumbnail = theBook->GetThumbnail(n);
		if (thumbnail) {
			dc.DrawRectangle(((rect.GetWidth() - thumbnail->GetWidth()) / 2) - 1, rect.y, thumbnail->GetWidth() + 2, thumbnail->GetHeight() + 2); 
			dc.DrawBitmap(*thumbnail, (rect.GetWidth() - thumbnail->GetWidth()) / 2, rect.y + 1, false);
			delete thumbnail;
		}
		else
			dc.DrawRectangle(0, rect.y, 102, 62);
	}
}

void ComicalBrowser::OnItemSelected(wxCommandEvent &event)
{
	if (event.IsSelection() && theCanvas)
		theCanvas->GoToPage(event.GetSelection());
}
