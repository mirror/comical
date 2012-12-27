/*
 * ComicalCanvas.cpp
 * Copyright (c) 2003-2011, James Athey. 2012, John Peterson.
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

#include "ComicalCanvas.h"
#include "ComicalFrame.h"
#include "Exceptions.h"

#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/stream.h>
#include <wx/log.h>
#include <wx/pen.h>
#include <wx/brush.h>
#include <wx/utils.h>
#include <wx/artprov.h>

#include "firstpage.h"
#include "prevpage.h"
#include "prev.h"
#include "next.h"
#include "nextpage.h"
#include "lastpage.h"
#include "rot_cw.h"
#include "rot_ccw.h"
#include "fullscreen.h"
#include "grab.h"
#include "grabbing.h"

DEFINE_EVENT_TYPE(EVT_PAGE_SHOWN)

ComicalCanvas::ComicalCanvas(ComicalFrame *_parent, COMICAL_MODE _mode, COMICAL_DIRECTION _direction):
wxScrolledWindow(_parent, -1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxFULL_REPAINT_ON_RESIZE),
m_timerWheel(this, ID_WheelTimer),
mode(_mode),
direction(_direction),
parent(_parent)
{
	SetBackgroundColour(* wxBLACK);
	theBook = NULL;
	contextMenu = NULL;
	contextRotate = NULL;
	zoomOn = false;
	zoomEnabled = false;
	scrollBottom = false;
	allowChangePage = false;
    allowWheelChangePage = true;
	
	wxImage img_grab = wxGetBitmapFromMemory(grab).ConvertToImage();
	wxImage img_grabbing = wxGetBitmapFromMemory(grabbing).ConvertToImage();
	img_grab.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 16);
	img_grab.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 16);
	img_grabbing.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 16);
	img_grabbing.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 16);	
	m_grab = wxCursor(img_grab);
	m_grabbing = wxCursor(img_grabbing);
	
	parent->Connect(ID_First, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnFirst), NULL, this);
	parent->Connect(ID_Last, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnLast), NULL, this);
	parent->Connect(ID_PrevTurn, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnPrevTurn), NULL, this);
	parent->Connect(ID_NextTurn, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnNextTurn), NULL, this);
	parent->Connect(ID_PrevSlide, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnPrevSlide), NULL, this);
	parent->Connect(ID_NextSlide, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnNextSlide), NULL, this);
	parent->Connect(ID_CW, ID_West, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnRotate), NULL, this);
	parent->Connect(ID_CWL, ID_WestLeft, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnRotateLeft), NULL, this);
}

BEGIN_EVENT_TABLE(ComicalCanvas, wxScrolledWindow)
	EVT_PAINT(ComicalCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(ComicalCanvas::OnEraseBackground)
	EVT_KEY_DOWN(ComicalCanvas::OnKeyDown)
	EVT_KEY_UP(ComicalCanvas::OnKeyUp)
	EVT_LEFT_DOWN(ComicalCanvas::OnLeftDown)
	EVT_LEFT_UP(ComicalCanvas::OnLeftUp)
	EVT_MOTION(ComicalCanvas::OnMouseMove)
	EVT_LEFT_DCLICK(ComicalCanvas::OnLeftDClick)
	EVT_MOUSEWHEEL(ComicalCanvas::OnMouseWheel)
	EVT_SCROLLWIN(ComicalCanvas::OnScroll)
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
	EVT_SIZE(ComicalCanvas::OnResize)
	EVT_TIMER(ID_WheelTimer, ComicalCanvas::OnWheelTimer)
END_EVENT_TABLE()

ComicalCanvas::~ComicalCanvas()
{
	clearBitmaps();

	parent->Disconnect(ID_First, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnFirst), NULL, this);
	parent->Disconnect(ID_Last, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnLast), NULL, this);
	parent->Disconnect(ID_PrevTurn, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnPrevTurn), NULL, this);
	parent->Disconnect(ID_NextTurn, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnNextTurn), NULL, this);
	parent->Disconnect(ID_PrevSlide, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnPrevSlide), NULL, this);
	parent->Disconnect(ID_NextSlide, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnNextSlide), NULL, this);
	parent->Disconnect(ID_CW, ID_West, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnRotate), NULL, this);
	parent->Disconnect(ID_CWL, ID_WestLeft, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalCanvas::OnRotateLeft), NULL, this);
}

void ComicalCanvas::OnScroll(wxScrollWinEvent &event)
{
	event.Skip();
	UpdatePage();
}

void ComicalCanvas::Scroll(int x, int y)
{
	wxScrolledWindow::Scroll(x, y);
	UpdatePage();
}

void ComicalCanvas::UpdatePage()
{
	switch (mode) {
	case CONTINUOUS:
		{
			int x = GetViewStart().x;
			if (x < 0) x = 0;
			wxSize size = GetSize();	
			
			wxUint32 leftNum_ = theBook->GetPageNum(x), rightNum_ = theBook->GetPageNum(x + size.x);
			
			if (leftNum != leftNum_ || rightNum != rightNum_) {
				leftNum = leftNum_;
				rightNum = rightNum_;
				if (!(parent->zoom == ZOOM_FIT || parent->zoom == ZOOM_WIDTH))
					setPage(leftNum);
				parent->UpdateToolbar();
			}
		}
		break;
	}
}

void ComicalCanvas::clearBitmaps()
{
	wxMutexLocker lock(paintingMutex);
	// Get the current scroll positions before we clear the bitmaps
	leftPage = wxBitmap();
	centerPage = wxBitmap();
	rightPage = wxBitmap();
}

void ComicalCanvas::ResetSize()
{
	if (!theBook) {
		Refresh();
		return;
	}

	wxMutexLocker lock(paintingMutex);

	wxSize virtua, window = GetSize();
	
	switch (mode) {
	case ONEPAGE:
		if (centerPage.Ok()) {
			virtua.x = centerPage.GetWidth();
			virtua.y = centerPage.GetHeight();
		}
		break;
		
	case TWOPAGE:
		if (rightPage.Ok()) {
			virtua.x = rightPage.GetWidth();
			virtua.y = rightPage.GetHeight();
		}
			
		if (leftPage.Ok()) {
			virtua.x += leftPage.GetWidth();
			virtua.y = wxMax(leftPage.GetHeight(), virtua.y);			
		}
		break;
		
	case CONTINUOUS:
		virtua = theBook->GetSize();
		break;
	}

	if (mode != CONTINUOUS || (virtua.x != GetVirtualSize().x || virtua.y != GetVirtualSize().y)) {
		if (virtua.x <= window.x && virtua.y <= window.y) { // no scrolling
			SetVirtualSize(window.x, window.y);
			SetScrollbars(0, 0, window.x, window.y, 0, 0, TRUE);
			scrollBottom = false;
		
		} else {
			if (virtua.x < window.x) virtua.x = window.x;
			if (virtua.y < window.y) virtua.y = window.y;
			SetVirtualSize(virtua.x, virtua.y);
			
			// if the pages will fit, make sure the scroll bars don't show up by making
			// the scroll step == 0.
			wxPoint step;
			wxInt32 xVirtualPos;
			if (virtua.x > window.x) {
				step.x = 1;
				xVirtualPos = (virtua.x / 2) - (window.x / 2);
			} else {
				step.x = 0;
				xVirtualPos = 0;
			}
			if (virtua.y > window.y)
				step.y = 1;
			else
				step.y = 0;
			
			switch (mode) {
			case ONEPAGE:
				SetScrollbars(step.x, step.y, virtua.x, virtua.y, xVirtualPos, scrollBottom ? virtua.y - window.y: 0, true);
					scrollBottom = false;
				break;
				
			case TWOPAGE:
				SetScrollbars(step.x, step.y, virtua.x, virtua.y, xVirtualPos, scrollBottom ? virtua.y - window.y: 0, true);
				if ((leftPart == EMPTY_PAGE || leftPage.Ok()) && (rightPart == EMPTY_PAGE || rightPage.Ok()))
					scrollBottom = false;
				break;
				
			case CONTINUOUS:
				SetScrollbars(step.x, step.y, virtua.x, virtua.y, GetViewStart().x, GetViewStart().y, true);			
				break;
			}		
		}
	}

	Refresh();
	SendPageShownEvent();
}

void ComicalCanvas::FirstPage()
{
	if (!theBook) return;
	if (!theBook->GetPageCount() || IsFirstPage()) return;
		
	setPage(0);
	clearBitmaps();

	switch (mode) {
	case ONEPAGE:
		centerPage = theBook->GetPage(0);
		break;
		
	case TWOPAGE:
		leftNum = NULL;
		rightNum = 0;
		leftPart = EMPTY_PAGE;
		leftPage = wxBitmap();
		if (theBook->IsPageLandscape(leftNum)) {
			rightPart = LEFT_HALF;
			rightPage = theBook->GetPageLeftHalf(rightNum);
		} else {
			rightPart = FULL_PAGE;
			rightPage = theBook->GetPage(rightNum);
		}
		break;
		
	case CONTINUOUS:
		Scroll(0, -1);
		break;
	}
	
	ResetSize();
}

void ComicalCanvas::LastPage()
{
	if (!theBook) return;	
	if (!theBook->GetPageCount() || IsLastPage())
		return;
	
	setPage(theBook->GetPageCount() - 1);
	clearBitmaps();	
	wxUint32 lastNum = theBook->GetCurrentPage();
	
	switch (mode) {
	case ONEPAGE:
		centerPage = theBook->GetPage(lastNum);
		ResetSize();
		break;
		
	case TWOPAGE:	
		if (theBook->GetPageCount() == 1 && !theBook->IsPageLandscape(0)) {
			FirstPage();
			return;
		}
	
		if (theBook->IsPageCountEven()) {
			leftNum = lastNum;
			if (theBook->IsPageLandscape(leftNum)) {
				leftPart = RIGHT_HALF;
				leftPage = theBook->GetPageRightHalf(leftNum);
			} else {
				leftPart = FULL_PAGE;
				leftPage = theBook->GetPage(leftNum);
			}
			rightNum = NULL;
			rightPart = EMPTY_PAGE;
			rightPage = wxBitmap();
		} else {
			if (theBook->IsPageLandscape(lastNum)) {
				leftNum = lastNum;
				rightNum = lastNum;
				leftPart = LEFT_HALF;
				rightPart = RIGHT_HALF;
				leftPage = theBook->GetPageLeftHalf(leftNum);
				rightPage = theBook->GetPageRightHalf(rightNum);
			} else {			
				rightPart = FULL_PAGE;
				leftNum = lastNum - 1;
				rightNum = lastNum;
				if (theBook->IsPageLandscape(leftNum)) {
					leftPart = RIGHT_HALF;
					leftPage = theBook->GetPageRightHalf(leftNum);
				} else {
					leftPart = FULL_PAGE;
					leftPage = theBook->GetPage(leftNum);
				}
				rightPage = theBook->GetPage(rightNum);
			}
		}
		ResetSize();
		break;

	case CONTINUOUS:
		ResetSize();
		Scroll(theBook->GetPagePos(theBook->GetPageCount() - 1), -1);
		break;
	}
}

void ComicalCanvas::GoToPage(wxUint32 pagenumber)
{
	if (!theBook) return;
	
	if (pagenumber >= wxINT32_MAX - 1) pagenumber = 0;
		
	if (pagenumber <= 0) {
		FirstPage();
		return;
	}
	if (theBook->IsReady() && pagenumber >= theBook->GetPageCount() - 1) {
		LastPage();
		return;
	}

	setPage(pagenumber);
	clearBitmaps();

	switch (mode) {
	case ONEPAGE:
		centerPage = theBook->GetPage(pagenumber);
		ResetSize();
		break;
		
	case TWOPAGE:
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
		ResetSize();
		break;
		
	case CONTINUOUS:
		if (!(parent->zoom == ZOOM_FIT || parent->zoom == ZOOM_WIDTH)) {
			ResetSize();
			Scroll(theBook->GetPagePos(pagenumber), -1);
		}
		break;
	}
}

void ComicalCanvas::ResetView()
{
	clearBitmaps();
	
	switch (mode) {
	case ONEPAGE:
		centerPage = theBook->GetPage(theBook->GetCurrentPage());
		ResetSize();
		break;
		
	case TWOPAGE:
		if (leftPart == EMPTY_PAGE) {
			if (theBook->IsPageLandscape(rightNum))
				rightPart = LEFT_HALF;
			else
				rightPart = FULL_PAGE;
		}
	
		if (leftPart != EMPTY_PAGE) {
			if (leftPart == LEFT_HALF) {
				leftPage = theBook->GetPageLeftHalf(leftNum);
			} else if (leftPart == RIGHT_HALF)
				leftPage = theBook->GetPageRightHalf(leftNum);
			else // leftPart == FULL_PAGE
				leftPage = theBook->GetPage(leftNum);
		}
			
		if (rightPart != EMPTY_PAGE) {
			if (rightPart == LEFT_HALF)
				rightPage = theBook->GetPageLeftHalf(rightNum);
			else if (rightPart == RIGHT_HALF)
				rightPage = theBook->GetPageRightHalf(rightNum);
			else // rightPart == FULL_PAGE
				rightPage = theBook->GetPage(rightNum);
		}
		ResetSize();
		break;
	
	case CONTINUOUS:
		ResetSize();
		GoToPage(theBook->GetCurrentPage());
		break;
	}
	UpdatePage();
	ShowCursor();
}

void ComicalCanvas::PrevPageTurn()
{
	if (!theBook) return;
	if (IsFirstPage()) return;
	
	switch (mode) {
	case ONEPAGE:
		PrevPageSlide();
		return;
	
	case TWOPAGE:
		if (theBook->GetCurrentPage() <= 1) {
			FirstPage();
			return;
		}
		clearBitmaps();
	
		setPage(leftNum - 1);
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
		ResetSize();
		break;

	case CONTINUOUS:	
		GoToPage(theBook->GetCurrentPage() - 2);
		break;
	}
}

void ComicalCanvas::NextPageTurn()
{
	if (!theBook) return;
	if (IsLastPage()) return;
	
	switch (mode) {
	case ONEPAGE:
		NextPageSlide();
		return;
	
	case TWOPAGE:
		if (rightPart == EMPTY_PAGE || rightNum >= theBook->GetPageCount() - 2) {
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
		ResetSize();
		break;
	
	case CONTINUOUS:
		GoToPage(theBook->GetCurrentPage() + 2);
		break;
	}
}

void ComicalCanvas::PrevPageSlide()
{
	if (!theBook) return;	
	if (IsFirstPage()) return;

	switch (mode) {
	case ONEPAGE:
		GoToPage(theBook->GetCurrentPage() - 1);
		return;

	case TWOPAGE:
		if (!theBook->GetCurrentPage() && leftPart != EMPTY_PAGE) {
			FirstPage();
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
		ResetSize();
		break;
	
	case CONTINUOUS:
		GoToPage(theBook->GetCurrentPage() - 1);
		break;
	}
}

void ComicalCanvas::NextPageSlide()
{
	if (!theBook) return;	
	if (IsLastPage()) return;

	switch (mode) {
	case ONEPAGE:
		GoToPage(theBook->GetCurrentPage() + 1);
		return;
	
	case TWOPAGE:
		clearBitmaps();
		leftNum = rightNum;
		if (rightPart == LEFT_HALF) {
			setPage(leftNum);
			leftPart = LEFT_HALF;
			leftPage = theBook->GetPageLeftHalf(leftNum);
			rightPart = RIGHT_HALF;
			rightPage = theBook->GetPageRightHalf(rightNum);
		} else {
			if (rightPart == RIGHT_HALF) {
				// no setPage necessary
				leftPart = RIGHT_HALF;
				leftPage = theBook->GetPageRightHalf(leftNum);
			} else {
				setPage(leftNum);
				leftPart = FULL_PAGE;
				leftPage = theBook->GetPage(leftNum);
			}
			if (rightNum >= theBook->GetPageCount() - 1) {
				rightNum = NULL;
				rightPart = EMPTY_PAGE;
				rightPage = wxBitmap();
			} else {
				rightNum++;
				if (theBook->IsPageLandscape(rightNum)) {
					rightPart = LEFT_HALF;
					rightPage = theBook->GetPageLeftHalf(rightNum);
				} else {
					rightPart = FULL_PAGE;
					rightPage = theBook->GetPage(rightNum);
				}
			}
		}
		ResetSize();
		break;
		
	case CONTINUOUS:
		GoToPage(theBook->GetCurrentPage() + 1);
		break;
	}
}

void ComicalCanvas::SetMode(COMICAL_MODE newMode)
{
	if (!theBook) {
		mode = newMode;
		return;
	}
	if (mode == newMode)
		return;
	
	switch (newMode) {			
	case ONEPAGE:
		mode = newMode;
		ResetView();
		break;

	case TWOPAGE:
		mode = newMode;
		if (theBook->GetCurrentPage() == 0) {
			FirstPage();
			return;
		} else if (theBook->GetCurrentPage() == theBook->GetPageCount() - 1) {
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
			ResetView();
		}
		break;
					
	case CONTINUOUS:
		mode = newMode;
		ResetSize();
		GoToPage(theBook->GetCurrentPage());
		break;
	}
}

void ComicalCanvas::OnRotate(wxCommandEvent& event)
{
	if (!theBook) return;

	wxUint32 pagenumber;
	if (mode == ONEPAGE)
		pagenumber = theBook->GetCurrentPage();
	else
		pagenumber = rightNum;
	COMICAL_ROTATE direction = theBook->GetPageOrientation(pagenumber);

	switch (event.GetId()) {
	case ID_CW:
	case ID_ContextCW:
		switch (direction) {
		case NORTH:
			theBook->RotatePage(pagenumber, EAST);
			break;
		case EAST:
			theBook->RotatePage(pagenumber, SOUTH);
			break;
		case SOUTH:
			theBook->RotatePage(pagenumber, WEST);
			break;
		case WEST:
			theBook->RotatePage(pagenumber, NORTH);
			break;
		}
		break;
	case ID_CCW:
	case ID_ContextCCW:
		switch (direction) {
		case NORTH:
			theBook->RotatePage(pagenumber, WEST);
			break;
		case EAST:
			theBook->RotatePage(pagenumber, NORTH);
			break;
		case SOUTH:
			theBook->RotatePage(pagenumber, EAST);
			break;
		case WEST:
			theBook->RotatePage(pagenumber, SOUTH);
			break;
		}
		break;
	case ID_North:
		theBook->RotatePage(pagenumber, NORTH);
		break;
	case ID_East:
		theBook->RotatePage(pagenumber, EAST);
		break;
	case ID_South:
		theBook->RotatePage(pagenumber, SOUTH);
		break;
	case ID_West:
		theBook->RotatePage(pagenumber, WEST);
		break;
	default:
		wxLogError(wxT("I don't think I can turn that way: %d"), event.GetId()); // we shouldn't be here... honest!
		break;
	}
	GoToPage(pagenumber);
}

void ComicalCanvas::OnRotateLeft(wxCommandEvent& event)
{
	if (!theBook) return;
	
	COMICAL_ROTATE direction = theBook->GetPageOrientation(leftNum);
	
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
	if (!theBook) return;
		
	wxInt32 xPosLeft, xPosRight;
	wxRect textarea;
	wxString text;
	wxBufferedPaintDC dc(this);
	PrepareDC(dc);
	wxSize canvas = GetVirtualSize();
	
	// text
	dc.SetTextForeground(wxColour(wxT("#999999")));
	dc.SetTextBackground(GetBackgroundColour());
	wxFont font(12, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL); 
	dc.SetFont(font);
	
	// clear
	dc.SetBrush(*wxBLACK_BRUSH);
	dc.DrawRectangle(wxRect(wxPoint(0, 0), GetVirtualSize()));
	
	switch (mode) {
	case ONEPAGE:
		if (centerPage.Ok())
			dc.DrawBitmap(centerPage, canvas.x/2 - centerPage.GetWidth()/2, canvas.y/2 - centerPage.GetHeight()/2, false);
		else {
			text = wxString::Format(wxT("Page %d loading..."), theBook->GetCurrentPage() + 1);
			dc.GetTextExtent(text, &textarea.width, &textarea.height);
			textarea.x = (canvas.x / 2) - (textarea.width / 2);
			if (textarea.x < 0)
				textarea.x = 0;
			textarea.y = (canvas.y / 2) - (textarea.height / 2);
			if (textarea.y < 0)
				textarea.y = 0;
			dc.DrawText(text, textarea.x, textarea.y);
		}
		break;
		
	case TWOPAGE:
		{
			wxBitmap trueLeftPage, trueRightPage;
			wxUint32 trueLeftNum, trueRightNum;
			if (direction == COMICAL_LTR) {
				trueLeftPage = leftPage;
				trueLeftNum = leftNum;
				trueRightPage = rightPage;
				trueRightNum = rightNum;
			} else { // direction == COMICAL_RTL
				trueLeftPage = rightPage;
				trueLeftNum = rightNum;
				trueRightPage = leftPage;
				trueRightNum = leftNum;
			}

			// x position
			xPosLeft = canvas.x/2 - trueLeftPage.GetWidth();
			if (xPosLeft < 0) xPosLeft = 0;
				
			if (leftPart != EMPTY_PAGE)
				xPosRight = xPosLeft + trueLeftPage.GetWidth();
			else
				xPosRight = canvas.x/2;
			if (xPosRight + trueRightPage.GetWidth() > canvas.x) xPosRight = canvas.x - trueRightPage.GetWidth();
					
			if (trueLeftPage.Ok()) {
				dc.DrawBitmap(trueLeftPage, xPosLeft, (canvas.y / 2) - (trueLeftPage.GetHeight() / 2), false);
			} else if (leftPart != EMPTY_PAGE) {
				text = wxString::Format(wxT("Page %d loading..."), trueLeftNum + 1);
				dc.GetTextExtent(text, &textarea.width, &textarea.height);
				textarea.x = (canvas.x / 4) - (textarea.width / 2);
				if (textarea.x < 0)
					textarea.x = 0;
				textarea.y = (canvas.y / 2) - (textarea.height / 2);
				if (textarea.y < 0)
					textarea.y = 0;
				dc.DrawText(text, textarea.x, textarea.y);
			}
			
			if (trueRightPage.Ok()) {
				dc.DrawBitmap(trueRightPage, xPosRight, (canvas.y / 2) - (trueRightPage.GetHeight() / 2), false);
			} else if (rightPart != EMPTY_PAGE) {
				text = wxString::Format(wxT("Page %d loading..."), trueRightNum + 1);
				dc.GetTextExtent(text, &textarea.width, &textarea.height);
				textarea.x = ((3 * canvas.x) / 4) - (textarea.width / 2);
				if (textarea.x < (canvas.x / 2))
					textarea.x = canvas.x / 2;
				textarea.y = (canvas.y / 2) - (textarea.height / 2);
				if (textarea.y < 0)
					textarea.y = 0;
				dc.DrawText(text, textarea.x, textarea.y);
			}
		}
		break;
		
	case CONTINUOUS:
		for (wxUint32 i = 0; i < theBook->Pages.size(); i++) {
			wxBitmap img = theBook->Pages.at(i)->GetPage();
			wxUint32 pos = theBook->GetPagePos(i);
			wxSize size = theBook->GetPageSize(i);
			if (img.Ok())
				dc.DrawBitmap(img, pos, (canvas.y / 2) - (size.y / 2), false);
			else {
				text = wxString::Format(wxT("Page %d loading..."), i + 1);
				dc.GetTextExtent(text, &textarea.width, &textarea.height);
				textarea.x = pos + (size.x / 2) - (textarea.width / 2);
				if (textarea.x < 0) textarea.x = 0;
				textarea.y = (canvas.y / 2) - (textarea.height / 2);
				if (textarea.y < 0) textarea.y = 0;
				dc.DrawText(text, textarea.x, textarea.y);
			}
		}
		break;
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

	SetFocus(); // So we can grab keydown events
}

void ComicalCanvas::OnEraseBackground(wxEraseEvent &event)
{
	if (!theBook) {
		event.Skip();
		return;
	}
	if (!theBook->GetPageCount())
		event.Skip();
}

void ComicalCanvas::OnKeyDown(wxKeyEvent& event)
{	
	switch(event.GetKeyCode()) {

	case WXK_PAGEUP:
		PrevPageTurn();
		break;

	case WXK_PAGEDOWN:
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
		ScrollLeft();
		break;

	case WXK_RIGHT:
		ScrollRight();
		break;

	case WXK_UP:
		if (IsScrolling(wxVERTICAL) && !IsScrollTop())
			ScrollUp();
		else if (!IsScrolling(wxVERTICAL) || allowChangePage) {
			scrollBottom = true;
			if (event.GetModifiers() == wxMOD_CONTROL) PrevPageSlide(); else PrevPageTurn();
		}
		allowChangePage = false;
		break;
		
	case WXK_DOWN:
		if (IsScrolling(wxVERTICAL) && !IsScrollBottom())
			ScrollDown();
		else if (!IsScrolling(wxVERTICAL) || allowChangePage) {
			if (event.GetModifiers() == wxMOD_CONTROL) NextPageSlide();	else NextPageTurn();
		}
		allowChangePage = false;
		break;
		
	case WXK_RETURN:
		parent->Full();
		break;
		
	case WXK_ESCAPE:
		parent->Quit();
		break;

	default:
		event.Skip();
	}
}

void ComicalCanvas::OnKeyUp(wxKeyEvent& event)
{
	switch(event.GetKeyCode()) {
		case WXK_UP:
		case WXK_DOWN:
			allowChangePage = true;
			break;
	}
}

void ComicalCanvas::setPage(wxInt32 pagenumber)
{
	if (theBook) {
		theBook->SetCurrentPage(pagenumber);
	}
}

void ComicalCanvas::OnLeftDClick(wxMouseEvent &event)
{
	bool isLeft, isTop, isBottom;
	wxPoint unit; GetScrollPixelsPerUnit(&unit.x, &unit.y);
	
	switch (mode) {
	case ONEPAGE:
		isLeft = event.GetPosition().x < GetSize().x / 2;
		isTop = event.GetPosition().y < GetSize().y / 2;
		break;
	
	case TWOPAGE:
		isLeft = (event.GetPosition().x + (GetViewStart().x * unit.x)) < (GetVirtualSize().x / 2);
		isTop = (event.GetPosition().y + (GetViewStart().y * unit.y)) < (GetVirtualSize().y / 2);
		break;		
	
	case CONTINUOUS:
		isLeft = event.GetPosition().x < GetSize().x / 2;
		isTop = event.GetPosition().y < 20;
		isBottom = event.GetPosition().y > GetSize().y - 20;
		break;
	}
	
	switch (mode) {
	case ONEPAGE:
		if (isTop)
			PrevPageSlide();			
		else			
			NextPageSlide();
		break;
	
	case TWOPAGE:
		if (isLeft) {
			if (direction == COMICAL_LTR) PrevPageTurn(); else NextPageTurn();	
		} else {			
			if (direction == COMICAL_LTR) NextPageTurn(); else PrevPageTurn();	
		}
		break;
		
	case CONTINUOUS:
		if (IsScrolling(wxVERTICAL) && (isTop || isBottom)) {
			if (isTop)
				Scroll(-1, 0);
			else
				Scroll(-1, GetVirtualSize().y);
		} else {
			if (isLeft)
				GoToPage(theBook->GetCurrentPage() - 1);
			else
				GoToPage(theBook->GetCurrentPage() + 1);
			break;
		}
	}
}

void ComicalCanvas::OnRightClick(wxContextMenuEvent &event)
{
	contextMenu = new wxMenu();

	wxMenuItem *openMenu = new wxMenuItem(contextMenu, ID_ContextOpen, wxT("Open..."), wxT("Open a comic book."));
	wxMenuItem *prevMenu = new wxMenuItem(contextMenu, ID_ContextPrevSlide, wxT("Previous Page"), wxT("Display the previous page."));
	wxMenuItem *nextMenu = new wxMenuItem(contextMenu, ID_ContextNextSlide, wxT("Next Page"), wxT("Display the next page."));
	wxMenuItem *prevTurnMenu = new wxMenuItem(contextMenu, ID_ContextPrevTurn, wxT("&Previous Page Turn"), wxT("Display the previous two pages."));
	wxMenuItem *nextTurnMenu = new wxMenuItem(contextMenu, ID_ContextNextTurn, wxT("&Next Page Turn"), wxT("Display the next two pages."));
	wxMenuItem *firstMenu = new wxMenuItem(contextMenu, ID_ContextFirst, wxT("&First Page"), wxT("Display the first page."));
	wxMenuItem *lastMenu = new wxMenuItem(contextMenu, ID_ContextLast, wxT("&Last Page"), wxT("Display the last page."));

	wxBitmap openIcon = wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_MENU, wxDefaultSize);
	openMenu->SetBitmap(openIcon);
	prevMenu->SetBitmap(wxGetBitmapFromMemory(prev));
	nextMenu->SetBitmap(wxGetBitmapFromMemory(next));
	prevTurnMenu->SetBitmap(wxGetBitmapFromMemory(prevpage));
	nextTurnMenu->SetBitmap(wxGetBitmapFromMemory(nextpage));
	firstMenu->SetBitmap(wxGetBitmapFromMemory(firstpage));
	lastMenu->SetBitmap(wxGetBitmapFromMemory(lastpage));

	contextMenu->Append(openMenu);
	contextMenu->Append(ID_ContextOpenDir, wxT("Open Directory..."), wxT("Open a directory of images."));
	contextMenu->AppendSeparator();
	contextMenu->Append(prevMenu);
	contextMenu->Append(nextMenu);
	contextMenu->AppendSeparator();
	contextMenu->Append(prevTurnMenu);
	contextMenu->Append(nextTurnMenu);
	contextMenu->AppendSeparator();
	contextMenu->Append(firstMenu);
	contextMenu->Append(lastMenu);
	
	contextMenu->AppendSeparator();
	wxMenu *contextZoom = new wxMenu();	
	contextZoom->AppendRadioItem(ID_Fit, wxT("F&it"), wxT("Scale pages to fit the window."));
	contextZoom->AppendRadioItem(ID_FitV, wxT("Fit to Height"), wxT("Scale pages to fit the window's height."));
	contextZoom->AppendRadioItem(ID_FitH, wxT("Fit to Width"), wxT("Scale pages to fit the window's width."));
	contextZoom->AppendRadioItem(ID_Unzoomed, wxT("O&riginal Size"), wxT("Show pages without resizing them."));
	contextZoom->AppendRadioItem(ID_Custom, wxT("Custom Zoom"), wxT("Scale pages to a custom percentage of their original size."));
	contextZoom->AppendSeparator();
	contextZoom->Append(ID_SetCustom, wxT("Set Custom Zoom Level..."), wxT("Choose the percentage that the Custom Zoom mode will use."));	
	contextMenu->Append(wxID_ANY, wxT("&Zoom"), contextZoom);
	
	contextMenu->AppendSeparator();	
	contextRotate = new wxMenu();		
	int viewX, viewY, unitX, unitY;
	GetViewStart(&viewX, &viewY);
	GetScrollPixelsPerUnit(&unitX, &unitY);
	wxSize size = GetVirtualSize();
	wxPoint eventPos = event.GetPosition();
	ScreenToClient(&(eventPos.x), &(eventPos.y));	
	wxMenuItem *ccwMenu, *cwMenu;
	if (mode == TWOPAGE &&
			leftNum != rightNum &&
			((direction == COMICAL_LTR && (eventPos.x + (viewX * unitX)) < (size.x / 2)) ||
			(direction == COMICAL_RTL && (eventPos.x + (viewX * unitX)) >= (size.x / 2)))) {
		ccwMenu = new wxMenuItem(contextRotate, ID_ContextLeftCCW, wxT("Rotate Counter-Clockwise"), wxT("Rotate 90 degrees counter-clockwise."));
		cwMenu = new wxMenuItem(contextRotate, ID_ContextLeftCW, wxT("Rotate Clockwise"), wxT("Rotate 90 degrees clockwise."));
	} else {
		ccwMenu = new wxMenuItem(contextRotate, ID_ContextCCW, wxT("Rotate Counter-Clockwise"), wxT("Rotate 90 degrees counter-clockwise."));
		cwMenu = new wxMenuItem(contextRotate, ID_ContextCW, wxT("Rotate Clockwise"), wxT("Rotate 90 degrees clockwise."));
	}
	ccwMenu->SetBitmap(wxGetBitmapFromMemory(rot_ccw));
	cwMenu->SetBitmap(wxGetBitmapFromMemory(rot_cw));
	contextRotate->Append(ccwMenu);
	contextRotate->Append(cwMenu);
	contextMenu->Append(ID_ContextRotate, wxT("&Rotate"), contextRotate);
	
	contextMenu->AppendSeparator();
	wxMenu *contextMode = new wxMenu();
	contextMode->AppendRadioItem(ID_Single, wxT("Single Page"), wxT("Show only a single page at a time."));
	contextMode->AppendRadioItem(ID_Double, wxT("Double Page"), wxT("Show two pages at a time."));
	contextMode->AppendRadioItem(ID_Continuous, wxT("Continuous Page"), wxT("Show a continuous page."));
	contextMenu->Append(ID_M, wxT("&Mode"), contextMode);
	
	contextMenu->AppendSeparator();
	wxMenuItem *fsMenu = new wxMenuItem(contextMenu, ID_ContextFull, wxT("Full Screen"));
	fsMenu->SetBitmap(wxGetBitmapFromMemory(fullscreen));	
	contextMenu->Append(fsMenu);
	
	contextMenu->AppendSeparator();
	contextMenu->Append(wxID_EXIT, wxT("E&xit"));
	
	switch (parent->zoom) {
		case ZOOM_FIT: contextMenu->Check(ID_Fit, true); break;
		case ZOOM_HEIGHT: contextMenu->Check(ID_FitV, true); break;
		case ZOOM_WIDTH: contextMenu->Check(ID_FitH, true); break;
		case ZOOM_FULL: contextMenu->Check(ID_Unzoomed, true); break;
		case ZOOM_CUSTOM: contextMenu->Check(ID_Custom, true); break;
	}
	
	switch (parent->mode) {
		case ONEPAGE: contextMenu->Check(ID_Single, true); break;
		case TWOPAGE: contextMenu->Check(ID_Double, true); break;
		case CONTINUOUS: contextMenu->Check(ID_Continuous, true); break;
	}
	
	PopupMenu(contextMenu, eventPos);
	delete contextMenu;
	contextMenu = NULL;
}

void ComicalCanvas::SetZoomEnable(bool enabled)
{
	zoomEnabled = enabled;
}

void ComicalCanvas::ShowCursor()
{
	if (IsScrolling(wxVERTICAL) || IsScrolling(wxHORIZONTAL)) {
		if (wxGetMouseState().LeftIsDown())
			SetCursor(m_grabbing);
		else
			SetCursor(m_grab);
	} else
		SetCursor(wxCURSOR_ARROW);
}

void ComicalCanvas::OnOpen(wxCommandEvent &event)
{
	parent->OnOpen(event);
}

void ComicalCanvas::OnOpenDir(wxCommandEvent &event)
{
	parent->OnOpenDir(event);
}

void ComicalCanvas::OnLeftDown(wxMouseEvent &event)
{
	SetFocus();
	
	if(!theBook) return;
	
	ShowCursor();
	
	pointerOrigin = event.GetPosition();
	
	if (zoomEnabled) {
		zoomOn = true;
	}
}

void ComicalCanvas::OnLeftUp(wxMouseEvent &event)
{
	if (!theBook) return;
		
	ShowCursor();
}

void ComicalCanvas::OnMouseMove(wxMouseEvent &event)
{
	if (zoomOn) {
		Refresh();
	} else if (event.Dragging() && event.LeftIsDown()) {
		wxPoint currMousePos = event.GetPosition();
		wxPoint diff, view, scroll;
		diff.x = currMousePos.x - pointerOrigin.x;
		diff.y = currMousePos.y - pointerOrigin.y;
		view = GetViewStart();
		scroll.x = view.x - diff.x;
		scroll.y  = view.y - diff.y;
		if (scroll.x < 0) scroll.x =  0;
		Scroll(scroll.x, scroll.y);
		pointerOrigin = currMousePos;
	}
	parent->ShowToolbar();
	ShowCursor();
}

void ComicalCanvas::SetDisallowWheelChangePage() {
	m_timerWheel.Start(1000, true);
	allowWheelChangePage = false;
}

void ComicalCanvas::OnWheelTimer(wxTimerEvent& event) {
	m_timerWheel.Stop();
	allowWheelChangePage = true;
}

void ComicalCanvas::OnMouseWheel(wxMouseEvent &event) {
	switch (mode) {
	case ONEPAGE:
	case TWOPAGE:
		if (event.GetWheelRotation() > 0) {
			if (IsScrolling(wxVERTICAL) && !IsScrollTop()) {
				ScrollUp();
				SetDisallowWheelChangePage();
			} else if (allowWheelChangePage) {
				if (!IsFirstPage()) scrollBottom = true;
				if (event.ControlDown()) PrevPageSlide(); else PrevPageTurn();
			}
		} else {
			if (IsScrolling(wxVERTICAL) && !IsScrollBottom()) {
				ScrollDown();
				SetDisallowWheelChangePage();
			} else if (allowWheelChangePage) {
				if (event.ControlDown()) NextPageSlide(); else NextPageTurn();
			}
		}		
		break;
		
	case CONTINUOUS:
		if (event.GetWheelRotation() > 0)
			if (parent->zoom == ZOOM_FIT || parent->zoom == ZOOM_WIDTH)
				PrevPageSlide();
			else
				if (event.MiddleIsDown() || event.LeftIsDown() || event.ControlDown()) ScrollUp(160); else ScrollLeft(160);
		else
			if (parent->zoom == ZOOM_FIT || parent->zoom == ZOOM_WIDTH)
				NextPageSlide();
			else
				if (event.MiddleIsDown() || event.LeftIsDown() || event.ControlDown()) ScrollDown(160); else ScrollRight(160);			
		break;
	}
}

void ComicalCanvas::SetComicBook(ComicBook *book)
{
	if (theBook) {
		theBook->Disconnect(EVT_PAGE_SCALED, wxCommandEventHandler(ComicalCanvas::OnPageReady), NULL, this);
	}
	theBook = book;
	if (theBook) {
		theBook->Connect(EVT_PAGE_SCALED, wxCommandEventHandler(ComicalCanvas::OnPageReady), NULL, this);
		parent->SetCanvasSize(GetSize());
		leftNum = NULL;
		rightNum = 0;
		if (mode == TWOPAGE) leftPart == EMPTY_PAGE;
	}
}

void ComicalCanvas::OnPageReady(wxCommandEvent &event)
{
	switch (mode) {
	case ONEPAGE:
		if (theBook->GetCurrentPage() == (wxUint32) event.GetInt()) {
			ResetView();
		}
		break;
		
	case TWOPAGE:
		if (leftNum == (wxUint32) event.GetInt() ||
				rightNum == (wxUint32) event.GetInt()) {
			ResetView();
		}
		break;
		
	case CONTINUOUS:
		ResetSize();
		UpdatePage();
		break;
	}
}

void ComicalCanvas::SendPageShownEvent()
{
	wxCommandEvent event(EVT_PAGE_SHOWN, -1);
	event.SetInt(theBook->GetCurrentPage());
	GetEventHandler()->AddPendingEvent(event);
}

void ComicalCanvas::OnResize(wxSizeEvent &event)
{
	if (!theBook) return;
	if (parent->SetCanvasSize(GetSize()) && theBook->IsRunning()) // if the parameters are actually different
		ResetView();
	parent->RepositionToolbar();
}

void ComicalCanvas::ClearCanvas()
{
	SetComicBook(NULL);
	clearBitmaps();
	ResetSize();
}

bool ComicalCanvas::IsScrolling(int orient) {
	switch (orient) {
		case wxHORIZONTAL: return GetVirtualSize().x > GetClientSize().x;
		case wxVERTICAL: return GetVirtualSize().y > GetClientSize().y;
	}
}

bool ComicalCanvas::IsScrollTop() {
	return !GetViewStart().y;
}

bool ComicalCanvas::IsScrollBottom() {
	return GetViewStart().y == GetVirtualSize().y - GetSize().y;
}

void ComicalCanvas::ScrollLeft(wxUint32 len) {
	Scroll(GetViewStart().x - len, -1);
}

void ComicalCanvas::ScrollRight(wxUint32 len) {
	Scroll(GetViewStart().x + len, -1);
}

void ComicalCanvas::ScrollUp(wxUint32 len) {
	Scroll(-1, GetViewStart().y - len);
}

void ComicalCanvas::ScrollDown(wxUint32 len) {
	Scroll(-1, GetViewStart().y + len);
}

void ComicalCanvas::ScrollLeftmost() {
	Scroll(0, -1);
}

void ComicalCanvas::ScrollRightmost() {
	Scroll(GetVirtualSize().x - GetSize().x, -1);
}

void ComicalCanvas::ScrollTop() {
	Scroll(-1, 0);
}

void ComicalCanvas::ScrollBottom() {
	Scroll(-1, GetVirtualSize().x - GetSize().GetY());
}

bool ComicalCanvas::IsFirstPage()
{
	if (!theBook) return false;

	switch (mode) {
	case ONEPAGE:
		return theBook->GetCurrentPage() == 0;
		break;
		
	case TWOPAGE:
		return leftPart == EMPTY_PAGE;
		
	case CONTINUOUS:
		if (parent->zoom == ZOOM_FIT || parent->zoom == ZOOM_WIDTH)
			return theBook->GetCurrentPage() == 0;
		else
			return GetViewStart().x == 0;
		break;
	}
}

bool ComicalCanvas::IsLastPage()
{
	if (!theBook) return false;
	
	switch (mode) {
	case ONEPAGE:
		return theBook->GetCurrentPage() == theBook->GetPageCount() - 1;
		
	case TWOPAGE:
		if (theBook->IsPageCountEven())
			return rightPart == EMPTY_PAGE;
		else {
			if (theBook->IsPageLandscape(rightNum))
				return rightNum == theBook->GetPageCount() - 1 && rightPart == RIGHT_HALF;
			else
				return rightNum == theBook->GetPageCount() - 1;
		}

	case CONTINUOUS:
		return GetViewStart().x == theBook->GetPagePos(theBook->GetPageCount() -1);
	}
}

bool ComicalCanvas::IsLeftPageOk()
{
	switch (mode) {
	case TWOPAGE:
		return leftPage.Ok();
		
	case CONTINUOUS:
		return theBook->Pages.at(leftNum)->GetPage().Ok();
	}
}

bool ComicalCanvas::IsRightPageOk()
{
	switch (mode) {
	case ONEPAGE:
	case TWOPAGE:
		return rightPage.Ok();
	
	case CONTINUOUS:
		return theBook->Pages.at(rightNum)->GetPage().Ok();
		break;
	}
}

bool ComicalCanvas::IsCenterPageOk()
{
	return centerPage.Ok();
}