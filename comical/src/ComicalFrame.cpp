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
	theBook = NULL;
	theBrowser = NULL;
	toolBarNav = NULL;
	labelLeft = NULL;
	labelRight = NULL;
	
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

	menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT, wxT("&About...\tF1"), wxT("Display About Dialog."));

	menuBar = new wxMenuBar();
	menuBar->Append(menuFile, wxT("&File"));
	menuBar->Append(menuGo, wxT("&Go"));
	menuBar->Append(menuView, wxT("&View"));
	menuBar->Append(menuHelp, wxT("&Help"));

	SetMenuBar(menuBar);

	// Each of the long values is followed by the letter L not the number one
	long Zoom = config->Read(wxT("/Comical/Zoom"), 2l); // Fit-to-Width is default
	long Mode = config->Read(wxT("/Comical/Mode"), 1l); // Double-Page is default
	long Filter = config->Read(wxT("/Comical/Filter"), 4l); // Catmull-Rom is default

	// Record the settings from the config in the menus
	menuZoom->FindItemByPosition(Zoom)->Check(true);
	menuMode->FindItemByPosition(Mode)->Check(true);
	menuFilter->FindItemByPosition(Filter)->Check(true);
	
	theBrowser = new ComicalBrowser(this, wxPoint(0,0), wxSize(110, GetClientSize().y));
	theCanvas = new ComicalCanvas(this, wxPoint(110,0), this->GetClientSize());
	theBrowser->SetComicalCanvas(theCanvas);

	toolBarNav = new wxToolBar(this, -1, wxPoint(110, this->GetClientSize().y), wxDefaultSize, wxNO_BORDER | wxTB_HORIZONTAL | wxTB_FLAT);
	toolBarNav->SetToolBitmapSize(wxSize(16, 16));
	toolBarNav->AddTool(ID_CCWL, wxT("Rotate Counter-Clockwise (left page)"), wxBITMAP(rot_ccw), wxT("Rotate Counter-Clockwise (left page)"));
	toolBarNav->AddTool(ID_CWL, wxT("Rotate Clockwise (left page)"), wxBITMAP(rot_cw), wxT("Rotate Clockwise (left page)"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_First, wxT("First Page"), wxBITMAP(firstpage), wxT("First Page"));
	toolBarNav->AddTool(ID_PrevTurn, wxT("Previous Page Turn"), wxBITMAP(prevpage), wxT("Previous Page Turn"));
	toolBarNav->AddTool(ID_PrevSlide, wxT("Previous Page"), wxBITMAP(prev), wxT("Previous Page"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_ZoomBox, wxT("Zoom"), wxBITMAP(rot_cw), wxT("Zoom"), wxITEM_CHECK);
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_NextSlide, wxT("Next Page"), wxBITMAP(next), wxT("Next Page"));
	toolBarNav->AddTool(ID_NextTurn, wxT("Next Page Turn"), wxBITMAP(nextpage), wxT("Next Page Turn"));
	toolBarNav->AddTool(ID_Last, wxT("Last Page"), wxBITMAP(lastpage), wxT("Last Page"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_CCW, wxT("Rotate Counter-Clockwise"), wxBITMAP(rot_ccw), wxT("Rotate Counter-Clockwise"));
	toolBarNav->AddTool(ID_CW, wxT("Rotate Clockwise"), wxBITMAP(rot_cw), wxT("Rotate Clockwise"));
	toolBarNav->Enable(false);
	toolBarNav->Fit();

	wxSize clientSize = GetClientSize();
	wxInt32 tbX = ((clientSize.x - toolBarNav->GetSize().x) / 2) + theBrowser->GetSize().x;
	toolBarNav->SetSize(tbX, clientSize.y, toolBarNav->GetSize().x, -1);
	toolBarNav->Realize();
	
	wxPoint tbPoint = toolBarNav->GetPosition();
	wxSize tbSize = toolBarNav->GetSize();
	labelLeft = new wxStaticText(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT/* | wxST_NO_AUTORESIZE*/);
	labelRight = new wxStaticText(this, -1, wxT(""));
	wxFont font = labelLeft->GetFont();
	font.SetPointSize(10);
	labelLeft->SetFont(font);
	labelRight->SetFont(font);
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
	EVT_CLOSE(ComicalFrame::OnClose)
	EVT_SIZE(ComicalFrame::OnSize)
END_EVENT_TABLE()

void ComicalFrame::OnClose(wxCloseEvent& event)
{
	if (theBook) {
		theBook->Delete(); // delete the ComicBook thread
		delete theBook; // clear out the rest of the ComicBook
		theBook = NULL;
	}

	wxRect frameDim = GetRect();
	config->Write(wxT("/Comical/FrameWidth"), frameDim.width);
	config->Write(wxT("/Comical/FrameHeight"), frameDim.height);
	config->Write(wxT("/Comical/FrameX"), frameDim.x);
	config->Write(wxT("/Comical/FrameY"), frameDim.y);
	
	Destroy();	// Close the window
}

void ComicalFrame::OnQuit(wxCommandEvent& event)
{
	Close(TRUE);
}

void ComicalFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageDialog AboutDlg(this, wxT("Comical 0.7, (c) 2003-2005 James Athey.\nComical is licensed under the GPL, version 2,\nwith a linking exception; see README for details."), wxT("About Comical"), wxOK);
	AboutDlg.ShowModal();
}

void ComicalFrame::OnOpen(wxCommandEvent& event)
{
	wxString cwd;
	config->Read(wxT("/Comical/CWD"), &cwd);
	wxString filename = wxFileSelector(wxT("Open a Comic Book"), cwd, wxT(""), wxT(""), wxT("Comic Books (*.cbr,*.cbz,*.rar,*.zip)|*.cbr;*.CBR;*.cbz;*.CBZ;*.rar;*.RAR;*.zip;*.ZIP"), wxOPEN | wxCHANGE_DIR | wxFILE_MUST_EXIST, this);

	if (!filename.empty())
		OpenFile(filename);
}

void ComicalFrame::OnOpenDir(wxCommandEvent& event)
{
	wxString cwd;
	config->Read(wxT("/Comical/CWD"), &cwd);
	wxString dir = wxDirSelector(wxT("Open a Directory"), cwd, 0, wxDefaultPosition, this);

	if (!dir.empty())
		OpenDir(dir);
}

void ComicalFrame::OpenFile(wxString filename)
{
	if (!filename.empty()) {
	
		if (theBook) {
			theBook->Delete(); // delete the ComicBook thread
			delete theBook; // clear out the rest of the ComicBook
			theBook = NULL;
		}

		try {
			if (filename.Right(4).Upper() == wxT(".CBR") || filename.Right(4).Upper() == wxT(".RAR"))
				theBook = new ComicBookRAR(filename);
			else if (filename.Right(4).Upper() == wxT(".CBZ") || filename.Right(4).Upper() == wxT(".ZIP"))
				theBook = new ComicBookZIP(filename);

			if (theBook) {
				theCanvas->SetComicBook(theBook);
				theCanvas->SetParams();
				
				theBrowser->SetComicBook(theBook);
				theBrowser->SetItemCount(theBook->GetPageCount());

				toolBarNav->Enable(true);
	
				theBook->Run(); // start the thread

				theCanvas->FirstPage();
				theCanvas->Scroll(-1, 0); // scroll to the top for the first page
				SetTitle(wxT("Comical - " + filename));
				config->Write(wxT("/Comical/CWD"), wxPathOnly(filename));
			}
		} catch (ArchiveException &ae) {
			wxLogError(ae.Message);
			wxLog::FlushActive();
		}
	}
}

void ComicalFrame::OpenDir(wxString directory)
{
	if (!directory.empty()) {
	
		if (theBook) {
			theBook->Delete(); // delete the ComicBook thread
			delete theBook; // clear out the rest of the ComicBook
			theBook = NULL;
		}

		try {
			theBook = new ComicBookDir(directory);

			if (theBook) {
				theCanvas->SetComicBook(theBook);
				theCanvas->SetParams();

				theBrowser->SetComicBook(theBook);
				theBrowser->SetItemCount(theBook->GetPageCount());
				
				toolBarNav->Enable(true);
	
				theBook->Run(); // start the thread

				theCanvas->FirstPage();
				theCanvas->Scroll(-1, 0); // scroll to the top for the first page
				SetTitle(wxT("Comical - " + directory));
				config->Write(wxT("/Comical/CWD"), directory);
			}
		} catch (ArchiveException &ae) {
			wxLogError(ae.Message);
			wxLog::FlushActive();
		}
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
	if (toolBarNav)
		toolBarNav->Show(IsFullScreen()); // IsFullScreen will be the opposite in a moment...
	if (theBrowser)
		theBrowser->Show(IsFullScreen());
	ShowFullScreen(!(IsFullScreen()), wxFULLSCREEN_ALL);
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

wxSize ComicalFrame::setCanvasSize()
{
	wxSize clientSize = GetClientSize();
	if (!theCanvas)
		return clientSize;
	if (theBrowser != NULL && theBrowser->IsShown())
		clientSize.x -= theBrowser->GetSize().x;
	if (toolBarNav != NULL && toolBarNav->IsShown())
		clientSize.y -= toolBarNav->GetSize().y;
	theCanvas->SetSize(clientSize);
	theCanvas->SetParams();
	return clientSize;
}

void ComicalFrame::OnSize(wxSizeEvent &event)
{
	if (theCanvas) {
		setCanvasSize();
		if (toolBarNav && toolBarNav->IsShown()) {
			wxSize canvasSize = theCanvas->GetSize();
			wxSize clientSize = theCanvas->GetClientSize();
			wxSize toolBarSize = toolBarNav->GetSize();
			wxInt32 tbxPos = (clientSize.x - toolBarSize.x) / 2;
			toolBarNav->SetSize(tbxPos, canvasSize.y, toolBarSize.x, -1);
			if (labelLeft && labelLeft->IsShown())
				labelLeft->SetSize(tbxPos - 70, canvasSize.y + 6, 50, toolBarSize.y);
			if (labelRight && labelRight->IsShown())
				labelRight->SetSize(tbxPos + toolBarSize.x + 20, canvasSize.y + 6, 50, toolBarSize.y);		
		}
	}
}

void ComicalFrame::OnZoomBox(wxCommandEvent &event)
{
	if (theCanvas != NULL)
		theCanvas->SetZoomEnable(event.IsChecked());
}
