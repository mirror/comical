/***************************************************************************
                ComicalApp.h - ComicalApp class and gui functions
                             -------------------
    begin                : Wed Oct 22 2003
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

#ifndef _ComicalApp_h
#define _ComicalApp_h

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/utils.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/textctrl.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#endif

#include "ComicalCanvas.h"
#include "ComicBook.h"
#include "ComicBookRAR.h"
#include "ComicBookZIP.h"
#include "ComicBookBZ2.h"

ComicBook *theBook;
wxLog *ComicalLog;
wxLogWindow *ComicalLogWindow;

class ComicalApp : public wxApp {

public:

  virtual bool OnInit();

};

DECLARE_APP(ComicalApp)

class MainFrame : public wxFrame {

  public:

    MainFrame(const wxString& title,
              const wxPoint& pos,
              const wxSize& size,
              long style = wxDEFAULT_FRAME_STYLE);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnOpen();

    void OnFirst();
    void OnLast();
    void OnPrevTurn();
    void OnNextTurn();
    void OnPrevSlide();
    void OnNextSlide();
    void OnGoTo();
    void OnZoom(wxCommandEvent& event);
    void OnFull(wxCommandEvent& event);
    void OnSingle(wxCommandEvent& event);
    void OnDouble(wxCommandEvent& event);
    void OnShowLog();

    bool OpenFile(wxString);

    ComicalCanvas *theCanvas;

  private:

    wxMenuBar *menuBar;
    wxMenu *menuFile, *menuView, *menuHelp, *menuZoom, *menuMode;

    DECLARE_EVENT_TABLE()

};

enum
{
ID_ZFull = FULL,
ID_Z3Q = THREEQ,
ID_ZHalf = HALF,
ID_Z1Q = ONEQ,
ID_ZFit = FIT,
ID_ZFitV = FITV,
ID_ZFitH = FITH,
ID_Z,
ID_M,
ID_Single,
ID_Double,
ID_First,
ID_Last,
ID_PrevTurn,
ID_NextTurn,
ID_PrevSlide,
ID_NextSlide,
ID_GoTo,
ID_Full,
ID_ShowLog
};

#endif
