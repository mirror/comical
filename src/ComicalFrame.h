/*
 * ComicalFrame.h
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

#ifndef _ComicalFrame_h
#define _ComicalFrame_h

#include <wx/aui/aui.h>
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/config.h>
#include <wx/toolbar.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/mstream.h>
#include <wx/timer.h>

#include "ComicalBrowser.h"
#include "ComicalCanvas.h"
#include "ComicBook.h"

class ComicalFrame : public wxFrame
{
	friend class ComicalApp;
	friend class ComicalCanvas;
  public:

    ComicalFrame(const wxString& title,
              const wxPoint& pos,
              const wxSize& size);
    ~ComicalFrame();

    void OpenFile(const wxString&);
    void OpenDir(const wxString&);
    void OnOpen(wxCommandEvent& event);
    void OnOpenDir(wxCommandEvent& event);

	void UpdateToolbar();
	void RepositionToolbar();
	void ShowToolbar();

	wxSize canvasSize;
	wxInt32 scrollbarThickness; wxSize scrollbarSize;

  private:

	void OnFull(wxCommandEvent& event); void Full();
	void OnQuit(wxCommandEvent& event); void Quit();
	void OnAbout(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnGoTo(wxCommandEvent& event);
	void OnMode(wxCommandEvent& event);
	void OnBuffer(wxCommandEvent& event);
	void OnZoomBox(wxCommandEvent& event);
	void OnBrowser(wxCommandEvent& event);
	void OnCustomEvent(wxCommandEvent& event);
	void OnHomepage(wxCommandEvent& event);
	void OnZoom(wxCommandEvent& event);
	void OnSetCustom(wxCommandEvent& event);
	void OnFilter(wxCommandEvent& event);
	void OnShowScrollbars(wxCommandEvent& event); void ShowScrollbars(bool);
	void OnFitOnlyOversize(wxCommandEvent& event);
	void OnDirection(wxCommandEvent& event);
	void OnPageShown(wxCommandEvent& event);
	void OnMove(wxMoveEvent& event);
	void OnToolbarHideTimer(wxTimerEvent& event);
	void OnPageTimer(wxTimerEvent& event);

	void SavePage();
	wxUint32 ReadPage(wxString filename);
	
	void startBook();
	void setComicBook(ComicBook *newBook);
	void clearComicBook();
	bool SetCanvasSize(wxSize size);
	void SetScrollbarSize(wxInt32 scrollbarThickness);
	
	wxMiniFrame *toolbarFrame;
	wxToolBar *toolBarNav;
	wxMenuBar *menuBar;
	wxMenu *menuFile, *menuGo, *menuView, *menuHelp, *menuZoom, *menuMode, *menuFilter, *menuDirection, *menuRotate, *menuRotateLeft, *menuRotateRight;
	wxStaticText *labelLeft, *labelRight;

    wxConfigBase *config;
	wxBoxSizer *toolbarSizer;

	wxAuiManager m_auiManager;
	wxTimer m_timerToolbarHide, m_timerPage;

	bool browserActive;
	ComicalCanvas *theCanvas;
	ComicBook *theBook;
	ComicalBrowser *theBrowser;

	wxUint32 cacheLen, pageNum;
	COMICAL_MODE mode;
	FREE_IMAGE_FILTER filter;
	COMICAL_ZOOM zoom;
	bool showScrollbars;
	bool fitOnlyOversize;
	long zoomLevel;
	COMICAL_DIRECTION direction;
	
	DECLARE_EVENT_TABLE()
};

#define wxGetBitmapFromMemory(name) _wxGetBitmapFromMemory(name ## _png, sizeof(name ## _png))

inline wxBitmap _wxGetBitmapFromMemory(const unsigned char *data, int length) {
   wxMemoryInputStream is(data, length);
   return wxBitmap(wxImage(is, wxBITMAP_TYPE_ANY, -1), -1);
}

enum
{
//Events
ID_Error,
ID_SetPassword,
ID_PageAdded,
ID_Opened,
//Input
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
ID_Homepage,
ID_HideTimer,
ID_PageTimer,
//Zooms
ID_Fit,
ID_FitV,
ID_FitH,
ID_Unzoomed,
ID_Custom,
ID_ShowScrollbars,
ID_FitOnlyOversize,
ID_SetCustom,
//Modes
ID_Single,
ID_Double,
ID_Continuous,
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
