/*
 * ComicalApp.cpp
 * Copyright (c) 2005-2011, James Athey. 2012, John Peterson.
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

#include "ComicalBrowser.h"

ComicalBrowser::ComicalBrowser(wxWindow *prnt, wxInt32 startingWidth):
wxVListBox(prnt, -1, wxDefaultPosition, wxSize(startingWidth,SPACING), wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE | wxLB_SINGLE | wxALWAYS_SHOW_SB),
parent(prnt),
theBook(NULL),
theCanvas(NULL)
{
	SetMargins(MARGINS, SPACING);
	SetBackgroundColour(*wxBLACK);
}

BEGIN_EVENT_TABLE(ComicalBrowser, wxVListBox)
	EVT_LISTBOX(wxID_ANY, ComicalBrowser::OnItemSelected)
	EVT_SIZE(ComicalBrowser::OnSize)
END_EVENT_TABLE()

wxCoord ComicalBrowser::OnMeasureItem(size_t n) const
{
	if (!theBook) return wxCoord(0);
	
	if (theBook->GetPageCount() > n) {
		wxCoord y = theBook->GetThumbnailSize(n).y;
		if (y) return y;
	}	
	
	wxASSERT(theBook->GetPageCount() > n);
}

void ComicalBrowser::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const
{
	if (!theBook) return;
	if (theBook->GetPageCount() <= n) return;
	
	wxBitmap thumbnail = theBook->GetThumbnail(n);
	wxUint32 xPos = (rect.width / 2) - (thumbnail.GetWidth() / 2) + rect.x;
	if (xPos < 0)
		xPos = 0;
	if (thumbnail.IsOk()) dc.DrawBitmap(thumbnail, xPos, rect.y, false);
}

void ComicalBrowser::OnItemSelected(wxCommandEvent &event)
{
	if (theCanvas) {
		theCanvas->GoToPage(event.GetInt());
	}
}

void ComicalBrowser::OnThumbnailReady(wxCommandEvent &event)
{
	Refresh();
}

void ComicalBrowser::UpdateItemCount()
{
	if (!theBook) return;

	SetItemCount(theBook->GetPageCount());
	if (GetSelection() < 0 && GetItemCount()) SetSelection(0);
}

void ComicalBrowser::OnCurrentPageChanged(wxCommandEvent &event)
{
	if (GetItemCount() > event.GetInt()) SetSelection(event.GetInt());
}

void ComicalBrowser::SetComicBook(ComicBook *book)
{
	if (theBook) {
		theBook->Disconnect(EVT_PAGE_THUMBNAILED, wxCommandEventHandler(ComicalBrowser::OnThumbnailReady), NULL, this);
		theBook->Disconnect(EVT_CURRENT_PAGE_CHANGED, wxCommandEventHandler(ComicalBrowser::OnCurrentPageChanged), NULL, this);
	}
	theBook = book;
	if (theBook) {
		theBook->Connect(EVT_PAGE_THUMBNAILED, wxCommandEventHandler(ComicalBrowser::OnThumbnailReady), NULL, this);
		theBook->Connect(EVT_CURRENT_PAGE_CHANGED, wxCommandEventHandler(ComicalBrowser::OnCurrentPageChanged), NULL, this);
	}
}

void ComicalBrowser::SetComicalCanvas(ComicalCanvas *canvas)
{
	theCanvas = canvas;
}

void ComicalBrowser::ClearBrowser()
{
	Clear();
	SetComicBook(NULL);
}

wxInt32 ComicalBrowser::GetThumbnailMaxWidth()
{
	return GetClientSize().x - (MARGINS * 2);
}

void ComicalBrowser::OnSize(wxSizeEvent &event)
{
	if (theBook)
		theBook->SetThumbnailMaxWidth(GetThumbnailMaxWidth());
}
