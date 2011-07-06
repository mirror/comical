/*
 * ComicalApp.cpp
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

#include "ComicalBrowser.h"

ComicalBrowser::ComicalBrowser(wxWindow *prnt, wxInt32 startingWidth):
wxVListBox(prnt, -1, wxDefaultPosition, wxSize(startingWidth,SPACING), wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE | wxLB_SINGLE),
parent(prnt),
theBook(NULL),
theCanvas(NULL),
m_iThumbMaxWidth(startingWidth - (MARGINS * 2))
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
	return wxCoord(m_iThumbMaxWidth * 0.6f);
}

void ComicalBrowser::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const
{
	if (theBook) {
		wxBitmap thumbnail = theBook->GetThumbnail(n);
		wxUint32 xPos = (rect.width / 2) - (thumbnail.GetWidth() / 2) + rect.x;
		if (xPos < 0)
			xPos = 0;
		dc.DrawBitmap(thumbnail, xPos, rect.y, false);
	}
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

void ComicalBrowser::OnCurrentPageChanged(wxCommandEvent &event)
{
	SetSelection(event.GetInt());
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
	SetComicBook(NULL);
	Clear();
	SetItemCount(1);
}


void ComicalBrowser::OnSize(wxSizeEvent &event)
{
	wxSize size = GetClientSize();
	m_iThumbMaxWidth = size.x - (MARGINS * 2);
	if (theBook)
		theBook->SetThumbnailMaxWidth(m_iThumbMaxWidth);
}
