//
// C++ Implementation: ComicalManager
//
// Description: 
//
//
// Author: James Leighton Athey <jathey@comcast.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "ComicalManager.h"

ComicalManager::ComicalManager(ComicalFrame *_frame) : wxDocManager(wxDEFAULT_DOCMAN_FLAGS, false), frame(_frame)
{
}

wxDocument* ComicalManager::CreateDocument(const wxString& filename, long flags)
{
	switch(flags) {
	case wxDOC_SILENT:
		frame->OpenFile(filename);
		break;
	case wxDOC_NEW:
	default:
		break;
	}
	
	return NULL;
}
