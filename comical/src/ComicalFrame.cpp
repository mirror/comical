//
// C++ Implementation: ComicalFrame
//
// Description: 
//
//
// Author: James Leighton Athey <jathey@comcast.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

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
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/utils.h>
#include <wx/mimetype.h>

#if !defined(__WXMSW__) && !defined(__WXPM__)
#include "../Comical Icons/firstpage.xpm"
#include "../Comical Icons/prevpage.xpm"
#include "../Comical Icons/prev.xpm"
#include "../Comical Icons/next.xpm"
#include "../Comical Icons/nextpage.xpm"
#include "../Comical Icons/lastpage.xpm"
#include "../Comical Icons/rot_cw.xpm"
#include "../Comical Icons/rot_ccw.xpm"
#endif

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
	
	config = new wxConfig(wxT("Comical"));
	wxConfigBase::Set(config); // Registers config globally	
	
	menuFile = new wxMenu;
	menuFile->Append(wxID_OPEN, wxT("&Open\tAlt-O"), wxT("Open a Comic Book."));
	menuFile->Append(ID_OpenDir, wxT("Open &Directory"), wxT("Open a directory of images."));
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT, wxT("E&xit\tAlt-X"), wxT("Quit Comical."));

	menuGo = new wxMenu;
	menuGo->Append(ID_PrevSlide, wxT("Previous Page"), wxT("Display the previous page."));
	menuGo->Append(ID_NextSlide, wxT("Next Page"), wxT("Display the next page."));
	menuGo->AppendSeparator();
	menuGo->Append(ID_PrevTurn, wxT("&Previous Page Turn"), wxT("Display the previous two pages."));
	menuGo->Append(ID_NextTurn, wxT("&Next Page Turn"), wxT("Display the next two pages."));
	menuGo->AppendSeparator();
	menuGo->Append(ID_First, wxT("&First Page"), wxT("Display the first page."));
	menuGo->Append(ID_Last, wxT("&Last Page"), wxT("Display the last page."));
	menuGo->Append(ID_GoTo, wxT("&Go to page..."), wxT("Jump to another page number."));
	menuGo->AppendSeparator();
	menuGo->Append(ID_Buffer, wxT("&Page Buffer Length..."), wxT("Set the number of pages Comical prefetches."));

	menuView = new wxMenu;

	menuZoom = new wxMenu;
	menuZoom->AppendRadioItem(ID_Fit, wxT("Fit"), wxT("Scale pages to fit the window."));
	menuZoom->AppendRadioItem(ID_FitV, wxT("Fit to Height"), wxT("Scale pages to fit the window's height."));
	menuZoom->AppendRadioItem(ID_FitH, wxT("Fit to Width"), wxT("Scale pages to fit the window's width."));
	menuZoom->AppendSeparator();
	menuZoom->AppendRadioItem(ID_Unzoomed, wxT("100%"), wxT("Original Size"));
	menuZoom->AppendRadioItem(ID_3Q, wxT("75%"), wxT("75% Zoom."));
	menuZoom->AppendRadioItem(ID_Half, wxT("50%"), wxT("50% Zoom."));
	menuZoom->AppendRadioItem(ID_1Q, wxT("25%"), wxT("25% Zoom."));
	menuView->Append(ID_S, wxT("&Zoom"), menuZoom);

	menuRotate = new wxMenu;
	menuRotate->AppendRadioItem(ID_North, wxT("Normal"), wxT("No rotation."));
	menuRotate->AppendRadioItem(ID_East, wxT("90 Clockwise"), wxT("Rotate 90 degrees clockwise."));
	menuRotate->AppendRadioItem(ID_South, wxT("180"), wxT("Rotate 180 degrees."));
	menuRotate->AppendRadioItem(ID_West, wxT("90 Counter-Clockwise"), wxT("Rotate 90 degrees counter-clockwise."));
	menuView->Append(ID_Rotate, wxT("&Rotate"), menuRotate);

	menuRotateLeft = new wxMenu;
	menuRotateLeft->AppendRadioItem(ID_NorthLeft, wxT("Normal"), wxT("No rotation."));
	menuRotateLeft->AppendRadioItem(ID_EastLeft, wxT("90 Clockwise"), wxT("Rotate 90 degrees clockwise."));
	menuRotateLeft->AppendRadioItem(ID_SouthLeft, wxT("180"), wxT("Rotate 180 degrees."));
	menuRotateLeft->AppendRadioItem(ID_WestLeft, wxT("90 Counter-Clockwise"), wxT("Rotate 90 degrees counter-clockwise."));
	menuView->Append(ID_RotateLeft, wxT("Rotate Left Page"), menuRotateLeft);

	menuRotateRight = new wxMenu;
	menuRotateRight->AppendRadioItem(ID_North, wxT("Normal"), wxT("No rotation."));
	menuRotateRight->AppendRadioItem(ID_East, wxT("90 Clockwise"), wxT("Rotate 90 degrees clockwise."));
	menuRotateRight->AppendRadioItem(ID_South, wxT("180"), wxT("Rotate 180 degrees."));
	menuRotateRight->AppendRadioItem(ID_West, wxT("90 Counter-Clockwise"), wxT("Rotate 90 degrees counter-clockwise."));
	menuView->Append(ID_RotateRight, wxT("Rotate Ri&ght Page"), menuRotateRight);

	menuMode = new wxMenu;
	menuMode->AppendRadioItem(ID_Single, wxT("Single Page"), wxT("Show only a single page at a time."));
	menuMode->AppendRadioItem(ID_Double, wxT("Double Page"), wxT("Show two pages at a time."));
	menuView->Append(ID_M, wxT("&Mode"), menuMode);

	menuFilter = new wxMenu;
	menuFilter->AppendRadioItem(ID_Box, wxT("Box"), wxT("Use the Box filter."));
	menuFilter->AppendRadioItem(ID_Bilinear, wxT("Bilinear"), wxT("Use the Bilinear filter."));
	menuFilter->AppendRadioItem(ID_Bicubic, wxT("Bicubic"), wxT("Use the Bicubic filter."));
	menuFilter->AppendRadioItem(ID_BSpline, wxT("B-Spline"), wxT("Use the B-Spline filter."));
	menuFilter->AppendRadioItem(ID_CatmullRom, wxT("Catmull-Rom"), wxT("Use the Catmull-Rom filter."));
	menuFilter->AppendRadioItem(ID_Lanczos, wxT("Lanczos 3"), wxT("Use the Box filter."));
	menuView->Append(ID_S, wxT("&Image Filter"), menuFilter);

	menuView->AppendSeparator();
	menuView->Append(ID_Full, wxT("Full &Screen\tAlt-Return"), wxT("Display Full Screen."));
	menuView->AppendSeparator();
	wxMenuItem *browserMenu = menuView->AppendCheckItem(ID_Browser, wxT("Thumbnail Browser"), wxT("Show/Hide the thumbnail browser"));
	wxMenuItem *toolbarMenu = menuView->AppendCheckItem(ID_Toolbar, wxT("Toolbar"), wxT("Show/Hide the toolbar"));

	menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT, wxT("&About...\tF1"), wxT("Display About Dialog."));
	menuHelp->Append(ID_Homepage, wxT("&Comical Homepage"), wxT("Go to http://comical.sourceforge.net/"));

	menuBar = new wxMenuBar();
	menuBar->Append(menuFile, wxT("&File"));
	menuBar->Append(menuGo, wxT("&Go"));
	menuBar->Append(menuView, wxT("&View"));
	menuBar->Append(menuHelp, wxT("&Help"));

	SetMenuBar(menuBar);

	// Each of the long values is followed by the letter L not the number one
	long Zoom = config->Read(wxT("Zoom"), 2l); // Fit-to-Width is default
	long Mode = config->Read(wxT("Mode"), 1l); // Double-Page is default
	long Filter = config->Read(wxT("Filter"), 4l); // Catmull-Rom is default
	browserActive = (bool) config->Read(wxT("BrowserActive"), 1l); // Shown by default
	toolbarActive = (bool) config->Read(wxT("ToolbarActive"), 1l); // Shown by default
	
	// Record the settings from the config in the menus
	menuZoom->FindItemByPosition(Zoom)->Check(true);
	menuMode->FindItemByPosition(Mode)->Check(true);
	menuFilter->FindItemByPosition(Filter)->Check(true);
	browserMenu->Check(browserActive);
	toolbarMenu->Check(toolbarActive);	
		
	theBrowser = new ComicalBrowser(this, wxDefaultPosition, wxSize(110, 10));
	frameSizer->Add(theBrowser, 0, wxEXPAND, 0);
	
	theCanvas = new ComicalCanvas(this, wxDefaultPosition, wxSize(10, 10));
	bookPanelSizer->Add(theCanvas, 1, wxEXPAND, 0);
	theBrowser->SetComicalCanvas(theCanvas);

	toolBarNav = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxTB_HORIZONTAL | wxTB_FLAT);
	toolBarNav->SetToolBitmapSize(wxSize(16, 16));
	toolBarNav->AddTool(ID_CCWL, wxT("Rotate Counter-Clockwise (left page)"), wxBITMAP(rot_ccw), wxT("Rotate Counter-Clockwise (left page)"));
	toolBarNav->AddTool(ID_CWL, wxT("Rotate Clockwise (left page)"), wxBITMAP(rot_cw), wxT("Rotate Clockwise (left page)"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_First, wxT("First Page"), wxBITMAP(firstpage), wxT("First Page"));
	toolBarNav->AddTool(ID_PrevTurn, wxT("Previous Page Turn"), wxBITMAP(prevpage), wxT("Previous Page Turn"));
	toolBarNav->AddTool(ID_PrevSlide, wxT("Previous Page"), wxBITMAP(prev), wxT("Previous Page"));
//	toolBarNav->AddSeparator();
//	toolBarNav->AddTool(ID_ZoomBox, wxT("Zoom"), wxBITMAP(rot_cw), wxT("Zoom"), wxITEM_CHECK); // Zoom Box disabled for now
//	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_NextSlide, wxT("Next Page"), wxBITMAP(next), wxT("Next Page"));
	toolBarNav->AddTool(ID_NextTurn, wxT("Next Page Turn"), wxBITMAP(nextpage), wxT("Next Page Turn"));
	toolBarNav->AddTool(ID_Last, wxT("Last Page"), wxBITMAP(lastpage), wxT("Last Page"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_CCW, wxT("Rotate Counter-Clockwise"), wxBITMAP(rot_ccw), wxT("Rotate Counter-Clockwise"));
	toolBarNav->AddTool(ID_CW, wxT("Rotate Clockwise"), wxBITMAP(rot_cw), wxT("Rotate Clockwise"));
	toolBarNav->Enable(false);
	toolBarNav->Fit();
	toolBarNav->Realize();

	labelLeft = new wxStaticText(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
	labelRight = new wxStaticText(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE);
	wxFont font = labelLeft->GetFont();
	font.SetPointSize(10);
	labelLeft->SetFont(font);
	labelRight->SetFont(font);
	
	toolbarSizer->Add(labelLeft, 1, wxALIGN_LEFT, 0);
	toolbarSizer->AddSpacer(10);
	toolbarSizer->Add(toolBarNav, 0, wxALIGN_CENTER, 0);
	toolbarSizer->AddSpacer(10);
	toolbarSizer->Add(labelRight, 1, wxALIGN_RIGHT, 0);
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
	EVT_MENU(ID_Double,	ComicalFrame::OnDouble)
	EVT_MENU(ID_Single,	ComicalFrame::OnSingle)
	EVT_MENU(ID_ZoomBox, ComicalFrame::OnZoomBox)
	EVT_MENU(ID_Browser, ComicalFrame::OnBrowser)
	EVT_MENU(ID_Toolbar, ComicalFrame::OnToolbar)
	EVT_MENU(ID_Homepage, ComicalFrame::OnHomepage)
	EVT_CLOSE(ComicalFrame::OnClose)
END_EVENT_TABLE()

void ComicalFrame::OnClose(wxCloseEvent& event)
{
	if (theBook) {
		theBook->Delete(); // delete the ComicBook thread
		delete theBook; // clear out the rest of the ComicBook
		theBook = NULL;
	}

	wxRect frameDim = GetRect();
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
	wxMessageDialog AboutDlg(this, wxT("Comical 0.8, (c) 2003-2005 James Athey.\nComical is licensed under the GPL, version 2,\nwith a linking exception; see README for details."), wxT("About Comical"), wxOK);
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

void ComicalFrame::OpenFile(wxString filename)
{
	ComicBook *newBook;
	
	if (!filename.empty()) {
		clearComicBook();
		try {
			if (filename.Right(4).Upper() == wxT(".CBR") || filename.Right(4).Upper() == wxT(".RAR"))
				newBook = new ComicBookRAR(filename);
			else if (filename.Right(4).Upper() == wxT(".CBZ") || filename.Right(4).Upper() == wxT(".ZIP"))
				newBook = new ComicBookZIP(filename);
			
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

void ComicalFrame::OpenDir(wxString directory)
{
	ComicBook *newBook;
	if (!directory.empty()) {
		clearComicBook();
		try {
			newBook = new ComicBookDir(directory);
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
		theCanvas->SetParams(false);

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

void ComicalFrame::OnSingle(wxCommandEvent& event)
{
	wxMenuItem *prev = menuGo->FindItem(ID_PrevTurn);
	wxMenuItem *next = menuGo->FindItem(ID_NextTurn);
	prev->Enable(false);
	next->Enable(false);
	theCanvas->Mode(ONEPAGE);
}

void ComicalFrame::OnDouble(wxCommandEvent& event)
{
	wxMenuItem *prev = menuGo->FindItem(ID_PrevTurn);
	wxMenuItem *next = menuGo->FindItem(ID_NextTurn);
	prev->Enable(true);
	next->Enable(true);
	theCanvas->Mode(TWOPAGE);
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
	// Untested code follows
	wxFileType *ft = wxTheMimeTypesManager->GetFileTypeFromMimeType(wxT("text/html"));
	if (!ft) {
		wxLogError(wxT("Could not find a program to open HTML files."));
		wxLog::FlushActive();
		return;
	}

	wxString cmd;
	bool open = ft->GetOpenCommand(&cmd, wxFileType::MessageParameters(wxT("http://comical.sourceforge.net/"), wxT("text/html")));
	delete ft;

	wxLogError(cmd);
	
	if (!open) {
		wxLogError(wxT("Could not construct command to start web browser."));
		wxLog::FlushActive();
		return;
	}

	if (wxExecute(cmd) == 0) {
		wxLogError(wxT("Could not launch web browser."));
		wxLog::FlushActive();
	}
}

void ComicalFrame::setComicBook(ComicBook *newBook)
{
	if (theBook)
		clearComicBook();
	theBook = newBook;
	if (theBook) {
		theBook->Connect(ID_PageError, EVT_PAGE_ERROR, wxCommandEventHandler(ComicalFrame::OnPageError), NULL, this);
	}
}

void ComicalFrame::clearComicBook()
{
	if (theBook) {
		theBook->Disconnect(ID_PageError, EVT_PAGE_ERROR, wxCommandEventHandler(ComicalFrame::OnPageError), NULL, this);
		theBook->Delete(); // delete the ComicBook thread
		delete theBook; // clear out the rest of the ComicBook
		theBook = NULL;
	}
	if (theCanvas)
		theCanvas->ClearCanvas();
	if (theBrowser)
		theBrowser->ClearBrowser();
}
