/***************************************************************************
                              ComicalFrame.cpp
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

#include "ComicalFrame.h"
#include "ComicBookRAR.h"
#include "ComicBookZIP.h"
#include "ComicBookDir.h"
#include "Exceptions.h"
#include <wx/textdlg.h>
#include <wx/textctrl.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <wx/numdlg.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/utils.h>
#include <wx/mimetype.h>
#include <wx/event.h>
#include <wx/scrolbar.h>

ComicalFrame::ComicalFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(NULL, -1, title, pos, size, style)
{
	theCanvas = NULL;
	theBook = NULL;
	theBrowser = NULL;
	toolBarNav = NULL;
	labelLeft = NULL;
	labelRight = NULL;
	
	frameSizer = new wxBoxSizer(wxHORIZONTAL);
	bookPanelSizer = new wxBoxSizer(wxVERTICAL);
	toolbarSizer = new wxBoxSizer(wxHORIZONTAL);
	
	config = wxConfig::Get();
	
	// Get the thickness of scrollbars.  Knowing this, we can precalculate
	// whether the current page(s) will need scrollbars.
	wxScrollBar *tempBar = new wxScrollBar(this, -1);
	scrollbarThickness = tempBar->GetSize().y;
	tempBar->Destroy();

	wxMenuItem *openMenu = new wxMenuItem(NULL, wxID_OPEN, wxT("&Open\tAlt-O"), wxT("Open a Comic Book."));
	wxMenuItem *exitMenu = new wxMenuItem(NULL, wxID_EXIT, wxT("E&xit\tAlt-X"), wxT("Quit Comical."));

	openMenu->SetBitmap(wxGetBitmapFromMemory(open));
	exitMenu->SetBitmap(wxGetBitmapFromMemory(exit));
	
	menuFile = new wxMenu();
	menuFile->Append(openMenu);
	menuFile->Append(ID_OpenDir, wxT("Open &Directory"), wxT("Open a directory of images."));
	menuFile->AppendSeparator();
	menuFile->Append(exitMenu);

	wxMenuItem *prevMenu = new wxMenuItem(NULL, ID_PrevSlide, wxT("Previous Page"), wxT("Display the previous page."));
	wxMenuItem *nextMenu = new wxMenuItem(NULL, ID_NextSlide, wxT("Next Page"), wxT("Display the next page."));
	wxMenuItem *prevTurnMenu = new wxMenuItem(NULL, ID_PrevTurn, wxT("&Previous Page Turn"), wxT("Display the previous two pages."));
	wxMenuItem *nextTurnMenu = new wxMenuItem(NULL, ID_NextTurn, wxT("&Next Page Turn"), wxT("Display the next two pages."));
	wxMenuItem *firstMenu = new wxMenuItem(NULL, ID_First, wxT("&First Page"), wxT("Display the first page."));
	wxMenuItem *lastMenu = new wxMenuItem(NULL, ID_Last, wxT("&Last Page"), wxT("Display the last page."));

	prevMenu->SetBitmap(wxGetBitmapFromMemory(prev));
	nextMenu->SetBitmap(wxGetBitmapFromMemory(next));
	prevTurnMenu->SetBitmap(wxGetBitmapFromMemory(prevpage));
	nextTurnMenu->SetBitmap(wxGetBitmapFromMemory(nextpage));
	firstMenu->SetBitmap(wxGetBitmapFromMemory(firstpage));
	lastMenu->SetBitmap(wxGetBitmapFromMemory(lastpage));

	menuGo = new wxMenu();
	menuGo->Append(prevMenu);
	menuGo->Append(nextMenu);
	menuGo->AppendSeparator();
	menuGo->Append(prevTurnMenu);
	menuGo->Append(nextTurnMenu);
	menuGo->AppendSeparator();
	menuGo->Append(firstMenu);
	menuGo->Append(lastMenu);
	menuGo->Append(ID_GoTo, wxT("&Go to page..."), wxT("Jump to another page number."));
	menuGo->AppendSeparator();
	menuGo->Append(ID_Buffer, wxT("&Page Buffer Length..."), wxT("Set the number of pages Comical prefetches."));
	menuView = new wxMenu();

	menuZoom = new wxMenu();
	menuZoom->AppendRadioItem(ID_Fit, wxT("Fit"), wxT("Scale pages to fit the window."));
	menuZoom->AppendRadioItem(ID_FitV, wxT("Fit to Height"), wxT("Scale pages to fit the window's height."));
	menuZoom->AppendRadioItem(ID_FitH, wxT("Fit to Width"), wxT("Scale pages to fit the window's width."));
	menuZoom->AppendRadioItem(ID_Unzoomed, wxT("Original Size"), wxT("Show pages without resizing them."));
	menuZoom->AppendRadioItem(ID_Custom, wxT("Custom Zoom"), wxT("Scale pages to a custom percentage of their original size."));
	menuZoom->AppendSeparator();
	wxMenuItem *fitOnlyOversizeMenu = menuZoom->AppendCheckItem(ID_FitOnlyOversize, wxT("Fit Oversized Pages Only"), wxT("Check to see if a page will fit on screen before resizing it."));
	menuZoom->Append(ID_SetCustom, wxT("Set Custom Zoom Level..."), wxT("Choose the percentage that the Custom Zoom mode will use."));
	menuView->Append(ID_S, wxT("&Zoom"), menuZoom);
	
	menuRotate = new wxMenu();
	menuRotate->AppendRadioItem(ID_North, wxT("Normal"), wxT("No rotation."));
	menuRotate->AppendRadioItem(ID_East, wxT("90 Clockwise"), wxT("Rotate 90 degrees clockwise."));
	menuRotate->AppendRadioItem(ID_South, wxT("180"), wxT("Rotate 180 degrees."));
	menuRotate->AppendRadioItem(ID_West, wxT("90 Counter-Clockwise"), wxT("Rotate 90 degrees counter-clockwise."));
	menuView->Append(ID_Rotate, wxT("&Rotate"), menuRotate);

	menuRotateLeft = new wxMenu();
	menuRotateLeft->AppendRadioItem(ID_NorthLeft, wxT("Normal"), wxT("No rotation."));
	menuRotateLeft->AppendRadioItem(ID_EastLeft, wxT("90 Clockwise"), wxT("Rotate 90 degrees clockwise."));
	menuRotateLeft->AppendRadioItem(ID_SouthLeft, wxT("180"), wxT("Rotate 180 degrees."));
	menuRotateLeft->AppendRadioItem(ID_WestLeft, wxT("90 Counter-Clockwise"), wxT("Rotate 90 degrees counter-clockwise."));
	menuView->Append(ID_RotateLeft, wxT("Rotate Left Page"), menuRotateLeft);

	menuRotateRight = new wxMenu();
	menuRotateRight->AppendRadioItem(ID_North, wxT("Normal"), wxT("No rotation."));
	menuRotateRight->AppendRadioItem(ID_East, wxT("90 Clockwise"), wxT("Rotate 90 degrees clockwise."));
	menuRotateRight->AppendRadioItem(ID_South, wxT("180"), wxT("Rotate 180 degrees."));
	menuRotateRight->AppendRadioItem(ID_West, wxT("90 Counter-Clockwise"), wxT("Rotate 90 degrees counter-clockwise."));
	menuView->Append(ID_RotateRight, wxT("Rotate Ri&ght Page"), menuRotateRight);

	menuMode = new wxMenu();
	menuMode->AppendRadioItem(ID_Single, wxT("Single Page"), wxT("Show only a single page at a time."));
	menuMode->AppendRadioItem(ID_Double, wxT("Double Page"), wxT("Show two pages at a time."));
	menuView->Append(ID_M, wxT("&Mode"), menuMode);

	menuDirection = new wxMenu();
	menuDirection->AppendRadioItem(ID_LeftToRight, wxT("&Left-to-Right"), wxT("Turn pages from left-to-right, suitable for Western comics."));
	menuDirection->AppendRadioItem(ID_RightToLeft, wxT("&Right-to-Left"), wxT("Turn pages from right-to-left, suitable for Manga."));
	menuView->Append(ID_D, wxT("&Direction"), menuDirection);
	
	menuFilter = new wxMenu();
	menuFilter->AppendRadioItem(ID_Box, wxT("Box"), wxT("Use the Box filter."));
	menuFilter->AppendRadioItem(ID_Bilinear, wxT("Bilinear"), wxT("Use the Bilinear filter."));
	menuFilter->AppendRadioItem(ID_Bicubic, wxT("Bicubic"), wxT("Use the Bicubic filter."));
	menuFilter->AppendRadioItem(ID_BSpline, wxT("B-Spline"), wxT("Use the B-Spline filter."));
	menuFilter->AppendRadioItem(ID_CatmullRom, wxT("Catmull-Rom"), wxT("Use the Catmull-Rom filter."));
	menuFilter->AppendRadioItem(ID_Lanczos, wxT("Lanczos 3"), wxT("Use the Box filter."));
	menuView->Append(ID_S, wxT("&Image Filter"), menuFilter);

	wxMenuItem *fsMenu = new wxMenuItem(NULL, ID_Full, wxT("Full &Screen\tAlt-Return"), wxT("Display Full Screen."));
	fsMenu->SetBitmap(wxGetBitmapFromMemory(fullscreen));
	
	menuView->AppendSeparator();
	menuView->Append(fsMenu);
	menuView->AppendSeparator();
	wxMenuItem *browserMenu = menuView->AppendCheckItem(ID_Browser, wxT("Thumbnail Browser"), wxT("Show/Hide the thumbnail browser"));
	wxMenuItem *toolbarMenu = menuView->AppendCheckItem(ID_Toolbar, wxT("Toolbar"), wxT("Show/Hide the toolbar"));

	menuHelp = new wxMenu();
	menuHelp->Append(wxID_ABOUT, wxT("&About...\tF1"), wxT("Display About Dialog."));
	menuHelp->Append(ID_Homepage, wxT("&Comical Homepage"), wxT("Go to http://comical.sourceforge.net/"));

	menuBar = new wxMenuBar();
	menuBar->Append(menuFile, wxT("&File"));
	menuBar->Append(menuGo, wxT("&Go"));
	menuBar->Append(menuView, wxT("&View"));
	menuBar->Append(menuHelp, wxT("&Help"));

	SetMenuBar(menuBar);

	// Each of the long values is followed by the letter L not the number one
	cacheLen = (wxUint32) config->Read(wxT("CacheLength"), 10l); // Fit-to-Width is default
	zoom = (COMICAL_ZOOM) config->Read(wxT("Zoom"), 2l); // Fit-to-Width is default
	zoomLevel = config->Read(wxT("ZoomLevel"), 100l); // 100% is the default
	mode = (COMICAL_MODE) config->Read(wxT("Mode"), 1l); // Double-Page is default
	filter = (FREE_IMAGE_FILTER) config->Read(wxT("Filter"), 4l); // Catmull-Rom is default
	direction = (COMICAL_DIRECTION) config->Read(wxT("Direction"), 0l); // Left-To-Right by default
	fitOnlyOversize = (bool) config->Read(wxT("FitOnlyOversize"), 0l); // off by default
	browserActive = (bool) config->Read(wxT("BrowserActive"), 1l); // Shown by default
	toolbarActive = (bool) config->Read(wxT("ToolbarActive"), 1l); // Shown by default

	// Record the settings from the config in the menus
	menuZoom->FindItemByPosition(zoom)->Check(true);
	menuMode->FindItemByPosition(mode)->Check(true);
	menuFilter->FindItemByPosition(filter)->Check(true);
	menuDirection->FindItemByPosition(direction)->Check(true);
	fitOnlyOversizeMenu->Check(fitOnlyOversize);
	browserMenu->Check(browserActive);
	toolbarMenu->Check(toolbarActive);
	if (mode == ONEPAGE) {
		prevTurnMenu->Enable(false);
		nextTurnMenu->Enable(false);
	} // else they're already enabled

	theBrowser = new ComicalBrowser(this, wxDefaultPosition, wxSize(110, 10));
	frameSizer->Add(theBrowser, 0, wxEXPAND, 0);
	
	theCanvas = new ComicalCanvas(this, wxDefaultPosition, wxSize(10, 10), mode, direction, scrollbarThickness);
	theCanvas->Connect(EVT_PAGE_SHOWN, wxCommandEventHandler(ComicalFrame::OnPageShown), NULL, this);
	theCanvas->Connect(ID_ContextFull, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ComicalFrame::OnFull), NULL, this);
	bookPanelSizer->Add(theCanvas, 1, wxEXPAND, 0);
	theBrowser->SetComicalCanvas(theCanvas);

	toolBarNav = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxTB_HORIZONTAL | wxTB_FLAT);
	toolBarNav->SetToolBitmapSize(wxSize(16, 16));
	toolBarNav->AddTool(ID_CCWL, wxT("Rotate Counter-Clockwise (left page)"), wxGetBitmapFromMemory(rot_ccw), wxT("Rotate Counter-Clockwise (left page)"));
	toolBarNav->AddTool(ID_CWL, wxT("Rotate Clockwise (left page)"), wxGetBitmapFromMemory(rot_cw), wxT("Rotate Clockwise (left page)"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_First, wxT("First Page"), wxGetBitmapFromMemory(firstpage), wxT("First Page"));
	toolBarNav->AddTool(ID_PrevTurn, wxT("Previous Page Turn"), wxGetBitmapFromMemory(prevpage), wxT("Previous Page Turn"));
	toolBarNav->AddTool(ID_PrevSlide, wxT("Previous Page"), wxGetBitmapFromMemory(prev), wxT("Previous Page"));
//	toolBarNav->AddSeparator();
//	toolBarNav->AddTool(ID_ZoomBox, wxT("Zoom"), wxGetBitmapFromMemory(rot_cw), wxT("Zoom"), wxITEM_CHECK); // Zoom Box disabled for now
//	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_NextSlide, wxT("Next Page"), wxGetBitmapFromMemory(next), wxT("Next Page"));
	toolBarNav->AddTool(ID_NextTurn, wxT("Next Page Turn"), wxGetBitmapFromMemory(nextpage), wxT("Next Page Turn"));
	toolBarNav->AddTool(ID_Last, wxT("Last Page"), wxGetBitmapFromMemory(lastpage), wxT("Last Page"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_CCW, wxT("Rotate Counter-Clockwise"), wxGetBitmapFromMemory(rot_ccw), wxT("Rotate Counter-Clockwise"));
	toolBarNav->AddTool(ID_CW, wxT("Rotate Clockwise"), wxGetBitmapFromMemory(rot_cw), wxT("Rotate Clockwise"));
	toolBarNav->Enable(false);
	toolBarNav->Fit();
	toolBarNav->Realize();

	labelLeft = new wxStaticText(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
	labelRight = new wxStaticText(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	wxFont font = labelLeft->GetFont();
	font.SetPointSize(10);
	labelLeft->SetFont(font);
	labelRight->SetFont(font);
	
	toolbarSizer->Add(labelLeft, 1, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, 0);
	toolbarSizer->AddSpacer(10);
	toolbarSizer->Add(toolBarNav, 0, wxALIGN_CENTER, 0);
	toolbarSizer->AddSpacer(10);
	toolbarSizer->Add(labelRight, 1, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 0);
	toolbarSizer->Layout();
	bookPanelSizer->Add(toolbarSizer, 0, wxEXPAND, 0);
	frameSizer->Add(bookPanelSizer, 1, wxEXPAND);
	SetSizer(frameSizer);
	
	frameSizer->Show(theBrowser, browserActive);
	bookPanelSizer->Show(toolbarSizer, toolbarActive, true);
}

BEGIN_EVENT_TABLE(ComicalFrame, wxFrame)
	EVT_MENU(wxID_EXIT,	ComicalFrame::OnQuit)
	EVT_MENU(wxID_ABOUT,	ComicalFrame::OnAbout)
	EVT_MENU(wxID_OPEN,	ComicalFrame::OnOpen)
	EVT_MENU(ID_OpenDir,	ComicalFrame::OnOpenDir)
	EVT_MENU(ID_GoTo,	ComicalFrame::OnGoTo)
	EVT_MENU(ID_Buffer,	ComicalFrame::OnBuffer)
	EVT_MENU(ID_Full,	ComicalFrame::OnFull)
	EVT_MENU_RANGE(ID_Single, ID_Double, ComicalFrame::OnMode)
	EVT_MENU_RANGE(ID_Fit, ID_Custom, ComicalFrame::OnZoom)
	EVT_MENU(ID_FitOnlyOversize, ComicalFrame::OnFitOnlyOversize)
	EVT_MENU(ID_SetCustom, ComicalFrame::OnSetCustom)
	EVT_MENU_RANGE(ID_Box, ID_Lanczos, ComicalFrame::OnFilter)
	EVT_MENU_RANGE(ID_LeftToRight, ID_RightToLeft, ComicalFrame::OnDirection)
//	EVT_MENU(ID_ZoomBox, ComicalFrame::OnZoomBox)
	EVT_MENU(ID_Browser, ComicalFrame::OnBrowser)
	EVT_MENU(ID_Toolbar, ComicalFrame::OnToolbar)
	EVT_MENU(ID_Homepage, ComicalFrame::OnHomepage)
	EVT_CLOSE(ComicalFrame::OnClose)
END_EVENT_TABLE()

void ComicalFrame::OnClose(wxCloseEvent& event)
{
	clearComicBook();
	theCanvas->ClearCanvas();
	delete theCanvas;

	wxRect frameDim = GetRect();
	config->Write(wxT("CacheLength"), (int) cacheLen);
	config->Write(wxT("Zoom"), zoom);
	config->Write(wxT("ZoomLevel"), zoomLevel);
	config->Write(wxT("FitOnlyOversize"), fitOnlyOversize);
	config->Write(wxT("Filter"), filter);
	config->Write(wxT("Mode"), mode);
	config->Write(wxT("Direction"), direction);
	config->Write(wxT("FrameWidth"), frameDim.width);
	config->Write(wxT("FrameHeight"), frameDim.height);
	config->Write(wxT("FrameX"), frameDim.x);
	config->Write(wxT("FrameY"), frameDim.y);
	config->Write(wxT("ToolbarActive"), toolbarActive);
	config->Write(wxT("BrowserActive"), browserActive);
	Destroy();	// Close the window
}

void ComicalFrame::OnQuit(wxCommandEvent& event)
{
	Close(TRUE);
}

void ComicalFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageDialog AboutDlg(this, wxT("Comical 0.8, (c) 2003-2006 James Athey.\nComical is licensed under the GPL, version 2,\nwith a linking exception; see README for details."), wxT("About Comical"), wxOK);
	AboutDlg.ShowModal();
}

void ComicalFrame::OnOpen(wxCommandEvent& event)
{
	wxString cwd;
	config->Read(wxT("CWD"), &cwd);
	wxString filename = wxFileSelector(wxT("Open a Comic Book"), cwd, wxT(""), wxT(""), wxT("Comic Books (*.cbr,*.cbz,*.rar,*.zip)|*.cbr;*.CBR;*.cbz;*.CBZ;*.rar;*.RAR;*.zip;*.ZIP"), wxOPEN | wxCHANGE_DIR | wxFILE_MUST_EXIST, this);

	if (!filename.empty())
		OpenFile(filename);
}

void ComicalFrame::OnOpenDir(wxCommandEvent& event)
{
	wxString cwd;
	config->Read(wxT("CWD"), &cwd);
	wxString dir = wxDirSelector(wxT("Open a Directory"), cwd, 0, wxDefaultPosition, this);

	if (!dir.empty())
		OpenDir(dir);
}

void ComicalFrame::OpenFile(const wxString& filename)
{
	ComicBook *newBook;
	
	if (!filename.empty()) {
		clearComicBook();
		try {
			if (filename.Right(4).Upper() == wxT(".CBR") || filename.Right(4).Upper() == wxT(".RAR"))
				newBook = new ComicBookRAR(filename, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction, scrollbarThickness);
			else if (filename.Right(4).Upper() == wxT(".CBZ") || filename.Right(4).Upper() == wxT(".ZIP"))
				newBook = new ComicBookZIP(filename, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction, scrollbarThickness);
			else {
				wxLogError(wxT("Cannot open ") + filename);
				wxLog::FlushActive();
				return;
			}
			
			if (newBook->GetPageCount() == 0) {
				wxLogError(wxT("The archive \"") + filename + wxT("\" does not contain any pages."));
				wxLog::FlushActive();
				return;
			}
			setComicBook(newBook);
			startBook();
			SetTitle(wxT("Comical - " + filename));
			config->Write(wxT("CWD"), wxPathOnly(filename));
		} catch (ArchiveException &ae) {
			clearComicBook();
			wxLogError(ae.Message);
			wxLog::FlushActive();
		}
	}
}

void ComicalFrame::OpenDir(const wxString& directory)
{
	ComicBook *newBook;
	if (!directory.empty()) {
		clearComicBook();
		try {
			newBook = new ComicBookDir(directory, cacheLen, zoom, zoomLevel, fitOnlyOversize, mode, filter, direction, scrollbarThickness);
			setComicBook(newBook);
			startBook();
			SetTitle(wxT("Comical - " + directory));
			config->Write(wxT("CWD"), directory);
		} catch (ArchiveException &ae) {
			wxLogError(ae.Message);
			wxLog::FlushActive();
		}
	}
}

void ComicalFrame::startBook()
{
	if (theBook) {
		theCanvas->SetComicBook(theBook);
		theBrowser->SetComicBook(theBook);
		theBrowser->SetItemCount(theBook->GetPageCount());

		toolBarNav->Enable(true);
	
		theBook->Run(); // start the thread
	
		theCanvas->FirstPage();
		theCanvas->Scroll(-1, 0); // scroll to the top for the first page
	}
}

void ComicalFrame::OnGoTo(wxCommandEvent& event)
{
	wxString message ;
	long pagenumber;
	if (theBook != NULL) {
		message = wxT("Enter a page number from 1 to ");
		message += wxString::Format(wxT("%d"), theBook->GetPageCount());
		pagenumber = wxGetNumberFromUser(message, wxT("Page"), wxT("Go To Page"), theBook->GetCurrentPage() + 1, 1, theBook->GetPageCount(), this);
		if (pagenumber != -1)
			theCanvas->GoToPage(pagenumber - 1);
	}

}

void ComicalFrame::OnBuffer(wxCommandEvent& event)
{
	wxString message;
	long buffer;
	if (theBook != NULL) {
		message = wxT("Set the number of pages you would like Comical to prefetch.");
		buffer = wxGetNumberFromUser(message, wxT("Buffer Length"), wxT("Set Buffer Length"), theBook->GetCacheLen(), 3, 20, this);
		if (buffer != -1)
			theBook->SetCacheLen(buffer);
	}
}

void ComicalFrame::OnFull(wxCommandEvent& event)
{
	if (IsFullScreen()) {
		bookPanelSizer->Show(toolbarSizer, toolbarActive, true);
		if (theBrowser)
			frameSizer->Show(theBrowser, browserActive);
		ShowFullScreen(false, wxFULLSCREEN_ALL);
	} else {
		bookPanelSizer->Show(toolbarSizer, false, true);
		if (theBrowser)
			frameSizer->Show(theBrowser, false);
		ShowFullScreen(true, wxFULLSCREEN_ALL);		
	}
}

void ComicalFrame::OnMode(wxCommandEvent& event)
{
	wxMenuItem *prev = menuGo->FindItem(ID_PrevTurn);
	wxMenuItem *next = menuGo->FindItem(ID_NextTurn);
	
	switch (event.GetId()) {
	
	case ID_Single:
		prev->Enable(false);
		next->Enable(false);
		mode = ONEPAGE;
		break;
	
	case ID_Double:
		prev->Enable(true);
		next->Enable(true);
		mode = TWOPAGE;
		break;
	
	default:
		return;
	}

	if (theBook)
		theBook->SetMode(mode);
	if (theCanvas)
		theCanvas->SetMode(mode); // In this case, theCanvas decides how to redraw
}

void ComicalFrame::OnZoomBox(wxCommandEvent &event)
{
	if (theCanvas != NULL)
		theCanvas->SetZoomEnable(event.IsChecked());
}

void ComicalFrame::OnToolbar(wxCommandEvent &event)
{
	toolbarActive = event.IsChecked();
	bookPanelSizer->Show(toolbarSizer, toolbarActive, true);
	bookPanelSizer->Layout();
}

void ComicalFrame::OnBrowser(wxCommandEvent &event)
{
	browserActive = event.IsChecked();
	if (theBrowser)
		frameSizer->Show(theBrowser, browserActive);
	frameSizer->Layout();
}

void ComicalFrame::OnPageError(wxCommandEvent &event)
{
	wxLogError(event.GetString());
	wxLog::FlushActive();
}

void ComicalFrame::OnHomepage(wxCommandEvent &event)
{
	wxLaunchDefaultBrowser(wxT("http://comical.sourceforge.net/"));
}

void ComicalFrame::OnZoom(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case ID_Fit:
		zoom = ZOOM_FIT;
		if (theCanvas)
			theCanvas->EnableScrolling(false, false); // Fit, no scrolling
		break;
	case ID_FitV:
		zoom = ZOOM_HEIGHT;
		if (theCanvas)
			theCanvas->EnableScrolling(true, false); // Vertical fit, no vertical scrolling
		break;
	case ID_FitH:
		zoom = ZOOM_WIDTH;
		if (theCanvas)
			theCanvas->EnableScrolling(false, true); // Horizontal fit, no horizontal scrolling
		break;
	case ID_Unzoomed:
		zoom = ZOOM_FULL;
		if (theCanvas)
			theCanvas->EnableScrolling(true, true);	
		break;
	case ID_Custom:
		zoom = ZOOM_CUSTOM;
		if (theCanvas)
			theCanvas->EnableScrolling(true, true);	
		break;
	default:
		wxLogError(wxT("Zoom mode %d is undefined."), event.GetId()); // we shouldn't be here... honest!
		return;
	}

	if (theBook && theBook->SetZoom(zoom) && theBook->IsRunning())
		theCanvas->ResetView();
}

void ComicalFrame::OnSetCustom(wxCommandEvent& event)
{
	long newZoomLevel = wxGetNumberFromUser(wxT("Set the zoom level (in %)"), wxT("Zoom"), wxT("Custom Zoom"), zoomLevel, 1, 200);
	if (newZoomLevel < 0) {
		return;
	}
	zoomLevel = newZoomLevel;
	if (theBook && theBook->SetZoomLevel(zoomLevel) && theBook->IsRunning() && theCanvas && zoom == ZOOM_CUSTOM)
		theCanvas->ResetView();
}

void ComicalFrame::OnFilter(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case ID_Box:
		filter = FILTER_BOX;
		break;
	case ID_Bilinear:
		filter = FILTER_BILINEAR;
		break;
	case ID_Bicubic:
		filter = FILTER_BICUBIC;
		break;
	case ID_BSpline:
		filter = FILTER_BSPLINE;
		break;
	case ID_CatmullRom:
		filter = FILTER_CATMULLROM;
		break;
	case ID_Lanczos:
		filter = FILTER_LANCZOS3;
		break;
	default:
		wxLogError(wxT("Filter %d is undefined."), event.GetId()); // we shouldn't be here... honest!
		return;
	}
	
	if (theBook && theBook->SetFilter(filter) && theBook->IsRunning())
		theCanvas->ResetView();
}

void ComicalFrame::OnFitOnlyOversize(wxCommandEvent& event)
{
	fitOnlyOversize = event.IsChecked();
	if (theBook && theBook->SetFitOnlyOversize(fitOnlyOversize) && theBook->IsRunning())
		theCanvas->ResetView();
}

void ComicalFrame::OnDirection(wxCommandEvent& event)
{
	switch (event.GetId()) {
	case ID_LeftToRight:
		direction = COMICAL_LTR;
		break;
	case ID_RightToLeft:
		direction = COMICAL_RTL;
		break;
	default:
		wxLogError(wxT("I don't know what direction this is: %d"), event.GetId());
		return;
	}
	if (theCanvas)
		theCanvas->SetDirection(direction);
	if (theBook && theBook->SetDirection(direction) && theBook->IsRunning())
		theCanvas->ResetView();	
}

void ComicalFrame::OnPageShown(wxCommandEvent& event)
{
	wxUint32 leftNum = theCanvas->GetLeftNum();
	wxUint32 rightNum = theCanvas->GetRightNum();
	
	if (mode == ONEPAGE || theBook->GetPageCount() == 1 || leftNum == rightNum) {
		menuView->Enable(ID_RotateLeft, false);
		menuView->Enable(ID_RotateRight, false);
		menuView->Enable(ID_Rotate, true);
		menuRotate->FindItemByPosition(theBook->Orientations[theBook->GetCurrentPage()])->Check();
		toolBarNav->EnableTool(ID_CCWL, false);
		toolBarNav->EnableTool(ID_CWL, false);
		toolBarNav->EnableTool(ID_CCW, true);
		toolBarNav->EnableTool(ID_CW, true);
		
		labelLeft->SetLabel(wxT(""));
		labelRight->SetLabel(wxString::Format(wxT("%d of %d"), theBook->GetCurrentPage() + 1, theBook->GetPageCount()));
	} else {
		menuView->Enable(ID_Rotate, false);

		if (theCanvas->IsRightPageOk()) {
			menuView->Enable(ID_RotateRight, true);
			menuRotateRight->FindItemByPosition(theBook->Orientations[rightNum])->Check();
			toolBarNav->EnableTool(ID_CCW, true);
			toolBarNav->EnableTool(ID_CW, true);
			if (direction == COMICAL_LTR)
				labelRight->SetLabel(wxString::Format(wxT("%d of %d"), rightNum + 1, theBook->GetPageCount()));
			else // direction == COMICAL_RTL
				labelLeft->SetLabel(wxString::Format(wxT("%d of %d"), rightNum + 1, theBook->GetPageCount()));
		} else {
			menuView->Enable(ID_RotateRight, false);
			toolBarNav->EnableTool(ID_CCW, false);
			toolBarNav->EnableTool(ID_CW, false);
			if (direction == COMICAL_LTR)
				labelRight->SetLabel(wxT(""));
			else // direction == COMICAL_RTL
				labelLeft->SetLabel(wxT(""));
		}
	
		if (theCanvas->IsLeftPageOk()) {
			menuView->Enable(ID_RotateLeft, true);
			menuRotateLeft->FindItemByPosition(theBook->Orientations[leftNum])->Check();
			toolBarNav->EnableTool(ID_CCWL, true);
			toolBarNav->EnableTool(ID_CWL, true);
			
			if (direction == COMICAL_LTR)
				labelLeft->SetLabel(wxString::Format(wxT("%d of %d"), leftNum + 1, theBook->GetPageCount()));
			else // direction == COMICAL_RTL
				labelRight->SetLabel(wxString::Format(wxT("%d of %d"), leftNum + 1, theBook->GetPageCount()));
		} else {
			menuView->Enable(ID_RotateLeft, false);
			toolBarNav->EnableTool(ID_CCWL, false);
			toolBarNav->EnableTool(ID_CWL, false);
			if (direction == COMICAL_LTR)
				labelLeft->SetLabel(wxT(""));
			else // direction == COMICAL_RTL
				labelRight->SetLabel(wxT(""));
		}
	}

	toolBarNav->Realize();

}

void ComicalFrame::setComicBook(ComicBook *newBook)
{
	if (theBook)
		clearComicBook();
	theBook = newBook;
	if (theBook) {
		theBook->Connect(EVT_PAGE_ERROR, wxCommandEventHandler(ComicalFrame::OnPageError), NULL, this);
	}
}

void ComicalFrame::clearComicBook()
{
	if (theCanvas)
		theCanvas->SetComicBook(NULL);
	if (theBrowser)
		theBrowser->SetComicBook(NULL);
	if (theBook) {
		theBook->Disconnect(EVT_PAGE_ERROR, wxCommandEventHandler(ComicalFrame::OnPageError), NULL, this);
		theBook->Delete(); // delete the ComicBook thread
		delete theBook; // clear out the rest of the ComicBook
		theBook = NULL;
	}
	if (theCanvas)
		theCanvas->ClearCanvas();
	if (theBrowser)
		theBrowser->ClearBrowser();
}
