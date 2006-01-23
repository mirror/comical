/***************************************************************************
              ComicalCanvas.cpp - ComicalCanvas implementation
                             -------------------
    begin                : Thu Dec 18 2003
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

#include "ComicalCanvas.h"
#include "ComicalFrame.h"
#include "Exceptions.h"

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
	zoom = (COMICAL_ZOOM) config->Read(wxT("Zoom"), 2l); // Fit-to-Width is default
	mode = (COMICAL_MODE) config->Read(wxT("Mode"), 1l); // Double-Page is default
	filter = (FREE_IMAGE_FILTER) config->Read(wxT("Filter"), 4l); // Catmull-Rom is default
	leftPage = NULL;
	rightPage = NULL;
	centerPage = NULL;
	theBook = NULL;
	contextMenu = NULL;
	contextRotate = NULL;
	zoomOn = false;
	zoomEnabled = false;
	
	// Get the thickness of scrollbars.  Knowing this, we can precalculate
	// whether the current page(s) will need scrollbars.
	wxScrollBar *tempBar = new wxScrollBar(this, -1);
	scrollBarThickness = tempBar->GetSize().y;
	tempBar->Destroy();

	parent->Connect(ID_First, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnFirst), NULL, this);
	parent->Connect(ID_Last, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnLast), NULL, this);
	parent->Connect(ID_PrevTurn, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnPrevTurn), NULL, this);
	parent->Connect(ID_NextTurn, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnNextTurn), NULL, this);
	parent->Connect(ID_PrevSlide, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnPrevSlide), NULL, this);
	parent->Connect(ID_NextSlide, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnNextSlide), NULL, this);
	parent->Connect(ID_Unzoomed, ID_FitH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnZoom), NULL, this);
	parent->Connect(ID_Box, ID_Lanczos, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnFilter), NULL, this);
	parent->Connect(ID_CW, ID_West, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnRotate), NULL, this);
	parent->Connect(ID_CWL, ID_WestLeft, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnRotateLeft), NULL, this);
}

BEGIN_EVENT_TABLE(ComicalCanvas, wxScrolledWindow)
	EVT_PAINT(ComicalCanvas::OnPaint)
	EVT_KEY_DOWN(ComicalCanvas::OnKeyDown)
	EVT_LEFT_DOWN(ComicalCanvas::OnLeftDown)
	EVT_LEFT_UP(ComicalCanvas::OnLeftUp)
	EVT_MOTION(ComicalCanvas::OnMouseMove)
	EVT_CONTEXT_MENU(ComicalCanvas::OnRightClick)
	EVT_MENU(ID_ContextOpen, ComicalCanvas::OnOpen)
	EVT_MENU(ID_ContextOpenDir, ComicalCanvas::OnOpenDir)
	EVT_MENU(ID_ContextFirst, ComicalCanvas::OnFirst)
	EVT_MENU(ID_ContextLast, ComicalCanvas::OnLast)
	EVT_MENU(ID_ContextPrevTurn, ComicalCanvas::OnPrevTurn)
	EVT_MENU(ID_ContextNextTurn, ComicalCanvas::OnNextTurn)
	EVT_MENU(ID_ContextPrevSlide, ComicalCanvas::OnPrevSlide)
	EVT_MENU(ID_ContextNextSlide, ComicalCanvas::OnNextSlide)
	EVT_MENU(ID_ContextLeftCW, ComicalCanvas::OnRotateLeft)
	EVT_MENU(ID_ContextLeftCCW, ComicalCanvas::OnRotateLeft)
	EVT_MENU(ID_ContextCW, ComicalCanvas::OnRotate)
	EVT_MENU(ID_ContextCCW, ComicalCanvas::OnRotate)
	EVT_MENU(ID_ContextFull, ComicalCanvas::OnFull)
	EVT_SIZE(ComicalCanvas::OnSize)
END_EVENT_TABLE()

ComicalCanvas::~ComicalCanvas()
{
	clearBitmaps();
	wxConfigBase *config = wxConfigBase::Get();
	config->Write(wxT("Zoom"), zoom);
	config->Write(wxT("Mode"), mode);
	config->Write(wxT("Filter"), filter);

	parent->Disconnect(ID_First, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnFirst), NULL, this);
	parent->Disconnect(ID_Last, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnLast), NULL, this);
	parent->Disconnect(ID_PrevTurn, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnPrevTurn), NULL, this);
	parent->Disconnect(ID_NextTurn, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnNextTurn), NULL, this);
	parent->Disconnect(ID_PrevSlide, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnPrevSlide), NULL, this);
	parent->Disconnect(ID_NextSlide, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnNextSlide), NULL, this);
	parent->Disconnect(ID_Unzoomed, ID_FitH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnZoom), NULL, this);
	parent->Disconnect(ID_Box, ID_Lanczos, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnFilter), NULL, this);
	parent->Disconnect(ID_CW, ID_West, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnRotate), NULL, this);
	parent->Disconnect(ID_CWL, ID_WestLeft, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnRotateLeft), NULL, this);
}

void ComicalCanvas::clearBitmap(wxBitmap *&bitmap)
{
	if (bitmap) {
		delete bitmap;
		bitmap = NULL;
	}
}

void ComicalCanvas::clearBitmaps()
{
	wxMutexLocker lock(paintingMutex);
	// Get the current scroll positions before we clear the bitmaps
	clearBitmap(leftPage);
	clearBitmap(centerPage);
	clearBitmap(rightPage);
}

void ComicalCanvas::createBitmaps()
{
	wxMutexLocker lock(paintingMutex);

	wxInt32 xScroll = 0, yScroll = 0, xWindow, yWindow;
	bool leftOk = false, rightOk = false;
	
	ComicalFrame *cParent = (ComicalFrame *) parent;

	if (mode == ONEPAGE || theBook->GetPageCount() == 1 || leftNum == rightNum) {
		if (mode == ONEPAGE || theBook->GetPageCount() == 1) {
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
				xScroll += leftPage->GetWidth();
				yScroll = (leftPage->GetHeight() > yScroll) ? leftPage->GetHeight() : yScroll;
			}
		}
		cParent->menuView->FindItem(ID_RotateLeft)->Enable(false);
		cParent->menuView->FindItem(ID_RotateRight)->Enable(false);
		cParent->menuView->FindItem(ID_Rotate)->Enable(true);
		cParent->menuRotate->FindItemByPosition(theBook->Orientations[theBook->GetCurrentPage()])->Check();
		cParent->toolBarNav->EnableTool(ID_CCWL, false);
		cParent->toolBarNav->EnableTool(ID_CWL, false);
		cParent->toolBarNav->EnableTool(ID_CCW, true);
		cParent->toolBarNav->EnableTool(ID_CW, true);
		
		cParent->labelLeft->SetLabel(wxT(""));
		cParent->labelRight->SetLabel(wxString::Format(wxT("%d of %d"), theBook->GetCurrentPage() + 1, theBook->GetPageCount()));
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

			cParent->labelRight->SetLabel(wxString::Format(wxT("%d of %d"), rightNum + 1, theBook->GetPageCount()));
		}
		else
		{
			cParent->menuView->FindItem(ID_RotateRight)->Enable(false);
			cParent->toolBarNav->EnableTool(ID_CCW, false);
			cParent->toolBarNav->EnableTool(ID_CW, false);
			cParent->labelRight->SetLabel(wxT(""));
		}
	
		if (leftPage && (leftOk = leftPage->Ok()))
		{
			xScroll += leftPage->GetWidth();
			yScroll = (leftPage->GetHeight() > yScroll) ? leftPage->GetHeight() : yScroll;

			cParent->menuView->FindItem(ID_RotateLeft)->Enable(true);
			cParent->menuRotateLeft->FindItemByPosition(theBook->Orientations[leftNum])->Check();
			cParent->toolBarNav->EnableTool(ID_CCWL, true);
			cParent->toolBarNav->EnableTool(ID_CWL, true);

			cParent->labelLeft->SetLabel(wxString::Format(wxT("%d of %d"), leftNum + 1, theBook->GetPageCount()));
		}
		else
		{
			cParent->menuView->FindItem(ID_RotateLeft)->Enable(false);
			cParent->toolBarNav->EnableTool(ID_CCWL, false);
			cParent->toolBarNav->EnableTool(ID_CWL, false);

			cParent->labelLeft->SetLabel(wxT(""));
		}
	}

	cParent->toolBarNav->Realize();

	GetSize(&xWindow, &yWindow);

	if (xScroll <= xWindow && yScroll <= yWindow) { // no scrollbars neccessary
		SetVirtualSize(xWindow, yWindow);
		SetScrollbars(0, 0, xWindow, yWindow, 0, 0, TRUE);
	} else {
		if (xScroll < (xWindow - scrollBarThickness))
			xScroll = xWindow - scrollBarThickness;
		if (yScroll < (yWindow - scrollBarThickness))
			yScroll = yWindow - scrollBarThickness;
		SetVirtualSize(xScroll, yScroll);
		// if the pages will fit, make sure the scroll bars don't show up by making
		// the scroll step == 0.
		wxInt32 xStep, yStep, xScrollPos;
		if (xScroll > xWindow - scrollBarThickness) {
			xStep = 1;
			xScrollPos = (xScroll / 2) - (xWindow / 2);
		} else {
			xStep = 0;
			xScrollPos = 0;
		}
		if (yScroll > yWindow - scrollBarThickness) {
			yStep = 1;
		} else {
			yStep = 0;
		}
		SetScrollbars(xStep, yStep, xScroll, yScroll, xScrollPos, 0, true);
	}

	Refresh();

}

void ComicalCanvas::FirstPage()
{
	if (theBook == NULL)
		return;

	setPage(0);
	clearBitmaps();

	if (mode == ONEPAGE || theBook->GetPageCount() == 1)
		centerPage = theBook->GetPage(0);
	else {
		if (theBook->IsPageLandscape(0)) {
			leftNum = 0;
			rightNum = 0;
			leftPart = LEFT_HALF;
			rightPart = RIGHT_HALF;
			leftPage = theBook->GetPageLeftHalf(0);
			rightPage = theBook->GetPageRightHalf(0);
		} else {
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
	
	wxUint32 lastNum = theBook->GetCurrentPage();

	if (mode == ONEPAGE || theBook->GetPageCount() == 1)
		centerPage = theBook->GetPage(lastNum);
	else if (theBook->IsPageLandscape(lastNum)) {
		leftNum = lastNum;
		rightNum = lastNum;
		leftPart = LEFT_HALF;
		rightPart = RIGHT_HALF;
		leftPage = theBook->GetPageLeftHalf(lastNum);
		rightPage = theBook->GetPageRightHalf(lastNum);
	} else {
		setPage(theBook->GetPageCount() - 2);
		leftNum = lastNum - 1;
		rightNum = lastNum;
		rightPart = FULL_PAGE;
		rightPage = theBook->GetPage(rightNum);
		if (theBook->IsPageLandscape(leftNum)) {
			leftPart = RIGHT_HALF;
			leftPage = theBook->GetPageRightHalf(lastNum - 1);
		} else {
			leftPart = FULL_PAGE;
			leftPage = theBook->GetPage(lastNum - 1);
		}
	}
	createBitmaps();
}

void ComicalCanvas::GoToPage(wxUint32 pagenumber)
{
	if (theBook == NULL)
		return;
	if (pagenumber >= theBook->GetPageCount())
		throw new PageOutOfRangeException(pagenumber, theBook->GetPageCount());

	if (pagenumber == 0) {
		FirstPage();
		return;
	}
	if (pagenumber == theBook->GetPageCount() - 1) {
		LastPage();
		return;
	}

	setPage(pagenumber);
	clearBitmaps();

	if (mode == ONEPAGE)
		centerPage = theBook->GetPage(pagenumber);
	else {
		leftNum = pagenumber;
		if (theBook->IsPageLandscape(leftNum)) {
			rightNum = leftNum;
			leftPart = LEFT_HALF;
			rightPart = RIGHT_HALF;
			leftPage = theBook->GetPageLeftHalf(pagenumber);
			rightPage = theBook->GetPageRightHalf(pagenumber);
		} else {
			rightNum = leftNum + 1;
			leftPart = FULL_PAGE;
			leftPage = theBook->GetPage(leftNum);
			if (theBook->IsPageLandscape(rightNum)) {
				rightPart = LEFT_HALF;
				rightPage = theBook->GetPageLeftHalf(rightNum);
			} else {
				rightPart = FULL_PAGE;
				rightPage = theBook->GetPage(rightNum);
			}
		}
	}
	createBitmaps();
}

void ComicalCanvas::resetView()
{
	clearBitmaps();

	if (mode == ONEPAGE)
		centerPage = theBook->GetPage(theBook->GetCurrentPage());
	else {
		if (leftPart == LEFT_HALF)
			leftPage = theBook->GetPageLeftHalf(leftNum);
		else if (leftPart == RIGHT_HALF)
			leftPage = theBook->GetPageRightHalf(leftNum);
		else // leftPart == FULL_PAGE
			leftPage = theBook->GetPage(leftNum);
			
		if (rightPart == LEFT_HALF)
			rightPage = theBook->GetPageLeftHalf(rightNum);
		else if (rightPart == RIGHT_HALF)
			rightPage = theBook->GetPageRightHalf(rightNum);
		else // rightPart == FULL_PAGE
			rightPage = theBook->GetPage(rightNum);
	}
	createBitmaps();
}

void ComicalCanvas::PrevPageTurn()
{
	if (theBook == NULL)
		return;
		
	if (mode == ONEPAGE) {
		PrevPageSlide();
		return;
	}
	
	if (leftNum == 0 ||
			(leftNum == 1 && leftPart != RIGHT_HALF)) {
		FirstPage();
		return;
	}

	setPage(leftNum - 1);
	clearBitmaps();

	if (leftPart == RIGHT_HALF) {
		rightPart = LEFT_HALF;
		rightNum = leftNum;
		rightPage = theBook->GetPageLeftHalf(rightNum);
		leftNum--;
		if (theBook->IsPageLandscape(leftNum)) {
			leftPart = RIGHT_HALF;
			leftPage = theBook->GetPageRightHalf(leftNum);
		} else {
			leftPart = FULL_PAGE;
			leftPage = theBook->GetPage(leftNum);
		}
	} else {
		rightNum = leftNum - 1;
		if (theBook->IsPageLandscape(rightNum)) {
			leftNum = rightNum;
			leftPart = LEFT_HALF;
			rightPart = RIGHT_HALF;
			leftPage = theBook->GetPageLeftHalf(leftNum);
			rightPage = theBook->GetPageRightHalf(rightNum);
		} else {
			setPage(leftNum - 2);
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
	}

	createBitmaps();
}

void ComicalCanvas::NextPageTurn()
{
	if (theBook == NULL)
		return;
		
	if (mode == ONEPAGE) {
		NextPageSlide();
		return;
	}
	
	if (rightNum >= theBook->GetPageCount() - 1 ||
			(rightNum == theBook->GetPageCount() - 2 && rightPart != LEFT_HALF)) {
		LastPage();
		return;
	}
	
	clearBitmaps();
	
	if (rightPart == LEFT_HALF) {
		setPage(rightNum);
		leftNum = rightNum;
		leftPart = RIGHT_HALF;
		leftPage = theBook->GetPageRightHalf(leftNum);
		rightNum++;
		if (theBook->IsPageLandscape(rightNum)) {
			rightPart = LEFT_HALF;
			rightPage = theBook->GetPageLeftHalf(rightNum);
		} else {
			rightPart = FULL_PAGE;
			rightPage = theBook->GetPage(rightNum);
		}
	} else {
		setPage(rightNum + 1);
		leftNum = rightNum + 1;
		if (theBook->IsPageLandscape(leftNum)) {
			rightNum = leftNum;
			leftPart = LEFT_HALF;
			rightPart = RIGHT_HALF;
			leftPage = theBook->GetPageLeftHalf(leftNum);
			rightPage = theBook->GetPageRightHalf(rightNum);
		} else {
			rightNum = leftNum + 1;
			leftPart = FULL_PAGE;
			leftPage = theBook->GetPage(leftNum);
			if (theBook->IsPageLandscape(rightNum)) {
				rightPart = LEFT_HALF;
				rightPage = theBook->GetPageLeftHalf(rightNum);
			} else {
				rightPart = FULL_PAGE;
				rightPage = theBook->GetPage(rightNum);
			}
		}
	}

	createBitmaps();
}

void ComicalCanvas::PrevPageSlide()
{
	if (theBook == NULL)
		return;
	if (theBook->GetCurrentPage() == 0) // leftNum == currentPage in this case
		return;
	
	if (mode == ONEPAGE) {
		GoToPage(theBook->GetCurrentPage() - 1);
		return;
	}

	clearBitmaps();

	rightNum = leftNum;
	if (leftPart == RIGHT_HALF) {
		leftPart = LEFT_HALF;
		leftPage = theBook->GetPageLeftHalf(leftNum);
		rightPart = RIGHT_HALF;
		rightPage =theBook->GetPageRightHalf(rightNum);
	} else {
		setPage(leftNum - 1);
		if (leftPart == LEFT_HALF) {
			rightPart = LEFT_HALF;
			rightPage = theBook->GetPageLeftHalf(rightNum);
		} else {
			rightPart = FULL_PAGE;
			rightPage = theBook->GetPage(rightNum);
		}
		leftNum--;
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
	if (theBook->GetCurrentPage() >= theBook->GetPageCount() - 1)
		return;
		
	if (mode == ONEPAGE) {
		GoToPage(theBook->GetCurrentPage() + 1);
		return;
	}

	if (theBook->GetCurrentPage() == theBook->GetPageCount() - 2 && leftPart != LEFT_HALF) {
		LastPage();
		return;
	}

	clearBitmaps();

	leftNum = rightNum;
	if (rightPart == LEFT_HALF) {
		setPage(leftNum);
		leftPart = LEFT_HALF;
		leftPage = theBook->GetPageLeftHalf(leftNum);
		rightPart = RIGHT_HALF;
		rightPage = theBook->GetPageRightHalf(rightNum);
	} else {
		rightNum++;
		if (rightPart == RIGHT_HALF) {
			// no setPage necessary
			leftPart = RIGHT_HALF;
			leftPage = theBook->GetPageRightHalf(leftNum);
		} else {
			setPage(leftNum);
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

void ComicalCanvas::OnZoom(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case ID_Unzoomed:
		zoom = FULL;
		EnableScrolling(true, true);	
		break;
	case ID_3Q:
		zoom = THREEQ;
		EnableScrolling(true, true);	
		break;
	case ID_Half:
		zoom = HALF;
		EnableScrolling(true, true);	
		break;
	case ID_1Q:
		zoom = ONEQ;
		EnableScrolling(true, true);	
		break;
	case ID_Fit:
		zoom = FIT;
		EnableScrolling(false, false); // Fit, no scrolling
		break;
	case ID_FitV:
		zoom = FITHEIGHT;
		EnableScrolling(true, false); // Vertical fit, no vertical scrolling
		break;
	case ID_FitH:
		zoom = FITWIDTH;
		EnableScrolling(false, true); // Horizontal fit, no horizontal scrolling
		break;
	default:
		wxLogError(wxT("Zoom mode %d is undefined."), event.GetId()); // we shouldn't be here... honest!
		break;
	}

	if (theBook) {
		SetParams(true);
	}
}

void ComicalCanvas::OnFilter(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case ID_Box:
		filter = FILTER_BOX;
		break;
	case ID_Bilinear:
		filter = FILTER_BILINEAR;
		break;
	case ID_Bicubic:
		filter = FILTER_BICUBIC;
		break;
	case ID_BSpline:
		filter = FILTER_BSPLINE;
		break;
	case ID_CatmullRom:
		filter = FILTER_CATMULLROM;
		break;
	case ID_Lanczos:
		filter = FILTER_LANCZOS3;
		break;
	default:
		wxLogError(wxT("Filter %d is undefined."), event.GetId()); // we shouldn't be here... honest!
		break;
	}
	
	if (theBook) {
		SetParams(true);
	}
}

void ComicalCanvas::Mode(COMICAL_MODE newMode)
{
	if (theBook) {
		if (mode == ONEPAGE && newMode == TWOPAGE) {
			mode = newMode;
			if (theBook->GetCurrentPage() == 0) {
				SetParams(false);
				FirstPage();
				return;
			} else if (theBook->GetCurrentPage() == theBook->GetPageCount() - 1) {
				SetParams(false);
				LastPage();
				return;
			} else { // theBook->GetCurrentPage() >= 1
				leftNum = theBook->GetCurrentPage();
				if (theBook->IsPageLandscape(leftNum)) {
					leftPart = LEFT_HALF;
					rightPart = RIGHT_HALF;
					rightNum = leftNum;
				} else {
					leftPart = FULL_PAGE;
					rightNum = leftNum + 1;
					if (theBook->IsPageLandscape(rightNum))
						rightPart = LEFT_HALF;
					else
						rightPart = FULL_PAGE;
					rightNum = leftNum + 1;
				}
				SetParams(true);
			}
		} else if (mode == TWOPAGE && newMode == ONEPAGE) {
			mode = newMode;
			SetParams(true);
		} // else nothing has changed !
	} else
		mode = newMode;
}

// boolean to reset view
void ComicalCanvas::SetParams(bool repaint)
{
	wxSize canvasSize = GetSize();
	if (!theBook)
		return;
	if (theBook->SetParams(mode, filter, zoom, canvasSize.x, canvasSize.y, scrollBarThickness) && theBook->IsRunning() && repaint) // if the parameters are actually different
		resetView();
}

void ComicalCanvas::OnRotate(wxCommandEvent& event)
{
	if (!theBook)
		return;

	COMICAL_ROTATE direction = theBook->Orientations[theBook->GetCurrentPage()];

	switch (event.GetId()) {
	case ID_CW:
	case ID_ContextCW:
		switch (direction) {
		case NORTH:
			theBook->RotatePage(theBook->GetCurrentPage(), EAST);
			break;
		case EAST:
			theBook->RotatePage(theBook->GetCurrentPage(), SOUTH);
			break;
		case SOUTH:
			theBook->RotatePage(theBook->GetCurrentPage(), WEST);
			break;
		case WEST:
			theBook->RotatePage(theBook->GetCurrentPage(), NORTH);
			break;
		}
		break;
	case ID_CCW:
	case ID_ContextCCW:
		switch (direction) {
		case NORTH:
			theBook->RotatePage(theBook->GetCurrentPage(), WEST);
			break;
		case EAST:
			theBook->RotatePage(theBook->GetCurrentPage(), NORTH);
			break;
		case SOUTH:
			theBook->RotatePage(theBook->GetCurrentPage(), EAST);
			break;
		case WEST:
			theBook->RotatePage(theBook->GetCurrentPage(), SOUTH);
			break;
		}
		break;
	case ID_North:
		theBook->RotatePage(theBook->GetCurrentPage(), NORTH);
		break;
	case ID_East:
		theBook->RotatePage(theBook->GetCurrentPage(), EAST);
		break;
	case ID_South:
		theBook->RotatePage(theBook->GetCurrentPage(), SOUTH);
		break;
	case ID_West:
		theBook->RotatePage(theBook->GetCurrentPage(), WEST);
		break;
	default:
		wxLogError(wxT("I don't think I can turn that way: %d"), event.GetId()); // we shouldn't be here... honest!
		break;
	}
	GoToPage(theBook->GetCurrentPage());
}

void ComicalCanvas::OnRotateLeft(wxCommandEvent& event)
{
	if (!theBook || theBook->GetCurrentPage() <= 0)
		return;
	
	COMICAL_ROTATE direction = theBook->Orientations[leftNum];
	
	switch (event.GetId()) {
	case ID_CWL:
	case ID_ContextLeftCW:
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
		break;
	case ID_CCWL:
	case ID_ContextLeftCCW:
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
		break;
	case ID_NorthLeft:
		theBook->RotatePage(leftNum, NORTH);
		break;
	case ID_EastLeft:
		theBook->RotatePage(leftNum, EAST);
		break;
	case ID_SouthLeft:
		theBook->RotatePage(leftNum, SOUTH);
		break;
	case ID_WestLeft:
		theBook->RotatePage(leftNum, WEST);
		break;
	default:
		wxLogError(wxT("I don't think I can turn that way: %d"), event.GetId()); // we shouldn't be here... honest!
		break;
	}
	GoToPage(leftNum);
}

void ComicalCanvas::OnPaint(wxPaintEvent &WXUNUSED(event))
{
	wxInt32 xCanvas, yCanvas, textWidth, textHeight, textX, textY;
	wxString text;
	wxPaintDC dc(this);
	PrepareDC(dc);
	dc.SetTextForeground(wxTheColourDatabase->Find(wxT("WHITE")));
	dc.SetTextBackground(GetBackgroundColour());

	GetVirtualSize(&xCanvas, &yCanvas);
	
	if (mode == ONEPAGE) {
		if (centerPage && centerPage->Ok())
			dc.DrawBitmap(*centerPage, xCanvas/2 - centerPage->GetWidth()/2, 0, false);
		else if (theBook) {
			text = wxString::Format(wxT("Page %d loading"), theBook->GetCurrentPage() + 1);
			dc.GetTextExtent(text, &textWidth, &textHeight);
			textX = (xCanvas / 2) - (textWidth / 2);
			if (textX < 0)
				textX = 0;
			textY = (yCanvas / 2) - (textHeight / 2);
			if (textY < 0)
				textY = 0;
			dc.DrawText(text, textX, textY);
		}
	} else { // mode == TWOPAGE
		if (leftPage && leftPage->Ok())
			dc.DrawBitmap(*leftPage, xCanvas/2 - leftPage->GetWidth(), 0, false);
		else if (theBook) {
			text = wxString::Format(wxT("Page %d loading"), leftNum + 1);
			dc.GetTextExtent(text, &textWidth, &textHeight);
			textX = (xCanvas / 4) - (textWidth / 2);
			if (textX < 0)
				textX = 0;
			textY = (yCanvas / 2) - (textHeight / 2);
			if (textY < 0)
				textY = 0;
			dc.DrawText(text, textX, textY);
		}
		
		if (rightPage && rightPage->Ok())
			dc.DrawBitmap(*rightPage, xCanvas/2, 0, false);
		else if (theBook) {
			text = wxString::Format(wxT("Page %d loading"), rightNum + 1);
			dc.GetTextExtent(text, &textWidth, &textHeight);
			textX = ((3 * xCanvas) / 4) - (textWidth / 2);
			if (textX < (xCanvas / 2))
				textX = xCanvas / 2;
			textY = (yCanvas / 2) - (textHeight / 2);
			if (textY < 0)
				textY = 0;
			dc.DrawText(text, textX, textY);
		}
	}
	
	if (zoomOn) {
		wxPoint currMousePos = wxGetMousePosition();
		ScreenToClient(&(currMousePos.x), &(currMousePos.y));
		wxInt32 minX, maxX, minY, maxY, sizeX, sizeY;
		if (currMousePos.x < pointerOrigin.x) {
			minX = currMousePos.x;
			maxX = pointerOrigin.x;
		} else {
			minX = pointerOrigin.x;
			maxX = currMousePos.x;
		}
		if (currMousePos.y < pointerOrigin.y) {
			minY = currMousePos.y;
			maxY = pointerOrigin.y;
		} else {
			minY = pointerOrigin.y;
			maxY = currMousePos.y;
		}
		sizeX = maxX - minX;
		sizeY = maxY - minY;
		wxPen pen = dc.GetPen();
		pen.SetColour(255,255,255);
		pen.SetStyle(wxLONG_DASH);
		dc.SetPen(pen);
		
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		
		dc.DrawRectangle(minX, minY, sizeX, sizeY);
	}

	SetFocus(); // This is so we can grab keydown events

}

void ComicalCanvas::OnKeyDown(wxKeyEvent& event)
{
	int viewX, viewY;
	GetViewStart(&viewX, &viewY);
	
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

	case WXK_LEFT:
		Scroll(viewX - 10, -1);
		break;

	case WXK_RIGHT:
		Scroll(viewX + 10, -1);
		break;

	case WXK_DOWN:
		Scroll(-1, viewY + 10);
		break;

	case WXK_UP:
		Scroll(-1, viewY - 10);
		break;

	default:
		event.Skip();
	}
}

void ComicalCanvas::setPage(wxInt32 pagenumber)
{
	if (theBook) {
		theBook->SetCurrentPage(pagenumber);
		SendCurrentPageChangedEvent();
	}
}

void ComicalCanvas::OnRightClick(wxContextMenuEvent &event)
{
	if (contextMenu)
		delete contextMenu;
	contextMenu = new wxMenu();
	contextMenu->Append(ID_ContextOpen, wxT("Open..."), wxT("Open a comic book."));
	contextMenu->Append(ID_ContextOpenDir, wxT("Open Directory..."), wxT("Open a directory of images."));
	contextMenu->AppendSeparator();
	contextMenu->Append(ID_ContextPrevSlide, wxT("Previous Page"), wxT("Display the previous page."));
	contextMenu->Append(ID_ContextNextSlide, wxT("Next Page"), wxT("Display the next page."));
	contextMenu->AppendSeparator();
	contextMenu->Append(ID_ContextPrevTurn, wxT("&Previous Page Turn"), wxT("Display the previous two pages."));
	contextMenu->Append(ID_ContextNextTurn, wxT("&Next Page Turn"), wxT("Display the next two pages."));
	contextMenu->AppendSeparator();
	contextMenu->Append(ID_ContextFirst, wxT("&First Page"), wxT("Display the first page."));
	contextMenu->Append(ID_ContextLast, wxT("&Last Page"), wxT("Display the last page."));
	contextMenu->AppendSeparator();
	
	contextRotate = new wxMenu();
		
	int viewX, viewY, unitX, unitY;
	GetViewStart(&viewX, &viewY);
	GetScrollPixelsPerUnit(&unitX, &unitY);
	wxSize size = GetVirtualSize();
	wxPoint eventPos = event.GetPosition();
	ScreenToClient(&(eventPos.x), &(eventPos.y));
	
	if (mode == TWOPAGE &&
			leftNum != rightNum &&
			(eventPos.x + (viewX * unitX)) < (size.x / 2)) {
		contextRotate->Append(ID_ContextLeftCCW, wxT("Rotate Counter-Clockwise"), wxT("Rotate 90 degrees counter-clockwise."));
		contextRotate->Append(ID_ContextLeftCW, wxT("Rotate Clockwise"), wxT("Rotate 90 degrees clockwise."));
	} else {
		contextRotate->Append(ID_ContextCCW, wxT("Rotate Counter-Clockwise"), wxT("Rotate 90 degrees counter-clockwise."));
		contextRotate->Append(ID_ContextCW, wxT("Rotate Clockwise"), wxT("Rotate 90 degrees clockwise."));
	}
	contextMenu->Append(ID_ContextRotate, wxT("Rotate"), contextRotate);
	contextMenu->AppendSeparator();
	contextMenu->Append(ID_ContextFull, wxT("Full Screen"));
	
	PopupMenu(contextMenu, eventPos);
}

void ComicalCanvas::SetZoomEnable(bool enabled)
{
	zoomEnabled = enabled;
}

void ComicalCanvas::OnOpen(wxCommandEvent &event)
{
	ComicalFrame *cParent = (ComicalFrame*) parent;
	cParent->OnOpen(event);
}

void ComicalCanvas::OnOpenDir(wxCommandEvent &event)
{
	ComicalFrame *cParent = (ComicalFrame*) parent;
	cParent->OnOpenDir(event);
}

void ComicalCanvas::OnFull(wxCommandEvent &event)
{
	ComicalFrame *cParent = (ComicalFrame*) parent;
	cParent->OnFull(event);
}

void ComicalCanvas::OnLeftDown(wxMouseEvent &event)
{
	SetFocus();
	
	if(!theBook)
		return;
	
	pointerOrigin = event.GetPosition();
	
	if (zoomEnabled) {
		zoomOn = true;
	}
}

void ComicalCanvas::OnLeftUp(wxMouseEvent &event)
{
	if (!theBook)
		return;

	if (zoomEnabled) {
		zoomOn = false;
	
		wxPoint currMousePos = wxGetMousePosition();
		wxInt32 minX, maxX, minY, maxY, sizeX, sizeY;
		if (currMousePos.x < pointerOrigin.x) {
			minX = currMousePos.x;
			maxX = pointerOrigin.x;
		} else {
			minX = pointerOrigin.x;
			maxX = currMousePos.x;
		}
		if (currMousePos.y < pointerOrigin.y) {
			minY = currMousePos.y;
			maxY = pointerOrigin.y;
		} else {
			minY = pointerOrigin.y;
			maxY = currMousePos.y;
		}
		sizeX = maxX - minX;
		sizeY = maxY - minY;
		Refresh();
	}
}

void ComicalCanvas::OnMouseMove(wxMouseEvent &event)
{
	if (zoomOn) {
		Refresh();
	} else if (event.Dragging()) {
		wxPoint currMousePos = event.GetPosition();
		wxInt32 diffX, diffY, viewX, viewY;
		diffX = currMousePos.x - pointerOrigin.x;
		diffY = currMousePos.y - pointerOrigin.y;
		GetViewStart(&viewX, &viewY);
		Scroll(viewX - diffX, viewY - diffY);
		pointerOrigin = currMousePos;
	}
}

void ComicalCanvas::SetComicBook(ComicBook *book)
{
	if (theBook) {
		theBook->Disconnect(ID_PageScaled, EVT_PAGE_SCALED, wxCommandEventHandler(ComicalCanvas::OnPageReady), NULL, this);
	}
	theBook = book;
	if (theBook) {
		theBook->Connect(ID_PageScaled, EVT_PAGE_SCALED, wxCommandEventHandler(ComicalCanvas::OnPageReady), NULL, this);
		leftNum = 0;
		rightNum = 1;
	}
}

void ComicalCanvas::OnPageReady(wxCommandEvent &event)
{
	if (mode == ONEPAGE) {
		if (theBook->GetCurrentPage() == (wxUint32) event.GetInt()) {
			resetView();
		}
	} else { // mode == TWOPAGE
		if (leftNum == (wxUint32) event.GetInt() ||
				rightNum == (wxUint32) event.GetInt()) {
			resetView();
		}
	}
}

void ComicalCanvas::SendCurrentPageChangedEvent()
{
	wxCommandEvent event(EVT_CURRENT_PAGE_CHANGED, ID_CurrentPageChanged);
	event.SetInt(theBook->GetCurrentPage());
	GetEventHandler()->AddPendingEvent(event);
}

void ComicalCanvas::OnSize(wxSizeEvent &event)
{
	SetParams(true);
}

void ComicalCanvas::ClearCanvas()
{
	SetComicBook(NULL);
	clearBitmaps();
	Refresh();
}

