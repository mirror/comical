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

#include <wx/utils.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/textctrl.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/tokenzr.h>
#include <wx/config.h>
#include <wx/log.h>
#include <wx/toolbar.h>
#include <wx/stattext.h>

#include "ComicalCanvas.h"
#include "ComicBook.h"
#include "ComicBookRAR.h"
#include "ComicBookZIP.h"
#include "Exceptions.h"

class ComicalApp : public wxApp {

public:

  virtual bool OnInit();

};

DECLARE_APP(ComicalApp)

class ComicalFrame : public wxFrame
{
  public:

    ComicalFrame(const wxString& title,
              const wxPoint& pos,
              const wxSize& size,
              long style = wxDEFAULT_FRAME_STYLE);

    void OpenFile(wxString);
    void OnOpen(wxCommandEvent& event);
    void OnFull(wxCommandEvent& event);

    wxToolBar *toolBarNav;
    wxMenuBar *menuBar;
    wxMenu *menuFile, *menuGo, *menuView, *menuHelp, *menuZoom, *menuMode, *menuFilter, *menuRotate, *menuRotateLeft, *menuRotateRight;
	wxStaticText *labelLeft, *labelRight;

  protected:
    wxSize GetClientSize();

  private:

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
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
    void OnSingle(wxCommandEvent& event);
    void OnDouble(wxCommandEvent& event);
    void OnRotate(wxCommandEvent& event);
    void OnRotateLeft(wxCommandEvent& event);
    void OnSize(wxSizeEvent &event);
    void OnBuffer(wxCommandEvent& event);

    wxConfig *config;
    wxLog *ComicalLog;

    ComicalCanvas *theCanvas;
    ComicBook *theBook;

    DECLARE_EVENT_TABLE()
};

enum
{
ID_S,
ID_M,
ID_Rotate,
ID_RotateLeft,
ID_RotateRight,
ID_Full,
//Zooms
ID_Unzoomed,
ID_3Q,
ID_Half,
ID_1Q,
ID_Fit,
ID_FitV,
ID_FitH,
//Modes
ID_Single,
ID_Double,
//Filters
ID_Box,
ID_Bicubic,
ID_Bilinear,
ID_BSpline,
ID_CatmullRom,
ID_Lanczos,
//Navigation
ID_First,
ID_Last,
ID_PrevTurn,
ID_NextTurn,
ID_PrevSlide,
ID_NextSlide,
ID_GoTo,
ID_Buffer,
//Rotation
ID_CW,
ID_CCW,
ID_CWL,
ID_CCWL,
ID_North,
ID_East,
ID_South,
ID_West,
ID_NorthLeft,
ID_EastLeft,
ID_SouthLeft,
ID_WestLeft
};

#endif
