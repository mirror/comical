/***************************************************************************
              ComicalCanvas.cpp - ComicalCanvas implementation
                             -------------------
    begin                : Thu Dec 18 2003
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

#include "ComicalCanvas.h"
#include "ComicalApp.h"

IMPLEMENT_DYNAMIC_CLASS(ComicalCanvas, wxScrolledWindow)

#if wxCHECK_VERSION(2, 5, 1)
ComicalCanvas::ComicalCanvas(wxWindow *prnt, const wxPoint &pos, const wxSize &size) : wxScrolledWindow(prnt, -1, pos, size, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE)
#else
ComicalCanvas::ComicalCanvas(wxWindow *prnt, const wxPoint &pos, const wxSize &size) : wxScrolledWindow(prnt, -1, pos, size, wxNO_BORDER)
#endif
{
	parent = prnt;
	SetBackgroundColour(* wxBLACK);
	wxConfigBase *config = wxConfigBase::Get();
	// Each of the long values is followed by the letter L not the number one
	zoom = (COMICAL_ZOOM) config->Read("/Comical/Zoom", 2l); // Fit-to-Width is default
	mode = (COMICAL_MODE) config->Read("/Comical/Mode", 1l); // Double-Page is default
	filter = (FREE_IMAGE_FILTER) config->Read("/Comical/Filter", 4l); // Catmull-Rom is default
	leftPage = rightPage = centerPage = NULL;
	theBook = NULL;
}

BEGIN_EVENT_TABLE(ComicalCanvas, wxScrolledWindow)
	EVT_PAINT(ComicalCanvas::OnPaint)
	EVT_KEY_DOWN(ComicalCanvas::OnKeyDown)
	EVT_SIZE(ComicalCanvas::OnSize)
END_EVENT_TABLE()

ComicalCanvas::~ComicalCanvas()
{
	clearBitmaps();
	wxConfigBase *config = wxConfigBase::Get();
	config->Write("/Comical/Zoom", zoom);
	config->Write("/Comical/Mode", mode);
	config->Write("/Comical/Filter", filter);
}

void ComicalCanvas::clearBitmap(wxBitmap *&bitmap)
{
	if (bitmap && bitmap->Ok())
	{
		delete bitmap;
		bitmap = NULL;
	}
}

void ComicalCanvas::clearBitmaps()
{
	clearBitmap(leftPage);
	clearBitmap(centerPage);
	clearBitmap(rightPage);
}

void ComicalCanvas::createBitmaps()
{
	int xScroll = 0, yScroll = 0, xWindow, yWindow;
	bool leftOk = false, rightOk = false;
	
	GetClientSize(&xWindow, &yWindow);

	ComicalFrame *cParent = (ComicalFrame *) parent;

	if (mode == SINGLE || theBook->GetPageCount() == 1 || leftNum == rightNum) {
		if (mode == SINGLE || theBook->GetPageCount() == 1) {
			if (centerPage && centerPage->Ok()) {
				xScroll = centerPage->GetWidth();
				yScroll = centerPage->GetHeight();
			}
		} else {
			if (rightPage && rightPage->Ok()) {
				xScroll = rightPage->GetWidth();
				yScroll = rightPage->GetHeight();
			}
			if (leftPage && leftPage->Ok()) {
				xScroll = (leftPage->GetWidth() > xScroll) ? leftPage->GetWidth() : xScroll;
				yScroll = (leftPage->GetHeight() > yScroll) ? leftPage->GetHeight() : yScroll;
			}
			xScroll *= 2;
		}
		cParent->menuView->FindItem(ID_RotateLeft)->Enable(false);
		cParent->menuView->FindItem(ID_RotateRight)->Enable(false);
		cParent->menuView->FindItem(ID_Rotate)->Enable(true);
		cParent->menuRotate->FindItemByPosition(theBook->Orientations[theBook->Current])->Check();
		cParent->toolBarNav->EnableTool(ID_CCWL, false);
		cParent->toolBarNav->EnableTool(ID_CWL, false);
		cParent->toolBarNav->EnableTool(ID_CCW, true);
		cParent->toolBarNav->EnableTool(ID_CW, true);
	} else {
		cParent->menuView->FindItem(ID_Rotate)->Enable(false);

		if (rightPage && (rightOk = rightPage->Ok()))
		{
			xScroll = rightPage->GetWidth();
			yScroll = rightPage->GetHeight();

			cParent->menuView->FindItem(ID_RotateRight)->Enable(true);
			cParent->menuRotateRight->FindItemByPosition(theBook->Orientations[rightNum])->Check();
			cParent->toolBarNav->EnableTool(ID_CCW, true);
			cParent->toolBarNav->EnableTool(ID_CW, true);
		}
		else
		{
			cParent->menuView->FindItem(ID_RotateRight)->Enable(false);
			cParent->toolBarNav->EnableTool(ID_CCW, false);
			cParent->toolBarNav->EnableTool(ID_CW, false);
		}
	
		if (leftPage && (leftOk = leftPage->Ok()))
		{
			xScroll = (leftPage->GetWidth() > xScroll) ? leftPage->GetWidth() : xScroll;
			yScroll = (leftPage->GetHeight() > yScroll) ? leftPage->GetHeight() : yScroll;

			cParent->menuView->FindItem(ID_RotateLeft)->Enable(true);
			cParent->menuRotateLeft->FindItemByPosition(theBook->Orientations[leftNum])->Check();
			cParent->toolBarNav->EnableTool(ID_CCWL, true);
			cParent->toolBarNav->EnableTool(ID_CWL, true);
		}
		else
		{
			cParent->menuView->FindItem(ID_RotateLeft)->Enable(false);
			cParent->toolBarNav->EnableTool(ID_CCWL, false);
			cParent->toolBarNav->EnableTool(ID_CWL, false);
		}

		xScroll *= 2;
	}

	cParent->toolBarNav->Realize();
	
#if wxCHECK_VERSION(2, 5, 1)
	// Add the remainder before dividing, so we have round-up instead of round-down integer division
	// Keeps those pesky scrollbars from appearing when they're not needed
	xScroll += xScroll % 10;
	yScroll += yScroll % 10;
#endif
	SetScrollbars(10, 10, xScroll / 10, yScroll / 10);
	Scroll((xScroll / 20) - (xWindow / 20), 0);
	Refresh();
}

void ComicalCanvas::FirstPage()
{
	if (theBook == NULL)
		return;

	setPage(0);
	clearBitmaps();

	if (mode == SINGLE || theBook->GetPageCount() == 1)
		centerPage = theBook->GetPage(0);
	else
	{
		if (theBook->IsPageLandscape(0))
		{
			leftNum = 0;
			rightNum = 0;
			leftPart = LEFT_HALF;
			rightPart = RIGHT_HALF;
			leftPage = theBook->GetPageLeftHalf(0);
			rightPage = theBook->GetPageRightHalf(0);
		}
		else
		{
			setPage(1);
			leftNum = 0;
			rightNum = 1;
			leftPart = FULL_PAGE;
			leftPage = theBook->GetPage(0);
			if (theBook->IsPageLandscape(1)) {
				rightPart = LEFT_HALF;
				rightPage = theBook->GetPageLeftHalf(1);
			}
			else {
				rightPart = FULL_PAGE;
				rightPage = theBook->GetPage(1);
			}
		}
	}
	createBitmaps();
}

void ComicalCanvas::LastPage()
{
	if (theBook == NULL)
		return;

	setPage(theBook->GetPageCount() - 1);
	clearBitmaps();

	if (mode == SINGLE || theBook->GetPageCount() == 1)
		centerPage = theBook->GetPage(theBook->Current);
	else if (theBook->IsPageLandscape(theBook->Current)) {
		leftNum = theBook->Current;
		rightNum = theBook->Current;
		leftPart = LEFT_HALF;
		rightPart = RIGHT_HALF;
		leftPage = theBook->GetPageLeftHalf(theBook->Current);
		rightPage = theBook->GetPageRightHalf(theBook->Current);
	} else {
		leftNum = theBook->Current - 1;
		rightNum = theBook->Current;
		rightPart = FULL_PAGE;
		rightPage = theBook->GetPage(rightNum);
		if (theBook->IsPageLandscape(leftNum)) {
			leftPart = RIGHT_HALF;
			leftPage = theBook->GetPageRightHalf(theBook->Current - 1);
		} else {
			leftPart = FULL_PAGE;
			leftPage = theBook->GetPage(theBook->Current - 1);
		}
	}
	createBitmaps();
}

void ComicalCanvas::GoToPage(uint pagenumber)
{
	if (theBook == NULL)
		return;
	if (pagenumber >= theBook->GetPageCount())
		throw new PageOutOfRangeException(pagenumber, theBook->GetPageCount());

	if (pagenumber == 0)
	{
		FirstPage();
		return;
	}
	if (pagenumber == theBook->GetPageCount() - 1)
	{
		LastPage();
		return;
	}

	setPage(pagenumber);
	clearBitmaps();

	if (mode == SINGLE)
		centerPage = theBook->GetPage(pagenumber);
	else
	{
		if (theBook->IsPageLandscape(pagenumber))
		{
			leftNum = pagenumber;
			rightNum = pagenumber;
			leftPart = LEFT_HALF;
			rightPart = RIGHT_HALF;
			leftPage = theBook->GetPageLeftHalf(pagenumber);
			rightPage = theBook->GetPageRightHalf(pagenumber);
		}
		else
		{
			leftNum = pagenumber - 1;
			rightNum = pagenumber;
			if (theBook->IsPageLandscape(pagenumber - 1))
			{
				leftPart = RIGHT_HALF;
				leftPage = theBook->GetPageRightHalf(pagenumber - 1);
			}
			else
			{
				leftPart = FULL_PAGE;
				leftPage = theBook->GetPage(pagenumber - 1);
			}
			rightPart = FULL_PAGE;
			rightPage = theBook->GetPage(pagenumber);
		}
	}
	createBitmaps();
}

void ComicalCanvas::PrevPageTurn()
{
	if (theBook == NULL)
		return;
	if (theBook->Current <= 0)
		return;
	if (theBook->Current == 1) {
		FirstPage();
		return;
	}
	if (mode == SINGLE) {
		PrevPageSlide();
		return;
	}

	if (leftPart != FULL_PAGE) // this covers two different cases
		setPage(theBook->Current - 1);
	else
		setPage(theBook->Current - 2);

	clearBitmaps();

	rightNum = theBook->Current;
	if (theBook->IsPageLandscape(rightNum)) {
		if (leftPart == RIGHT_HALF) { // i.e., if the old left page is the right half of the new current
			rightPart = LEFT_HALF;
			rightPage = theBook->GetPageLeftHalf(rightNum);
			leftNum = theBook->Current - 1;
			if (theBook->IsPageLandscape(leftNum)) {
				leftPart = RIGHT_HALF;
				leftPage = theBook->GetPageRightHalf(leftNum);
			} else {
				leftPart = FULL_PAGE;
				leftPage = theBook->GetPage(leftNum);
			}
		} else {
			leftPart = LEFT_HALF;
			leftNum = theBook->Current;
			leftPage = theBook->GetPageLeftHalf(leftNum);
			rightPart = RIGHT_HALF;
			rightPage = theBook->GetPageRightHalf(rightNum);
		}
	} else {
		rightPart = FULL_PAGE;
		rightPage = theBook->GetPage(rightNum);
		leftNum = rightNum - 1;
		if (theBook->IsPageLandscape(leftNum)) {
			leftPart = RIGHT_HALF;
			leftPage = theBook->GetPageRightHalf(leftNum);
		} else {
			leftPart = FULL_PAGE;
			leftPage = theBook->GetPage(leftNum);
		}
	}
	createBitmaps();
}

void ComicalCanvas::NextPageTurn()
{
	if (theBook == NULL)
		return;
	if (theBook->Current >= theBook->GetPageCount() - 1)
		return;
	if (mode == SINGLE) {
		NextPageSlide();
		return;
	}
	if (theBook->Current == theBook->GetPageCount() - 2) {
		LastPage();
		return;
	}

	if (rightPart == LEFT_HALF || theBook->IsPageLandscape(rightNum + 1))
		setPage(rightNum + 1);
	else
		setPage(rightNum + 2);

	clearBitmaps();

	if (rightPart == LEFT_HALF) { // right page is old left half of current
		leftPart = RIGHT_HALF;
		leftNum = rightNum;
		leftPage = theBook->GetPageRightHalf(leftNum);
		rightNum = theBook->Current;
		if (theBook->IsPageLandscape(rightNum)) {
			rightPart = LEFT_HALF;
			rightPage = theBook->GetPageLeftHalf(rightNum);
		} else {
			rightPart = FULL_PAGE;
			rightPage = theBook->GetPage(rightNum);
		}
	} else if (theBook->Current == rightNum + 2) {
		leftNum = theBook->Current - 1;
		leftPart = FULL_PAGE;
		leftPage = theBook->GetPage(leftNum);
		rightNum = theBook->Current;
		if (theBook->IsPageLandscape(rightNum)) {
			rightPart = LEFT_HALF;
			rightPage = theBook->GetPageLeftHalf(rightNum);
		} else {
			rightPart = FULL_PAGE;
			rightPage = theBook->GetPage(rightNum);
		}
	} else { // theBook->Current == rightNum + 1, IsPageLandscape(rightNum)
		leftNum = theBook->Current;
		leftPart = LEFT_HALF;
		leftPage = theBook->GetPageLeftHalf(leftNum);
		rightNum = theBook->Current;
		rightPart = RIGHT_HALF;
		rightPage = theBook->GetPageRightHalf(rightNum);
	}
	createBitmaps();
}

void ComicalCanvas::PrevPageSlide()
{
	if (theBook == NULL)
		return;
	if (theBook->Current <= 0)
		return;
	if (theBook->Current == 1)
	{
		FirstPage();
		return;
	}
	if (mode == SINGLE) {
		GoToPage(theBook->Current - 1);
		return;
	}

	if (leftNum != rightNum)
		setPage(theBook->Current - 1);

	clearBitmaps();

	rightNum = theBook->Current;
	if (leftPart == RIGHT_HALF) {
		leftNum = theBook->Current;
		leftPart = LEFT_HALF;
		leftPage = theBook->GetPageLeftHalf(leftNum);
		rightPart = RIGHT_HALF;
		rightPage =theBook->GetPageRightHalf(rightNum);
	} else {
		if (leftPart == LEFT_HALF) {
			rightPart = LEFT_HALF;
			rightPage = theBook->GetPageLeftHalf(rightNum);
		} else {
			rightPart = FULL_PAGE;
			rightPage = theBook->GetPage(rightNum);
		}
		leftNum = theBook->Current - 1;
		if (theBook->IsPageLandscape(leftNum)) {
			leftPart = RIGHT_HALF;
			leftPage = theBook->GetPageRightHalf(leftNum);
		} else {
			leftPart = FULL_PAGE;
			leftPage = theBook->GetPage(leftNum);
		}
	}
	createBitmaps();
}

void ComicalCanvas::NextPageSlide()
{
	if (theBook == NULL)
		return;
	if (theBook->Current >= theBook->GetPageCount() - 1)
		return;
	if (theBook->Current == theBook->GetPageCount() - 2 && rightPart != LEFT_HALF) {
		LastPage();
		return;
	}
	if (mode == SINGLE) {
		GoToPage(theBook->Current + 1);
		return;
	}

	if (rightPart != LEFT_HALF)
		setPage(rightNum + 1);

	clearBitmaps();

	rightNum = theBook->Current;
	if (rightPart == LEFT_HALF) {
		leftNum = rightNum;
		leftPart = LEFT_HALF;
		leftPage = theBook->GetPageLeftHalf(leftNum);
		rightPart = RIGHT_HALF;
		rightPage = theBook->GetPageRightHalf(rightNum);
	} else {
		leftNum = rightNum - 1;
		if (rightPart == RIGHT_HALF) {
			leftPart = RIGHT_HALF;
			leftPage = theBook->GetPageRightHalf(leftNum);
		} else {
			leftPart = FULL_PAGE;
			leftPage = theBook->GetPage(leftNum);
		}
		if (theBook->IsPageLandscape(rightNum)) {
			rightPart = LEFT_HALF;
			rightPage = theBook->GetPageLeftHalf(rightNum);
		} else {
			rightPart = FULL_PAGE;
			rightPage = theBook->GetPage(rightNum);
		}
	}
	createBitmaps();
}

void ComicalCanvas::Zoom(COMICAL_ZOOM value)
{
	zoom = value;
	if (zoom == FITH)
		EnableScrolling(false, true); // Horizontal fit, no horizontal scrolling
	else if (zoom == FITV)
		EnableScrolling(true, false); // Vertical fit, no vertical scrolling
	else if (zoom == FIT)
		EnableScrolling(false, false); // Fit, no scrolling
	else
		EnableScrolling(true, true);	
	if (theBook) {
		SetParams();
		GoToPage(theBook->Current);
	}
}

void ComicalCanvas::Filter(FREE_IMAGE_FILTER value)
{
	filter = value;
	if (theBook) {
		SetParams();
		GoToPage(theBook->Current);
	}
}

void ComicalCanvas::Mode(COMICAL_MODE value)
{
	mode = value;
	if (theBook) {
		SetParams();
		GoToPage(theBook->Current);
	}
}

void ComicalCanvas::SetParams()
{
	wxSize clientSize = GetClientSize(); // Client Size is the visible area
	theBook->SetParams(mode, filter, zoom, clientSize.x, clientSize.y);
}

void ComicalCanvas::Rotate(bool clockwise)
{
	COMICAL_ROTATE direction = theBook->Orientations[theBook->Current];
	if (clockwise)
	{
		switch (direction)
		{
		case NORTH:
			theBook->RotatePage(theBook->Current, EAST);
			break;
		case EAST:
			theBook->RotatePage(theBook->Current, SOUTH);
			break;
		case SOUTH:
			theBook->RotatePage(theBook->Current, WEST);
			break;
		case WEST:
			theBook->RotatePage(theBook->Current, NORTH);
			break;
		}
	}
	else
	{
		switch (direction)
		{
		case NORTH:
			theBook->RotatePage(theBook->Current, WEST);
			break;
		case EAST:
			theBook->RotatePage(theBook->Current, NORTH);
			break;
		case SOUTH:
			theBook->RotatePage(theBook->Current, EAST);
			break;
		case WEST:
			theBook->RotatePage(theBook->Current, SOUTH);
			break;
		}
	}
	GoToPage(theBook->Current);
}

void ComicalCanvas::Rotate(COMICAL_ROTATE direction)
{
	if (theBook) {
		theBook->RotatePage(theBook->Current, direction);
		GoToPage(theBook->Current);
	}
}

void ComicalCanvas::RotateLeft(bool clockwise)
{
	if(theBook->Current > 0)
	{
		COMICAL_ROTATE direction = theBook->Orientations[leftNum];
		if (clockwise) {
			switch (direction) {
			case NORTH:
				theBook->RotatePage(leftNum, EAST);
				break;
			case EAST:
				theBook->RotatePage(leftNum, SOUTH);
				break;
			case SOUTH:
				theBook->RotatePage(leftNum, WEST);
				break;
			case WEST:
				theBook->RotatePage(leftNum, NORTH);
				break;
			}
		} else {
			switch (direction) {
			case NORTH:
				theBook->RotatePage(leftNum, WEST);
				break;
			case EAST:
				theBook->RotatePage(leftNum, NORTH);
				break;
			case SOUTH:
				theBook->RotatePage(leftNum, EAST);
				break;
			case WEST:
				theBook->RotatePage(leftNum, SOUTH);
				break;
			}
		}
		GoToPage(leftNum);
	}
}

void ComicalCanvas::RotateLeft(COMICAL_ROTATE direction)
{
	if (theBook) {
		theBook->RotatePage(leftNum, direction);
		GoToPage(leftNum);
	}
}

void ComicalCanvas::OnPaint(wxPaintEvent &WXUNUSED(event))
{
	int xCanvas, yCanvas;

	wxPaintDC dc(this);
	PrepareDC(dc);

	GetVirtualSize(&xCanvas, &yCanvas);

	if (centerPage && centerPage->Ok()) {
		dc.DrawBitmap(*centerPage, (xCanvas/2) - centerPage->GetWidth()/2, 0, false);
	} else {
		if (leftPage && leftPage->Ok())
			dc.DrawBitmap(*leftPage, xCanvas/2 - leftPage->GetWidth(), 0, false);
		if (rightPage && rightPage->Ok())
			dc.DrawBitmap(*rightPage, xCanvas/2, 0, false);
	}

	SetFocus(); // This is so we can grab keydown events

}

void ComicalCanvas::OnKeyDown(wxKeyEvent& event)
{
	switch(event.GetKeyCode()) {

	case WXK_PRIOR:
		PrevPageTurn();
		break;

	case WXK_NEXT:
		NextPageTurn();
		break;

	case WXK_BACK:
		PrevPageSlide();
		break;

	case WXK_SPACE:
		NextPageSlide();
		break;

	case WXK_HOME:
		FirstPage();
		break;

	case WXK_END:
		LastPage();
		break;

	default:
		event.Skip();
	}
}

void ComicalCanvas::OnSize(wxSizeEvent& event)
{
	ComicalFrame *cParent = (ComicalFrame *) parent;
	if (cParent->toolBarNav != NULL && cParent->progress != NULL) {
		wxSize clientSize = GetClientSize();
		wxSize canvasSize = GetSize();
		wxSize toolBarSize = cParent->toolBarNav->GetSize();
		wxSize progressSize = cParent->progress->GetSize();
		int tbxPos = (clientSize.x - toolBarSize.x) / 2;
		cParent->toolBarNav->SetSize(tbxPos, canvasSize.y + progressSize.y, toolBarSize.x, -1);
		cParent->progress->SetSize(0, canvasSize.y, canvasSize.x, 10);
	}
	if (theBook) {
		SetParams();
		GoToPage(theBook->Current);
	}
}

void ComicalCanvas::setPage(int pagenumber)
{
	if (theBook) {
		theBook->Current = pagenumber;
		ComicalFrame *cParent = (ComicalFrame *) parent;
		cParent->progress->SetValue(pagenumber);
	}
}
