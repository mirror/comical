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
#include <wx/image.h>
#include <wx/scrolwin.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/mstream.h>
#include <wx/log.h>
#endif

enum zooms {ONEQ = 7, HALF, THREEQ, FULL, FIT, FITV, FITH};
enum modes {SINGLE = 1, DOUBLE = 2};

class ComicalCanvas : public wxScrolledWindow {

  public:
    ComicalCanvas() {}
    ComicalCanvas(wxWindow *parent, wxWindowID, const wxPoint &pos, const wxSize &size);
    ~ComicalCanvas();

    void OnPaint(wxPaintEvent &event);
    void OnKeyDown(wxKeyEvent &event);

    void ClearBitmaps();
    void ClearImages();
    void FirstPage();
    void LastPage();
    void GoToPage(int pagenumber);
    void PrevPageTurn();
    void NextPageTurn();
    void PrevPageSlide();
    void NextPageSlide();
    void Scale(int value);
    void Mode(int mode);

  private:
    wxImage GetPage(int pagenumber);
    wxImage ScaleImage(wxImage);
    void CreateBitmaps();
    wxBitmap *leftPage, *rightPage, *centerPage;
    wxImage leftImage, leftImageScaled, rightImage, rightImageScaled, centerImage, centerImageScaled;

    wxWindow *parent;
    int scaled, mode;
    int x,y;

    DECLARE_DYNAMIC_CLASS(ComicalCanvas)
    DECLARE_EVENT_TABLE()

};

#endif
