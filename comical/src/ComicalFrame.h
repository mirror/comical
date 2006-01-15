//
// C++ Interface: ComicalFrame
//
// Description: 
//
//
// Author: James Leighton Athey <jathey@comcast.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//


#ifndef _ComicalFrame_h
#define _ComicalFrame_h

#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/config.h>
#include <wx/toolbar.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

#include "ComicalBrowser.h"
#include "ComicalCanvas.h"
#include "ComicBook.h"

class ComicalFrame : public wxFrame
{
  public:

    ComicalFrame(const wxString& title,
              const wxPoint& pos,
              const wxSize& size,
              long style = wxDEFAULT_FRAME_STYLE);

    void OpenFile(wxString);
    void OpenDir(wxString);
    void OnOpen(wxCommandEvent& event);
    void OnOpenDir(wxCommandEvent& event);
    void OnFull(wxCommandEvent& event);

    wxToolBar *toolBarNav;
    wxMenuBar *menuBar;
    wxMenu *menuFile, *menuGo, *menuView, *menuHelp, *menuZoom, *menuMode, *menuFilter, *menuRotate, *menuRotateLeft, *menuRotateRight;
	wxStaticText *labelLeft, *labelRight;

  private:

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnGoTo(wxCommandEvent& event);
    void OnSingle(wxCommandEvent& event);
    void OnDouble(wxCommandEvent& event);
    void OnBuffer(wxCommandEvent& event);
	void OnZoomBox(wxCommandEvent& event);
	void OnBrowser(wxCommandEvent& event);
	void OnToolbar(wxCommandEvent& event);
	void startBook();
	
    wxConfig *config;
	wxBoxSizer *frameSizer, *bookPanelSizer, *toolbarSizer;	
	bool browserActive, toolbarActive;
    ComicalCanvas *theCanvas;
    ComicBook *theBook;
    ComicalBrowser *theBrowser;

    DECLARE_EVENT_TABLE()
};

enum
{
ID_S,
ID_M,
ID_OpenDir,
ID_Rotate,
ID_RotateLeft,
ID_RotateRight,
ID_Full,
ID_ZoomBox,
ID_Browser,
ID_Toolbar,
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
ID_North,
ID_East,
ID_South,
ID_West,
ID_CWL,
ID_CCWL,
ID_NorthLeft,
ID_EastLeft,
ID_SouthLeft,
ID_WestLeft
};

#endif
