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
#include <wx/config.h>
#endif

#include "ComicalCanvas.h"
#include "ComicBook.h"
#include "ComicBookRAR.h"
#include "ComicBookZIP.h"

wxLogGui *ComicalLog;

class ComicalApp : public wxApp {

public:

  virtual bool OnInit();

};

DECLARE_APP(ComicalApp)

class ComicalFrame : public wxFrame {

  public:

    ComicalFrame(const wxString& title,
              const wxPoint& pos,
              const wxSize& size,
              long style = wxDEFAULT_FRAME_STYLE);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void OnFirst(wxCommandEvent& event);
    void OnLast(wxCommandEvent& event);
    void OnPrevTurn(wxCommandEvent& event);
    void OnNextTurn(wxCommandEvent& event);
    void OnPrevSlide(wxCommandEvent& event);
    void OnNextSlide(wxCommandEvent& event);
    void OnGoTo(wxCommandEvent& event);
    void OnZoom(wxCommandEvent& event);
    void OnFilter(wxCommandEvent& event);
    void OnFull(wxCommandEvent& event);
    void OnSingle(wxCommandEvent& event);
    void OnDouble(wxCommandEvent& event);
    void OnRotate(wxCommandEvent& event);

    void OpenFile(wxString);

    ComicalCanvas *theCanvas;
    ComicBook *theBook;

  private:

    wxMenuBar *menuBar;
    wxMenu *menuFile, *menuGo, *menuView, *menuHelp, *menuZoom, *menuMode, *menuFilter, *menuRotate;
    wxConfig *config;
    
    DECLARE_EVENT_TABLE()

};

enum
{
ID_Unzoomed,
ID_3Q,
ID_Half,
ID_1Q,
ID_Fit,
ID_FitV,
ID_FitH,
ID_S,
ID_M,
ID_Single,
ID_Double,
ID_F,
ID_Box,
ID_Bicubic,
ID_Bilinear,
ID_BSpline,
ID_CatmullRom,
ID_Lanczos,
ID_First,
ID_Last,
ID_PrevTurn,
ID_NextTurn,
ID_PrevSlide,
ID_NextSlide,
ID_GoTo,
ID_Full,
ID_North,
ID_East,
ID_South,
ID_West
};

#endif
