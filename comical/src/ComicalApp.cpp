/***************************************************************************
              ComicalApp.cpp - ComicalApp class and gui functions
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

#include "ComicalApp.h"

#ifndef __WXMAC__
#include "Comical.xpm"  // the icon!
#endif

#define COMICAL_VERSION 0.4

// Create a new application object.
IMPLEMENT_APP(ComicalApp)

bool ComicalApp::OnInit()
{

  theBook = NULL;

  wxImage::AddHandler( new wxJPEGHandler );
  wxImage::AddHandler( new wxPNGHandler );
  wxImage::AddHandler( new wxGIFHandler );
  wxImage::AddHandler( new wxTIFFHandler );

  MainFrame *frame = new MainFrame(_T("Comical"), wxPoint(50, 50), wxSize(600, 400));

  frame->SetIcon(wxICON(Comical));
  frame->Show(TRUE);
  SetTopWindow(frame);
  theBook = NULL;

  if (argc == 1)
    frame->OnOpen();
  else {
    if (wxFileExists(argv[1])) {
      frame->OpenFile(wxString(argv[1]));
    }
  }

  return TRUE;

}

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(NULL, -1, title, pos, size, style)
{

  ComicalLog = new wxLogGui();
  ComicalLogWindow = new wxLogWindow(this, "Comical Log", false, true);
  ComicalLogWindow->SetVerbose();

  wxLogVerbose("Creating menus...");

  menuFile = new wxMenu;
  menuFile->Append(wxID_OPEN, _T("&Open\tAlt-O"), _T("Open a Comic Book."));
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT, _T("E&xit\tAlt-X"), _T("Quit Comical."));

  menuView = new wxMenu;
  menuView->Append(ID_PrevSlide, _T("Previous Page"), _T("Display the previous page."));
  menuView->Append(ID_NextSlide, _T("Next Page"), _T("Display the next page."));
  menuView->AppendSeparator();
  menuView->Append(ID_PrevTurn, _T("&Previous Page Turn"), _T("Display the previous two pages."));
  menuView->Append(ID_NextTurn, _T("&Next Page Turn"), _T("Display the next two pages."));
  menuView->AppendSeparator();
  menuView->Append(ID_First, _T("&First Page"), _T("Display the first page."));
  menuView->Append(ID_Last, _T("&Last Page"), _T("Display the last page."));
  menuView->Append(ID_GoTo, _T("&Go to page..."), _T("Jump to another page number."));
#ifndef __WXMAC__
  menuView->AppendSeparator();
  menuView->Append(ID_Full, _T("Full &Screen\tAlt-Return"), _T("Display Full Screen."));
#endif
  menuZoom = new wxMenu;          // a better place for this.
  menuZoom->AppendRadioItem(ID_ZFit, _T("Fit"), _T("Scale pages to fit the window."));
  menuZoom->AppendRadioItem(ID_ZFitV, _T("Fit to Height"), _T("Scale pages to fit the window's height."));
  menuZoom->AppendRadioItem(ID_ZFitH, _T("Fit to Width"), _T("Scale pages to fit the window's width."));
  menuZoom->AppendSeparator();
  menuZoom->AppendRadioItem(ID_ZFull, _T("100%"), _T("Original Size"));
  menuZoom->AppendRadioItem(ID_Z3Q, _T("75%"), _T("75% Zoom."));
  menuZoom->AppendRadioItem(ID_ZHalf, _T("50%"), _T("50% Zoom."));
  menuZoom->AppendRadioItem(ID_Z1Q, _T("25%"), _T("25% Zoom."));
  menuView->Append(ID_Z, _T("&Zoom"), menuZoom);

  menuMode = new wxMenu;
  menuMode->AppendRadioItem(ID_Double, _T("Double Page"), _T("Show two pages at a time."));
  menuMode->PrependRadioItem(ID_Single, _T("Single Page"), _T("Show only a single page at a time."));
  menuView->Append(ID_M, _T("&Mode"), menuMode);

  menuHelp = new wxMenu;
  menuHelp->Append(ID_ShowLog, _T("&Log Window\tF12"), _T("Show Log Window."));
  menuHelp->Append(wxID_ABOUT, _T("&About...\tF1"), _T("Show about dialog."));

  menuBar = new wxMenuBar();
  menuBar->Append(menuFile, _T("&File"));
  menuBar->Append(menuView, _T("&View"));
#ifndef __WXMAC__
  menuBar->Append(menuHelp, _T("&Help"));
#endif

  SetMenuBar(menuBar);

  wxLogVerbose("Creating the canvas...");
  theCanvas = new ComicalCanvas( this, -1, wxPoint(0,0), wxSize(10,10) );
  wxLogVerbose("done.");

}

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
  EVT_MENU(wxID_EXIT,	MainFrame::OnQuit)
  EVT_MENU(wxID_ABOUT,	MainFrame::OnAbout)
  EVT_MENU(wxID_OPEN,	MainFrame::OnOpen)
  EVT_MENU(ID_First,	MainFrame::OnFirst)
  EVT_MENU(ID_Last,	MainFrame::OnLast)
  EVT_MENU(ID_PrevTurn,	MainFrame::OnPrevTurn)
  EVT_MENU(ID_NextTurn,	MainFrame::OnNextTurn)
  EVT_MENU(ID_PrevSlide,	MainFrame::OnPrevSlide)
  EVT_MENU(ID_NextSlide,	MainFrame::OnNextSlide)
  EVT_MENU(ID_GoTo,	MainFrame::OnGoTo)
#ifndef __WXMAC__
  EVT_MENU(ID_Full,	MainFrame::OnFull)
#endif
  EVT_MENU(ID_ZFull,	MainFrame::OnZoom)
  EVT_MENU(ID_Z3Q,	MainFrame::OnZoom)
  EVT_MENU(ID_ZHalf,	MainFrame::OnZoom)
  EVT_MENU(ID_Z1Q,	MainFrame::OnZoom)
  EVT_MENU(ID_ZFit,	MainFrame::OnZoom)
  EVT_MENU(ID_ZFitH,	MainFrame::OnZoom)
  EVT_MENU(ID_ZFitV,	MainFrame::OnZoom)
  EVT_MENU(ID_Double,	MainFrame::OnDouble)
  EVT_MENU(ID_Single,	MainFrame::OnSingle)
  EVT_MENU(ID_ShowLog,	MainFrame::OnShowLog)
END_EVENT_TABLE()

void MainFrame::OnQuit(wxCommandEvent& event)
{
  Close(TRUE);
}

void MainFrame::OnAbout(wxCommandEvent& event)
{

  wxMessageDialog AboutDlg(this, "", _T("About Comical"), wxOK);
  AboutDlg.SetSize(500,330);
//  wxDialog AboutDlg(this, -1, _T("About Comical"), wxDefaultPosition, wxSize(500, 300), wxDEFAULT_DIALOG_STYLE, _T("AboutDlg"));
  wxTextCtrl *AboutTxt = new wxTextCtrl(&AboutDlg, -1, "", wxDefaultPosition, wxSize(500, 290), wxTE_READONLY | wxTE_MULTILINE, wxDefaultValidator, "AboutTxt");

  *AboutTxt	<< _T("Comical ") << COMICAL_VERSION << _T(" (c) 2003 by James Athey. Comical is licensed under the GPL, version 2.\n\n")
  		<< _T("This software uses the following libraries:\n\n")
  		<< _T("- the FreeImage_Rescale() function from the FreeImage open source image library. See http://freeimage.sourceforge.net/ for details. FreeImage is used under the GNU GPL, version 2.\n\n")
  		<< _T("- PStreams version 1.61. See http://pstreams.sourceforge.net/ for details. PStreams is licensed under the GNU LGPL, version 2.1.\n\n")
  		<< _T("- wxWindows version 2.4. See http://www.wxwindows.org/ for details. wxWindows is used under the wxWindows Library Licence, Version 3.");

  AboutDlg.ShowModal();

}

void MainFrame::OnOpen()
{

  wxString filename = wxFileSelector("Open a Comic Book", "", "", "",
  	"Comic Books (*.cbr,*.cbz,*.cbb,*.rar,*.zip,*.bz2)|*.cbr;*.CBR;*.cbz;*.CBZ;*.rar;*.RAR;*.zip;*.ZIP;*.cbb;*.CBB;*.bz2;*.BZ2",
	wxOPEN | wxCHANGE_DIR | wxFILE_MUST_EXIST, this);

  if (!filename.empty()) {
    OpenFile(filename);
  }

}

bool MainFrame::OpenFile(wxString filename) {

  if (!filename.empty()) {

    theCanvas->ClearBitmaps();
    theCanvas->ClearImages();

    if (theBook != NULL) {
      delete theBook; // clear out the old ComicBook object
      theBook = NULL;
    }

    if (filename.Right(4).Upper() == ".CBR" || filename.Right(4).Upper() == ".RAR")
      theBook = new ComicBookRAR(filename);
    else if (filename.Right(4).Upper() == ".CBZ" || filename.Right(4).Upper() == ".ZIP")
      theBook = new ComicBookZIP(filename);
    else if (filename.Right(4).Upper() == ".CBB" || filename.Right(4).Upper() == ".BZ2")
      theBook = new ComicBookBZ2(filename);

    OnFirst();
    SetTitle(_T("Comical - " + filename));

  }

  return true;

}

void MainFrame::OnFirst()
{
  if (theBook != NULL) theCanvas->FirstPage();
}

void MainFrame::OnLast()
{
  if (theBook != NULL) theCanvas->LastPage();
}

void MainFrame::OnPrevTurn()
{
  if (theBook != NULL) theCanvas->PrevPageTurn();
}

void MainFrame::OnNextTurn()
{
  if (theBook != NULL) theCanvas->NextPageTurn();
}

void MainFrame::OnPrevSlide()
{
  if (theBook != NULL) theCanvas->PrevPageSlide();
}

void MainFrame::OnNextSlide()
{
  if (theBook != NULL) theCanvas->NextPageSlide();
}

void MainFrame::OnGoTo()
{
	wxString message;
	long pagecount, pagenumber;
	if (theBook != NULL)
	{
		pagecount = theBook->pages.size() - 1;
		message = "Enter a page number from 0 to ";
		pagenumber = wxGetNumberFromUser(message, "Page", "Go To Page", 0, 0, pagecount, this);
		if (pagenumber != -1)
			theCanvas->GoToPage(pagenumber);
	}

}

void MainFrame::OnFull(wxCommandEvent& event)
{
  if (IsFullScreen()) wxLogVerbose("Leaving FullScreen.");
  else wxLogVerbose("Going FullScreen.");
  ShowFullScreen(!(IsFullScreen()), wxFULLSCREEN_ALL);
}

void MainFrame::OnZoom(wxCommandEvent& event)
{
  theCanvas->Scale(event.GetId());
}

void MainFrame::OnSingle(wxCommandEvent& event)
{
	wxMenuItem *prev = menuView->FindItemByPosition(3);
	wxMenuItem *next = menuView->FindItemByPosition(4);
	prev->Enable(false);
	next->Enable(false);
	theCanvas->Mode(SINGLE);
}

void MainFrame::OnDouble(wxCommandEvent& event)
{
	wxMenuItem *prev = menuView->FindItemByPosition(3);
	wxMenuItem *next = menuView->FindItemByPosition(4);
	prev->Enable(true);
	next->Enable(true);
	theCanvas->Mode(DOUBLE);
}

void MainFrame::OnShowLog()
{
	ComicalLogWindow->Show(true);
}
