/***************************************************************************
      ComicalCanvas.h - ComicalCanvas class and supporting declarations
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

#ifndef _ComicalCanvas_h_
#define _ComicalCanvas_h_

#include <wx/bitmap.h>
#include <wx/scrolwin.h>
#include <wx/scrolbar.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/stream.h>
#include <wx/log.h>
#include <wx/config.h>
#include <wx/event.h>
#include <wx/menu.h>
#include <wx/pen.h>
#include <wx/brush.h>
#include <wx/utils.h>
#include <wx/thread.h>

#include "ComicBook.h"

enum COMICAL_PAGETYPE {FULL_PAGE, LEFT_HALF, RIGHT_HALF};
//enum COMICAL_POSITION {POSITION_LEFT, POSITION_RIGHT, POSITION_CENTER};

class ComicalCanvas : public wxScrolledWindow
{

  public:
    ComicalCanvas(wxWindow *parent, const wxPoint &pos, const wxSize &size, COMICAL_MODE, COMICAL_DIRECTION, wxInt32 scrollbarThickness);
    ~ComicalCanvas();

    void FirstPage();
    void GoToPage(wxUint32 pagenumber);
    void SetMode(COMICAL_MODE);
	void SetDirection(COMICAL_DIRECTION newDirection) { direction = newDirection; }
	void SetZoomEnable(bool);
	void SetComicBook(ComicBook *book);	
	void ClearCanvas();
	void ResetView();

  private:
	void clearBitmap(wxBitmap *&bitmap);
	void clearBitmaps();
	void createBitmaps();
	void setPage(wxInt32 pagenumber);

	void OnFirst(wxCommandEvent& event) { FirstPage(); }
	void OnLast(wxCommandEvent& event) { LastPage(); }
	void OnPrevSlide(wxCommandEvent& event) { PrevPageSlide(); }
	void OnNextSlide(wxCommandEvent& event) { NextPageSlide(); }
	void OnPrevTurn(wxCommandEvent& event) { PrevPageTurn(); }
	void OnNextTurn(wxCommandEvent& event) { NextPageTurn(); }
	void OnPaint(wxPaintEvent &event);
	void OnKeyDown(wxKeyEvent &event);
	void OnLeftDown(wxMouseEvent &event);
	void OnLeftUp(wxMouseEvent &event);
	void OnMouseMove(wxMouseEvent &event);
	void OnRightClick(wxContextMenuEvent &event);
	void OnOpen(wxCommandEvent& event);
	void OnOpenDir(wxCommandEvent& event);
	void OnRotateLeft(wxCommandEvent& event);
	void OnRotate(wxCommandEvent& event);
	void OnFull(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnPageReady(wxCommandEvent& event);
	
	void LastPage();
	void PrevPageTurn();
	void NextPageTurn();
	void PrevPageSlide();
	void NextPageSlide();

	void SendCurrentPageChangedEvent();
	//void SendPageShownEvent(wxUint32 pagenumber, COMICAL_POSITION position);

    wxBitmap *leftPage, *rightPage, *centerPage;
    wxUint32 leftNum, rightNum;
    COMICAL_PAGETYPE leftPart, rightPart;
	wxMenu *contextMenu, *contextRotate;

    ComicBook *theBook;
	wxPoint pointerOrigin;
	bool zoomEnabled, zoomOn;
	COMICAL_MODE mode;
	COMICAL_DIRECTION direction;
	wxInt32 scrollbarThickness;
	
	wxMutex paintingMutex;
	
    wxWindow *parent;

    DECLARE_EVENT_TABLE()
};

enum
{
//ID_PageShown,
ID_ContextOpen,
ID_ContextOpenDir,
//Navigation
ID_ContextFirst,
ID_ContextLast,
ID_ContextPrevTurn,
ID_ContextNextTurn,
ID_ContextPrevSlide,
ID_ContextNextSlide,
ID_CurrentPageChanged,
//Rotation
ID_ContextRotate,
ID_ContextLeftCW,
ID_ContextLeftCCW,
ID_ContextCW,
ID_ContextCCW,
//View
ID_ContextFull
};

//DECLARE_EVENT_TYPE(EVT_PAGE_SHOWN, -1)

#endif
