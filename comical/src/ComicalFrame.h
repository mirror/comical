/***************************************************************************
                              ComicalFrame.h
                             -------------------
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

#ifndef _ComicalFrame_h
#define _ComicalFrame_h

#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/config.h>
#include <wx/toolbar.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/mstream.h>

#include "ComicalBrowser.h"
#include "ComicalCanvas.h"
#include "ComicBook.h"

// and the icons
#include "firstpage.h"
#include "prevpage.h"
#include "prev.h"
#include "next.h"
#include "nextpage.h"
#include "lastpage.h"
#include "rot_cw.h"
#include "rot_ccw.h"
#include "open.h"
#include "fullscreen.h"
#include "exit.h"

class ComicalFrame : public wxFrame
{
  public:

    ComicalFrame(const wxString& title,
              const wxPoint& pos,
              const wxSize& size,
              long style = wxDEFAULT_FRAME_STYLE);

    void OpenFile(const wxString&);
    void OpenDir(const wxString&);
    void OnOpen(wxCommandEvent& event);
    void OnOpenDir(wxCommandEvent& event);

  private:

	void OnFull(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnGoTo(wxCommandEvent& event);
	void OnMode(wxCommandEvent& event);
	void OnBuffer(wxCommandEvent& event);
	void OnZoomBox(wxCommandEvent& event);
	void OnBrowser(wxCommandEvent& event);
	void OnToolbar(wxCommandEvent& event);
	void OnPageError(wxCommandEvent& event);
	void OnHomepage(wxCommandEvent& event);
	void OnZoom(wxCommandEvent& event);
	void OnSetCustom(wxCommandEvent& event);
	void OnFilter(wxCommandEvent& event);
	void OnFitOnlyOversize(wxCommandEvent& event);
	void OnDirection(wxCommandEvent& event);
	void OnPageShown(wxCommandEvent& event);
	void startBook();
	void setComicBook(ComicBook *newBook);
	void clearComicBook();
	
	wxToolBar *toolBarNav;
	wxMenuBar *menuBar;
	wxMenu *menuFile, *menuGo, *menuView, *menuHelp, *menuZoom, *menuMode, *menuFilter, *menuDirection, *menuRotate, *menuRotateLeft, *menuRotateRight;
	wxStaticText *labelLeft, *labelRight;

    wxConfigBase *config;
	wxBoxSizer *frameSizer, *bookPanelSizer, *toolbarSizer;	
	bool browserActive, toolbarActive;
	ComicalCanvas *theCanvas;
	ComicBook *theBook;
	ComicalBrowser *theBrowser;

	wxUint32 cacheLen;
	COMICAL_MODE mode;
	FREE_IMAGE_FILTER filter;
	COMICAL_ZOOM zoom;
	bool fitOnlyOversize;
	long zoomLevel;
	COMICAL_DIRECTION direction;
	wxInt32 scrollbarThickness;

	DECLARE_EVENT_TABLE()
};

#define wxGetBitmapFromMemory(name) _wxGetBitmapFromMemory(name ## _png, sizeof(name ## _png))

inline wxBitmap _wxGetBitmapFromMemory(const unsigned char *data, int length) {
   wxMemoryInputStream is(data, length);
   return wxBitmap(wxImage(is, wxBITMAP_TYPE_ANY, -1), -1);
}

enum
{
ID_S,
ID_M,
ID_D,
ID_OpenDir,
ID_Rotate,
ID_RotateLeft,
ID_RotateRight,
ID_Full,
ID_ZoomBox,
ID_Browser,
ID_Toolbar,
ID_Homepage,
//Zooms
ID_Fit,
ID_FitV,
ID_FitH,
ID_Unzoomed,
ID_Custom,
ID_FitOnlyOversize,
ID_SetCustom,
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
//Directions
ID_LeftToRight,
ID_RightToLeft,
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
