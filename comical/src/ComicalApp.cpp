/***************************************************************************
              ComicalApp.cpp - ComicalApp class and gui functions
                             -------------------
    begin                : Wed Oct 22 2003
    copyright            : (C) 2005 by James Athey
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

#include "ComicalApp.h"

#ifndef __WXMAC__
#include "../Comical Icons/comical.xpm"  // the icon!
#include "../Comical Icons/firstpage.xpm"
#include "../Comical Icons/prevpage.xpm"
#include "../Comical Icons/prev.xpm"
#include "../Comical Icons/next.xpm"
#include "../Comical Icons/nextpage.xpm"
#include "../Comical Icons/lastpage.xpm"
#include "../Comical Icons/rot_cw.xpm"
#include "../Comical Icons/rot_ccw.xpm"
#endif

// Create a new application object.
IMPLEMENT_APP(ComicalApp)

bool ComicalApp::OnInit()
{
	wxImage::AddHandler(new wxJPEGHandler);
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxGIFHandler);
	wxImage::AddHandler(new wxTIFFHandler);

	//wxFileSystem::AddHandler(new wxZipFSHandler);
	
	ComicalFrame *frame = new ComicalFrame(_T("Comical"), wxPoint(50, 50), wxSize(600, 400), wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE);

#ifndef __WXMAC__
	frame->SetIcon(wxICON(Comical));
#endif
	frame->Show(TRUE);
	SetTopWindow(frame);

	if (argc == 1)
		frame->OnOpen(*(new wxCommandEvent()));
	else
	{
		if (wxFileExists(argv[1]))
		{
			frame->OpenFile(wxString(argv[1]));
		}
	}

	return TRUE;
}

ComicalFrame::ComicalFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(NULL, -1, title, pos, size, style)
{
	theBook = NULL;

	ComicalLog = new wxLogStderr();
	ComicalLog->SetVerbose(FALSE);
	wxLog::SetActiveTarget(ComicalLog);
	
	config = new wxConfig("Comical");
	wxConfigBase::Set(config); // Registers config globally
	
	menuFile = new wxMenu;
	menuFile->Append(wxID_OPEN, _T("&Open\tAlt-O"), _T("Open a Comic Book."));
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT, _T("E&xit\tAlt-X"), _T("Quit Comical."));

	menuGo = new wxMenu;
	menuGo->Append(ID_PrevSlide, _T("Previous Page"), _T("Display the previous page."));
	menuGo->Append(ID_NextSlide, _T("Next Page"), _T("Display the next page."));
	menuGo->AppendSeparator();
	menuGo->Append(ID_PrevTurn, _T("&Previous Page Turn"), _T("Display the previous two pages."));
	menuGo->Append(ID_NextTurn, _T("&Next Page Turn"), _T("Display the next two pages."));
	menuGo->AppendSeparator();
	menuGo->Append(ID_First, _T("&First Page"), _T("Display the first page."));
	menuGo->Append(ID_Last, _T("&Last Page"), _T("Display the last page."));
	menuGo->Append(ID_GoTo, _T("&Go to page..."), _T("Jump to another page number."));

	menuView = new wxMenu;

	menuZoom = new wxMenu;
	menuZoom->AppendRadioItem(ID_Fit, _T("Fit"), _T("Scale pages to fit the window."));
	menuZoom->AppendRadioItem(ID_FitV, _T("Fit to Height"), _T("Scale pages to fit the window's height."));
	menuZoom->AppendRadioItem(ID_FitH, _T("Fit to Width"), _T("Scale pages to fit the window's width."));
	menuZoom->AppendSeparator();
	menuZoom->AppendRadioItem(ID_Unzoomed, _T("100%"), _T("Original Size"));
	menuZoom->AppendRadioItem(ID_3Q, _T("75%"), _T("75% Zoom."));
	menuZoom->AppendRadioItem(ID_Half, _T("50%"), _T("50% Zoom."));
	menuZoom->AppendRadioItem(ID_1Q, _T("25%"), _T("25% Zoom."));
	menuView->Append(ID_S, _T("&Zoom"), menuZoom);

	menuRotate = new wxMenu;
	menuRotate->AppendRadioItem(ID_North, _T("Normal"), _T("No rotation."));
	menuRotate->AppendRadioItem(ID_East, _T("90 Clockwise"), _T("Rotate 90 degrees clockwise."));
	menuRotate->AppendRadioItem(ID_South, _T("180"), _T("Rotate 180 degrees."));
	menuRotate->AppendRadioItem(ID_West, _T("90 Counter-Clockwise"), _T("Rotate 90 degrees counter-clockwise."));
	menuView->Append(ID_Rotate, _T("&Rotate"), menuRotate);

	menuRotateLeft = new wxMenu;
	menuRotateLeft->AppendRadioItem(ID_NorthLeft, _T("Normal"), _T("No rotation."));
	menuRotateLeft->AppendRadioItem(ID_EastLeft, _T("90 Clockwise"), _T("Rotate 90 degrees clockwise."));
	menuRotateLeft->AppendRadioItem(ID_SouthLeft, _T("180"), _T("Rotate 180 degrees."));
	menuRotateLeft->AppendRadioItem(ID_WestLeft, _T("90 Counter-Clockwise"), _T("Rotate 90 degrees counter-clockwise."));
	menuView->Append(ID_RotateLeft, _T("Rotate Left Page"), menuRotateLeft);

	menuRotateRight = new wxMenu;
	menuRotateRight->AppendRadioItem(ID_North, _T("Normal"), _T("No rotation."));
	menuRotateRight->AppendRadioItem(ID_East, _T("90 Clockwise"), _T("Rotate 90 degrees clockwise."));
	menuRotateRight->AppendRadioItem(ID_South, _T("180"), _T("Rotate 180 degrees."));
	menuRotateRight->AppendRadioItem(ID_West, _T("90 Counter-Clockwise"), _T("Rotate 90 degrees counter-clockwise."));
	menuView->Append(ID_RotateRight, _T("Rotate Ri&ght Page"), menuRotateRight);

	menuMode = new wxMenu;
	menuMode->AppendRadioItem(ID_Single, _T("Single Page"), _T("Show only a single page at a time."));
	menuMode->AppendRadioItem(ID_Double, _T("Double Page"), _T("Show two pages at a time."));
	menuView->Append(ID_M, _T("&Mode"), menuMode);

	menuFilter = new wxMenu;
	menuFilter->AppendRadioItem(ID_Box, _T("Box"), _T("Use the Box filter."));
	menuFilter->AppendRadioItem(ID_Bilinear, _T("Bilinear"), _T("Use the Bilinear filter."));
	menuFilter->AppendRadioItem(ID_Bicubic, _T("Bicubic"), _T("Use the Bicubic filter."));
	menuFilter->AppendRadioItem(ID_BSpline, _T("B-Spline"), _T("Use the B-Spline filter."));
	menuFilter->AppendRadioItem(ID_CatmullRom, _T("Catmull-Rom"), _T("Use the Catmull-Rom filter."));
	menuFilter->AppendRadioItem(ID_Lanczos, _T("Lanczos 3"), _T("Use the Box filter."));
	menuView->Append(ID_S, _T("&Image Filter"), menuFilter);

#ifndef __WXMAC__
	menuView->AppendSeparator();
	menuView->Append(ID_Full, _T("Full &Screen\tAlt-Return"), _T("Display Full Screen."));
#endif

	menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT, _T("&About...\tF1"), _T("Display About Dialog."));

	menuBar = new wxMenuBar();
	menuBar->Append(menuFile, _T("&File"));
	menuBar->Append(menuGo, _T("&Go"));
	menuBar->Append(menuView, _T("&View"));
	menuBar->Append(menuHelp, _T("&Help"));

	SetMenuBar(menuBar);

	// Each of the long values is followed by the letter L not the number one
	long Zoom = config->Read("/Comical/Zoom", 2l); // Fit-to-Width is default
	long Mode = config->Read("/Comical/Mode", 1l); // Double-Page is default
	long Filter = config->Read("/Comical/Filter", 4l); // Catmull-Rom is default

	// Record the settings from the config in the menus
	menuZoom->FindItemByPosition(Zoom)->Check(true);
	menuMode->FindItemByPosition(Mode)->Check(true);
	menuFilter->FindItemByPosition(Filter)->Check(true);
	
	theCanvas = new ComicalCanvas(this, wxPoint(0,0), this->GetClientSize());

	progress = new wxGauge(this, -1, 10, wxPoint(0, this->GetClientSize().y), wxSize(this->GetClientSize().x, 10));

	toolBarNav = new wxToolBar(this, -1, wxPoint(0, this->GetClientSize().y + 10), wxDefaultSize, wxNO_BORDER | wxTB_HORIZONTAL | wxTB_FLAT);
	toolBarNav->SetToolBitmapSize(wxSize(16, 16));
	toolBarNav->AddTool(ID_CCWL, _T("Rotate Counter-Clockwise (left page)"), wxBITMAP(rot_ccw), _T("Rotate Counter-Clockwise (left page)"));
	toolBarNav->AddTool(ID_CWL, _T("Rotate Clockwise (left page)"), wxBITMAP(rot_cw), _T("Rotate Clockwise (left page)"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_First, _T("First Page"), wxBITMAP(firstpage), _T("First Page"));
	toolBarNav->AddTool(ID_PrevTurn, _T("Previous Page Turn"), wxBITMAP(prevpage), _T("Previous Page Turn"));
	toolBarNav->AddTool(ID_PrevSlide, _T("Previous Page"), wxBITMAP(prev), _T("Previous Page"));
	toolBarNav->AddTool(ID_NextSlide, _T("Next Page"), wxBITMAP(next), _T("Next Page"));
	toolBarNav->AddTool(ID_NextTurn, _T("Next Page Turn"), wxBITMAP(nextpage), _T("Next Page Turn"));
	toolBarNav->AddTool(ID_Last, _T("Last Page"), wxBITMAP(lastpage), _T("Last Page"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_CCW, _T("Rotate Counter-Clockwise"), wxBITMAP(rot_ccw), _T("Rotate Counter-Clockwise"));
	toolBarNav->AddTool(ID_CW, _T("Rotate Clockwise"), wxBITMAP(rot_cw), _T("Rotate Clockwise"));
	toolBarNav->Enable(false);
	toolBarNav->Fit();

	wxSize clientSize = GetClientSize();

	int tbX = (clientSize.x - toolBarNav->GetSize().x) / 2;
	progress->SetSize(0, clientSize.y, clientSize.x, 10);
	toolBarNav->SetSize(tbX, clientSize.y + 10, toolBarNav->GetSize().x, -1);
	toolBarNav->Realize();
}

BEGIN_EVENT_TABLE(ComicalFrame, wxFrame)
	EVT_MENU(wxID_EXIT,	ComicalFrame::OnQuit)
	EVT_MENU(wxID_ABOUT,	ComicalFrame::OnAbout)
	EVT_MENU(wxID_OPEN,	ComicalFrame::OnOpen)
	EVT_MENU(ID_First,	ComicalFrame::OnFirst)
	EVT_MENU(ID_Last,	ComicalFrame::OnLast)
	EVT_MENU(ID_PrevTurn,	ComicalFrame::OnPrevTurn)
	EVT_MENU(ID_NextTurn,	ComicalFrame::OnNextTurn)
	EVT_MENU(ID_PrevSlide,	ComicalFrame::OnPrevSlide)
	EVT_MENU(ID_NextSlide,	ComicalFrame::OnNextSlide)
	EVT_MENU(ID_GoTo,	ComicalFrame::OnGoTo)
#ifndef __WXMAC__
	EVT_MENU(ID_Full,	ComicalFrame::OnFull)
#endif
	EVT_MENU(ID_Double,	ComicalFrame::OnDouble)
	EVT_MENU(ID_Single,	ComicalFrame::OnSingle)
	EVT_MENU_RANGE(ID_Unzoomed, ID_FitH, ComicalFrame::OnZoom)
	EVT_MENU_RANGE(ID_Box, ID_Lanczos, ComicalFrame::OnFilter)
	EVT_MENU_RANGE(ID_CW, ID_CCW, ComicalFrame::OnRotate)
	EVT_MENU_RANGE(ID_CWL, ID_CCWL, ComicalFrame::OnRotateLeft)
	EVT_MENU_RANGE(ID_North, ID_West, ComicalFrame::OnRotate)
	EVT_MENU_RANGE(ID_NorthLeft, ID_WestLeft, ComicalFrame::OnRotateLeft)
	EVT_CLOSE(ComicalFrame::OnClose)
	EVT_SIZE(ComicalFrame::OnSize)
END_EVENT_TABLE()

void ComicalFrame::OnClose(wxCloseEvent& event)
{
	Destroy();	// Close the window
}

void ComicalFrame::OnQuit(wxCommandEvent& event)
{
	if (theBook)
	{
		theBook->Delete(); // delete the ComicBook thread
		theBook->Wait();
		delete theBook; // clear out the rest of the ComicBook
		theBook = NULL;
	}
	Close(TRUE);
}

void ComicalFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageDialog AboutDlg(this, "Comical 0.5, (c) 2003-2005 James Athey.\nComical is licensed under the GPL, version 2,\nwith a linking exception; see COPYING for details.", _T("About Comical"), wxOK);
	AboutDlg.ShowModal();
}

void ComicalFrame::OnOpen(wxCommandEvent& event)
{
	wxString cwd;
	config->Read("/Comical/CWD", &cwd);
	wxString filename = wxFileSelector("Open a Comic Book", cwd, "", "", "Comic Books (*.cbr,*.cbz,*.rar,*.zip)|*.cbr;*.CBR;*.cbz;*.CBZ;*.rar;*.RAR;*.zip;*.ZIP", wxOPEN | wxCHANGE_DIR | wxFILE_MUST_EXIST, this);

	if (!filename.empty())
	{
		OpenFile(filename);
	}

}

void ComicalFrame::OpenFile(wxString filename)
{
	
	if (!filename.empty())
	{
	
		if (theBook)
		{
			theBook->Delete(); // delete the ComicBook thread
			theBook->Wait();
			delete theBook; // clear out the rest of the ComicBook
			theBook = NULL;
		}

		if (filename.Right(4).Upper() == ".CBR" || filename.Right(4).Upper() == ".RAR")
			theBook = new ComicBookRAR(filename, 10);
		else if (filename.Right(4).Upper() == ".CBZ" || filename.Right(4).Upper() == ".ZIP")
			theBook = new ComicBookZIP(filename, 10);

		if (theBook)
		{
			theCanvas->theBook = theBook;
			theCanvas->SetParams();

			toolBarNav->Enable(true);
			progress->SetRange(theBook->GetPageCount() - 1);
			progress->SetValue(0);
	
			theBook->Run(); // start the thread
	
			OnFirst(*(new wxCommandEvent()));
			SetTitle(_T("Comical - " + filename));
			config->Write("/Comical/CWD", wxPathOnly(filename));
		}
	}
}

void ComicalFrame::OnFirst(wxCommandEvent& event)
{
	if (theBook != NULL) theCanvas->FirstPage();
}

void ComicalFrame::OnLast(wxCommandEvent& event)
{
	if (theBook != NULL) theCanvas->LastPage();
}

void ComicalFrame::OnPrevTurn(wxCommandEvent& event)
{
	if (theBook != NULL) theCanvas->PrevPageTurn();
}

void ComicalFrame::OnNextTurn(wxCommandEvent& event)
{
	if (theBook != NULL) theCanvas->NextPageTurn();
}

void ComicalFrame::OnPrevSlide(wxCommandEvent& event)
{
	if (theBook != NULL) theCanvas->PrevPageSlide();
}

void ComicalFrame::OnNextSlide(wxCommandEvent& event)
{
	if (theBook != NULL) theCanvas->NextPageSlide();
}

void ComicalFrame::OnGoTo(wxCommandEvent& event)
{
	wxString message ;
	long pagenumber;
	if (theBook != NULL)
	{
		message = "Enter a page number from 1 to ";
		message += wxString::Format("%d", theBook->GetPageCount());
		pagenumber = wxGetNumberFromUser(message, "Page", "Go To Page", theBook->Current + 1, 1, theBook->GetPageCount(), this);
		if (pagenumber != -1)
			theCanvas->GoToPage(pagenumber - 1);
	}

}

void ComicalFrame::OnFull(wxCommandEvent& event)
{
	ShowFullScreen(!(IsFullScreen()), wxFULLSCREEN_ALL);
}

void ComicalFrame::OnZoom(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case ID_Unzoomed:
		theCanvas->Zoom(FULL);
		break;
	case ID_3Q:
		theCanvas->Zoom(THREEQ);
		break;
	case ID_Half:
		theCanvas->Zoom(HALF);
		break;
	case ID_1Q:
		theCanvas->Zoom(ONEQ);
		break;
	case ID_Fit:
		theCanvas->Zoom(FIT);
		break;
	case ID_FitV:
		theCanvas->Zoom(FITV);
		break;
	case ID_FitH:
		theCanvas->Zoom(FITH);
		break;
	default:
		wxLogError("Zoom mode %d is undefined.", event.GetId()); // we shouldn't be here... honest!
		break;
	}
}

void ComicalFrame::OnFilter(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case ID_Box:
		theCanvas->Filter(FILTER_BOX);
		break;
	case ID_Bilinear:
		theCanvas->Filter(FILTER_BILINEAR);
		break;
	case ID_Bicubic:
		theCanvas->Filter(FILTER_BICUBIC);
		break;
	case ID_BSpline:
		theCanvas->Filter(FILTER_BSPLINE);
		break;
	case ID_CatmullRom:
		theCanvas->Filter(FILTER_CATMULLROM);
		break;
	case ID_Lanczos:
		theCanvas->Filter(FILTER_LANCZOS3);
		break;
	default:
		wxLogError("Filter %d is undefined.", event.GetId()); // we shouldn't be here... honest!
		break;
	}
}

void ComicalFrame::OnRotate(wxCommandEvent& event)
{
	switch (event.GetId()) {
	case ID_CW:
		theCanvas->Rotate(true);
		break;
	case ID_CCW:
		theCanvas->Rotate(false);
		break;
	case ID_North:
		theCanvas->Rotate(NORTH);
		break;
	case ID_East:
		theCanvas->Rotate(EAST);
		break;
	case ID_South:
		theCanvas->Rotate(SOUTH);
		break;
	case ID_West:
		theCanvas->Rotate(WEST);
		break;
	default:
		wxLogError("I don't think I can turn that way: %d", event.GetId()); // we shouldn't be here... honest!
		break;
	}
}

void ComicalFrame::OnRotateLeft(wxCommandEvent& event)
{
	switch (event.GetId()) {
	case ID_CWL:
		theCanvas->RotateLeft(true);
		break;
	case ID_CCWL:
		theCanvas->RotateLeft(false);
		break;
	case ID_NorthLeft:
		theCanvas->RotateLeft(NORTH);
		break;
	case ID_EastLeft:
		theCanvas->RotateLeft(EAST);
		break;
	case ID_SouthLeft:
		theCanvas->RotateLeft(SOUTH);
		break;
	case ID_WestLeft:
		theCanvas->RotateLeft(WEST);
		break;
	default:
		wxLogError("I don't think I can turn that way: %d", event.GetId()); // we shouldn't be here... honest!
		break;
	}
}

void ComicalFrame::OnSingle(wxCommandEvent& event)
{
	wxMenuItem *prev = menuGo->FindItem(ID_PrevTurn);
	wxMenuItem *next = menuGo->FindItem(ID_NextTurn);
	prev->Enable(false);
	next->Enable(false);
	theCanvas->Mode(SINGLE);
}

void ComicalFrame::OnDouble(wxCommandEvent& event)
{
	wxMenuItem *prev = menuGo->FindItem(ID_PrevTurn);
	wxMenuItem *next = menuGo->FindItem(ID_NextTurn);
	prev->Enable(true);
	next->Enable(true);
	theCanvas->Mode(DOUBLE);
}

wxSize ComicalFrame::GetClientSize()
{
	wxSize clientSize = this->wxFrame::GetClientSize();
	int y = clientSize.y;
	if (toolBarNav != NULL)
		y -= toolBarNav->GetSize().y;
	if (progress != NULL)
		y -= progress->GetSize().y;
	clientSize.SetHeight(y);
	return clientSize;
}

void ComicalFrame::OnSize(wxSizeEvent &event)
{
	if (theCanvas != NULL)
	{
		wxSize clientSize = GetClientSize();
		theCanvas->SetSize(clientSize);
	}
}
