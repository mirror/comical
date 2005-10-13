/***************************************************************************
              ComicalApp.cpp - ComicalApp class and gui functions
                             -------------------
    begin                : Wed Oct 22 2003
    copyright            : (C) 2003-2005 by James Athey
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
#ifndef __WXCOCOA__
#include "../Comical Icons/comical.xpm"  // the icon!
#endif
#endif

#include "../Comical Icons/firstpage.xpm"
#include "../Comical Icons/prevpage.xpm"
#include "../Comical Icons/prev.xpm"
#include "../Comical Icons/next.xpm"
#include "../Comical Icons/nextpage.xpm"
#include "../Comical Icons/lastpage.xpm"
#include "../Comical Icons/rot_cw.xpm"
#include "../Comical Icons/rot_ccw.xpm"

// Create a new application object.
IMPLEMENT_APP(ComicalApp)

bool ComicalApp::OnInit()
{
	wxImage::AddHandler(new wxJPEGHandler);
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxGIFHandler);
	wxImage::AddHandler(new wxTIFFHandler);
	
	wxConfig *config = new wxConfig(wxT("Comical"));
	
	wxInt32 width = (wxInt32) config->Read(wxT("/Comical/FrameWidth"), 600l);
	wxInt32 height = (wxInt32) config->Read(wxT("/Comical/FrameHeight"), 400l);
	wxInt32 x = (wxInt32) config->Read(wxT("/Comical/FrameX"), 50l);
	wxInt32 y = (wxInt32) config->Read(wxT("/Comical/FrameY"), 50l);
	
	ComicalFrame *frame = new ComicalFrame(wxT("Comical"), wxPoint(x, y), wxSize(width, height), wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE);

#ifndef __WXMAC__
#ifndef __WXCOCOA__
	frame->SetIcon(wxICON(Comical));
#endif
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
	toolBarNav = NULL;
	labelLeft = NULL;
	labelRight = NULL;
	
	ComicalLog = new wxLogStderr();
	ComicalLog->SetVerbose(FALSE);
	wxLog::SetActiveTarget(ComicalLog);
	
	config = new wxConfig(wxT("Comical"));
	wxConfigBase::Set(config); // Registers config globally	
	
	menuFile = new wxMenu;
	menuFile->Append(wxID_OPEN, wxT("&Open\tAlt-O"), wxT("Open a Comic Book."));
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

#ifndef __WXMAC__
	menuView->AppendSeparator();
	menuView->Append(ID_Full, wxT("Full &Screen\tAlt-Return"), wxT("Display Full Screen."));
#endif

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
	
	theCanvas = new ComicalCanvas(this, wxPoint(0,0), this->GetClientSize());

	toolBarNav = new wxToolBar(this, -1, wxPoint(0, this->GetClientSize().y + 10), wxDefaultSize, wxNO_BORDER | wxTB_HORIZONTAL | wxTB_FLAT);
	toolBarNav->SetToolBitmapSize(wxSize(16, 16));
	toolBarNav->AddTool(ID_CCWL, wxT("Rotate Counter-Clockwise (left page)"), wxBITMAP(rot_ccw), wxT("Rotate Counter-Clockwise (left page)"));
	toolBarNav->AddTool(ID_CWL, wxT("Rotate Clockwise (left page)"), wxBITMAP(rot_cw), wxT("Rotate Clockwise (left page)"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_First, wxT("First Page"), wxBITMAP(firstpage), wxT("First Page"));
	toolBarNav->AddTool(ID_PrevTurn, wxT("Previous Page Turn"), wxBITMAP(prevpage), wxT("Previous Page Turn"));
	toolBarNav->AddTool(ID_PrevSlide, wxT("Previous Page"), wxBITMAP(prev), wxT("Previous Page"));
	toolBarNav->AddTool(ID_NextSlide, wxT("Next Page"), wxBITMAP(next), wxT("Next Page"));
	toolBarNav->AddTool(ID_NextTurn, wxT("Next Page Turn"), wxBITMAP(nextpage), wxT("Next Page Turn"));
	toolBarNav->AddTool(ID_Last, wxT("Last Page"), wxBITMAP(lastpage), wxT("Last Page"));
	toolBarNav->AddSeparator();
	toolBarNav->AddTool(ID_CCW, wxT("Rotate Counter-Clockwise"), wxBITMAP(rot_ccw), wxT("Rotate Counter-Clockwise"));
	toolBarNav->AddTool(ID_CW, wxT("Rotate Clockwise"), wxBITMAP(rot_cw), wxT("Rotate Clockwise"));
	toolBarNav->Enable(false);
	toolBarNav->Fit();

	wxSize clientSize = GetClientSize();

	wxInt32 tbX = (clientSize.x - toolBarNav->GetSize().x) / 2;
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
	if (theBook)
	{
		theBook->Delete(); // delete the ComicBook thread
		theBook->Wait();
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
	wxMessageDialog AboutDlg(this, wxT("Comical 0.5, (c) 2003-2005 James Athey.\nComical is licensed under the GPL, version 2,\nwith a linking exception; see README for details."), wxT("About Comical"), wxOK);
	AboutDlg.ShowModal();
}

void ComicalFrame::OnOpen(wxCommandEvent& event)
{
	wxString cwd;
	config->Read(wxT("/Comical/CWD"), &cwd);
	wxString filename = wxFileSelector(wxT("Open a Comic Book"), cwd, wxT(""), wxT(""), wxT("Comic Books (*.cbr,*.cbz,*.rar,*.zip)|*.cbr;*.CBR;*.cbz;*.CBZ;*.rar;*.RAR;*.zip;*.ZIP"), wxOPEN | wxCHANGE_DIR | wxFILE_MUST_EXIST, this);

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

		if (filename.Right(4).Upper() == wxT(".CBR") || filename.Right(4).Upper() == wxT(".RAR"))
			theBook = new ComicBookRAR(filename, 10);
		else if (filename.Right(4).Upper() == wxT(".CBZ") || filename.Right(4).Upper() == wxT(".ZIP"))
			theBook = new ComicBookZIP(filename, 10);

		if (theBook)
		{
			theCanvas->theBook = theBook;
			theCanvas->SetParams();

			toolBarNav->Enable(true);
	
			theBook->Run(); // start the thread
	
			theCanvas->FirstPage();
			SetTitle(wxT("Comical - " + filename));
			config->Write(wxT("/Comical/CWD"), wxPathOnly(filename));
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
		message = wxT("Enter a page number from 1 to ");
		message += wxString::Format(wxT("%d"), theBook->GetPageCount());
		pagenumber = wxGetNumberFromUser(message, wxT("Page"), wxT("Go To Page"), theBook->Current + 1, 1, theBook->GetPageCount(), this);
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
		wxLogError(wxT("Zoom mode %d is undefined."), event.GetId()); // we shouldn't be here... honest!
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
		wxLogError(wxT("Filter %d is undefined."), event.GetId()); // we shouldn't be here... honest!
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
		wxLogError(wxT("I don't think I can turn that way: %d"), event.GetId()); // we shouldn't be here... honest!
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
		wxLogError(wxT("I don't think I can turn that way: %d"), event.GetId()); // we shouldn't be here... honest!
		break;
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

wxSize ComicalFrame::GetClientSize()
{
	wxSize clientSize = this->wxFrame::GetClientSize();
	wxInt32 y = clientSize.y;
	if (toolBarNav != NULL)
		y -= toolBarNav->GetSize().y;
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
