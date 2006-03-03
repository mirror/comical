/***************************************************************************
              ComicalApp.cpp - ComicalApp class and gui functions
                             -------------------
    begin                : Wed Oct 22 2003
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

#include "ComicalApp.h"
#include "ComicalFrame.h"
#include <wx/log.h>

#if !defined(__WXMAC__) && !defined(__WXCOCOA__) && !defined(__WXMSW__) && !defined(__WXPM__)
#include "../Comical Icons/comical.xpm"
#endif

// Create a new application object.
IMPLEMENT_APP(ComicalApp)

bool ComicalApp::OnInit()
{
//	wxLogStderr *log = new wxLogStderr();
//	wxLog::SetActiveTarget(log);
	
	wxImage::AddHandler(new wxJPEGHandler);
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxGIFHandler);
	
	wxConfig *config = new wxConfig(wxT("Comical"));
	wxConfigBase::Set(config); // Registers config globally

	if (config->HasGroup(wxT("Comical"))) // old versions of Comical had an extra path element
		config->DeleteGroup(wxT("Comical")); // this code will probably linger here forever, sadly, to catch upgraders
	
	wxInt32 width = (wxInt32) config->Read(wxT("FrameWidth"), 600l);
	wxInt32 height = (wxInt32) config->Read(wxT("FrameHeight"), 400l);
	wxInt32 x = (wxInt32) config->Read(wxT("FrameX"), 50l);
	wxInt32 y = (wxInt32) config->Read(wxT("FrameY"), 50l);
	
	ComicalFrame *frame = new ComicalFrame(wxT("Comical"), wxPoint(x, y), wxSize(width, height), wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE);
	docManager = new ComicalManager(frame);

#if !defined(__WXMAC__) && !defined(__WXCOCOA__)
	frame->SetIcon(wxICON(comical));
#endif

	frame->Show(true);
	SetTopWindow(frame);
	
#if !defined(__WXMAC__) && !defined(__WXCOCOA__)
	if (argc == 1)
		frame->OnOpen(*(new wxCommandEvent()));
	else if (wxFileExists(argv[1]))
		frame->OpenFile(wxString(argv[1]));
	else if (wxDirExists(argv[1]))
		frame->OpenDir(wxString(argv[1]));
	else {
		wxLogError(wxT("The file or directory \"") + wxString(argv[1]) + wxT("\" could not be found."));
		wxLog::FlushActive();
	}
#endif

	//delete log;
	
	return true;
}
