/***************************************************************************
      ComicalCanvas.h - ComicalCanvas class and supporting declarations
                             -------------------
    begin                : Thu Dec 18 2003
    copyright            : (C) 2003 by James Athey
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

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/bitmap.h>
#include <wx/scrolwin.h>
#include <wx/scrolbar.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/stream.h>
#include <wx/log.h>
#include <wx/config.h>
#endif

#include "ComicBook.h"

enum PAGETYPE {FULL_PAGE, LEFT_HALF, RIGHT_HALF};

class ComicalCanvas : public wxScrolledWindow
{

  public:
    ComicalCanvas() {}
    ComicalCanvas(wxWindow *parent, const wxPoint &pos, const wxSize &size);
    ~ComicalCanvas();

    void FirstPage();
    void LastPage();
    void GoToPage(uint pagenumber);
    void PrevPageTurn();
    void NextPageTurn();
    void PrevPageSlide();
    void NextPageSlide();
    void Zoom(COMICAL_ZOOM);
    void Mode(COMICAL_MODE);
    void Filter(FREE_IMAGE_FILTER);
    void Rotate(bool);
    void RotateLeft(bool);
    void Rotate(COMICAL_ROTATE);
    void RotateLeft(COMICAL_ROTATE);

    void SetParams();

    ComicBook *theBook;

  private:
    void clearBitmap(wxBitmap *&bitmap);
    void clearBitmaps();
    void createBitmaps();
    void setPage(int pagenumber);

    void OnPaint(wxPaintEvent &event);
    void OnKeyDown(wxKeyEvent &event);
    void OnSize(wxSizeEvent &event);

    wxBitmap *leftPage, *rightPage, *centerPage;
    uint leftNum, rightNum;
    PAGETYPE leftPart, rightPart;

	int scrollBarThickness;

    COMICAL_ZOOM zoom;
    COMICAL_MODE mode;
    FREE_IMAGE_FILTER filter;

    wxWindow *parent;

    DECLARE_DYNAMIC_CLASS(ComicalCanvas)
    DECLARE_EVENT_TABLE()

};

#endif
