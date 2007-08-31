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
#include <wx/thread.h>
#include <wx/stream.h>
#include <wx/bitmap.h>

class ComicPage; // forward declaration, mutually dependent classes
WX_DECLARE_OBJARRAY(ComicPage, ArrayPage);

#include "enums.h"

class ComicPage {

public:

	ComicPage(wxString &filename);
	~ComicPage();
	void Rotate(COMICAL_ROTATE direction);
	bool ExtractDimensions(wxInputStream *stream);
	
	void DestroyAll();
	void DestroyOriginal();
	void DestroyResample();
	void DestroyThumbnail();
	
	wxBitmap *GetPage();
	wxBitmap *GetPageLeftHalf();
	wxBitmap *GetPageRightHalf();
	wxBitmap *GetThumbnail();

	bool IsLandscape();

	COMICAL_ROTATE GetOrientation() { return orientation; }
	
	wxString Filename;
	wxUint32 Width;
	wxUint32 Height;
	wxImage Original;
	wxImage Resample;
	wxImage Thumbnail;
	wxMutex *OriginalLock;
	wxMutex *ResampleLock;
	wxMutex *ThumbnailLock;

private:
	COMICAL_ROTATE orientation;
};

#endif
