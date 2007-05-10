//
// C++ Interface: ComicPage
//
// Description: 
//
//
// Author: James Leighton Athey <jathey@comcast.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _ComicPage_h_
#define _ComicPage_h_

#include <wx/string.h>
#include <wx/image.h>
#include <wx/dynarray.h>

class ComicPage; // forward declaration, mutually dependent classes
WX_DECLARE_OBJARRAY(ComicPage, ArrayPage);

#include "ComicBook.h"
#include "enums.h"

class ComicPage {

public:

	ComicPage(ComicBook *theBook, wxString &filename);
	~ComicPage();
	void Rotate(COMICAL_ROTATE direction);
	bool ExtractDimensions();
	
	wxString Filename;
	wxUint32 Width;
	wxUint32 Height;
	wxImage Original;
	wxImage Resample;
	wxImage Thumbnail;
	wxMutex OriginalLock;
	wxMutex ResampleLock;
	wxMutex ThumbnailLock;
	COMICAL_ROTATE Orientation;

private:
	ComicBook *book;

};

#endif
