/*
 * ComicalCanvas.h
 * Copyright (c) 2003-2011, James Athey
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

#ifndef _ComicalCanvas_h_
#define _ComicalCanvas_h_

#include "ComicBook.h"
#include "enums.h"

#include <wx/bitmap.h>
#include <wx/event.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/scrolwin.h>

enum
{
ID_WheelTimer
};

class ComicalFrame;

class ComicalCanvas : public wxScrolledWindow
{
	friend class ComicalFrame;
  public:
    ComicalCanvas(ComicalFrame *parent, COMICAL_MODE, COMICAL_DIRECTION);
    ~ComicalCanvas();

    void FirstPage();
    void GoToPage(wxUint32 pagenumber);
    void SetMode(COMICAL_MODE);
	void SetDirection(COMICAL_DIRECTION newDirection) { direction = newDirection; }
	void SetZoomEnable(bool);
	void ShowCursor();
	void SetComicBook(ComicBook *book);	
	void ClearCanvas();
	void ResetView();
	
	wxUint32 GetLeftNum() { return leftNum; }
	wxUint32 GetRightNum() { return rightNum; }
	bool IsFirstPage();
	bool IsLastPage();	
	bool IsLeftPageOk();
	bool IsRightPageOk();
	bool IsCenterPageOk();
	bool IsScrolling(int orient);
	bool IsScrollTop();
	bool IsScrollBottom();
	void ScrollLeft(wxUint32 len = 20);
	void ScrollRight(wxUint32 len = 20);
	void ScrollUp(wxUint32 len = 20);
	void ScrollDown(wxUint32 len = 20);
	void ScrollLeftmost();
	void ScrollRightmost();
	void ScrollTop();
	void ScrollBottom();

  private:
	void clearBitmaps();
	void UpdatePage();
	void ResetSize();
	void setPage(wxInt32 pagenumber);

	void OnFirst(wxCommandEvent& event) { FirstPage(); }
	void OnLast(wxCommandEvent& event) { LastPage(); }
	void OnPrevSlide(wxCommandEvent& event) { PrevPageSlide(); }
	void OnNextSlide(wxCommandEvent& event) { NextPageSlide(); }
	void OnPrevTurn(wxCommandEvent& event) { PrevPageTurn(); }
	void OnNextTurn(wxCommandEvent& event) { NextPageTurn(); }
	void OnPaint(wxPaintEvent &event);
	void OnEraseBackground(wxEraseEvent &event);
	void OnKeyDown(wxKeyEvent &event);
	void OnKeyUp(wxKeyEvent &event);
	void OnLeftDown(wxMouseEvent &event);
	void OnLeftUp(wxMouseEvent &event);
	void OnMouseMove(wxMouseEvent &event);
	void OnLeftDClick(wxMouseEvent &event);
	void OnMouseWheel(wxMouseEvent &event);
	void SetDisallowWheelChangePage();
	void OnWheelTimer(wxTimerEvent& event);
	void OnScroll(wxScrollWinEvent &event);
	void OnRightClick(wxContextMenuEvent &event);
	void OnOpen(wxCommandEvent& event);
	void OnOpenDir(wxCommandEvent& event);
	void OnRotateLeft(wxCommandEvent& event);
	void OnRotate(wxCommandEvent& event);
	void OnFull(wxCommandEvent& event);
	void OnResize(wxSizeEvent& event);
	void OnPageReady(wxCommandEvent& event);
	
	void LastPage();
	void PrevPageTurn();
	void NextPageTurn();
	void PrevPageSlide();
	void NextPageSlide();

	void SendPageShownEvent();
	
	virtual void Scroll(int x, int y);
#if !wxCHECK_VERSION(2, 9, 0)
	#if SIZEOF_INT == 4
		#define wxINT32_MAX INT_MAX
	#elif SIZEOF_LONG == 4
		#define wxINT32_MAX LONG_MAX
	#endif
	void GetViewStart(int *x, int *y) const { wxScrolledWindow::GetViewStart(x, y); }
	wxPoint GetViewStart() const {
		wxPoint pt;
		wxScrolledWindow::GetViewStart(&pt.x, &pt.y);
		return pt;
	}
#endif

    wxBitmap leftPage, rightPage, centerPage;
	wxCursor m_grab, m_grabbing;
    wxUint32 leftNum, rightNum;
    COMICAL_PAGETYPE leftPart, rightPart;
	wxMenu *contextMenu, *contextRotate;
	wxTimer m_timerWheel;

	ComicalFrame *parent;
    ComicBook *theBook;
	wxPoint pointerOrigin;
	bool zoomEnabled, zoomOn, scrollBottom, allowChangePage, allowWheelChangePage;
	COMICAL_MODE mode;
	COMICAL_DIRECTION direction;
	
	wxMutex paintingMutex;

    DECLARE_EVENT_TABLE()
};

enum
{
ID_ContextOpen,
ID_ContextOpenDir,
//Navigation
ID_ContextFirst,
ID_ContextLast,
ID_ContextPrevTurn,
ID_ContextNextTurn,
ID_ContextPrevSlide,
ID_ContextNextSlide,
//Rotation
ID_ContextRotate,
ID_ContextLeftCW,
ID_ContextLeftCCW,
ID_ContextCW,
ID_ContextCCW,
//View
ID_ContextFull
};

DECLARE_EVENT_TYPE(EVT_PAGE_SHOWN, -1)

#endif
