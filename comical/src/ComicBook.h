/***************************************************************************
                ComicBook.h - ComicBook class and its children
                             -------------------
    begin                : Mon Sep 29 2003
    copyright            : (C) 2004 by James Athey
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

#ifndef _ComicBook_h_
#define _ComicBook_h_

#include <wx/bitmap.h>
#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/datetime.h>
#include <wx/utils.h>
#include <wx/stream.h>
#include <wx/thread.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/timer.h>

#include <vector>
#include <algorithm>

#include "Exceptions.h"
#include "Resize.h"

using namespace std;

enum COMICAL_MODE {ONEPAGE, TWOPAGE};
enum COMICAL_ZOOM {FIT, FITV, FITH, FULL, THREEQ, HALF, ONEQ};
enum COMICAL_ROTATE {NORTH = 0, EAST, SOUTH, WEST};

class ComicBook : public wxThread {

public:

	// Constructors / Destructors
	ComicBook(wxString file, wxUint32 cacheLength);
	virtual ~ComicBook();
  
	// wxThread required functions
	virtual void * Entry();

	void RotatePage(wxUint32 pagenumber, COMICAL_ROTATE direction);
	wxUint32 GetPageCount() { return pageCount; }
	bool SetParams(COMICAL_MODE newMode, FREE_IMAGE_FILTER newFilter, COMICAL_ZOOM newZoom, wxInt32 newWidth, wxInt32 newHeight, wxInt32 newScrollBarThickness);
	void SetCacheLen(wxUint32 newCacheLen);
	wxBitmap *GetPage(wxUint32 pagenumber);
	wxBitmap *GetPageLeftHalf(wxUint32 pagenumber);
	wxBitmap *GetPageRightHalf(wxUint32 pagenumber);
	bool IsPageLandscape(wxUint32 pagenumber);
	vector<wxString> Filenames;

	/* Used to prefetch nearby pages and discard distant pages. 
	 * when mode = TWOPAGE, Current is the pagenumber of the page on the right.
	 * when mode = ONEPAGE, Current is the pagenumber of the displayed page. */
	wxUint32 Current;

	COMICAL_ROTATE *Orientations;
	
protected:
	virtual wxInputStream * ExtractStream(wxUint32 pageindex) = 0;
	
	void ScaleImage(wxUint32 pagenumber);
	
	wxUint32 pageCount;
	wxString filename;
	wxImage *originals;
	wxImage *resamples;
	wxMutex *imageLockers;
	
	wxUint32 cacheLen;
	
	wxInt32 scrollBarThickness;
	
	// window parameters
	COMICAL_MODE mode;
	FREE_IMAGE_FILTER fiFilter;
	COMICAL_ZOOM zoom;
	wxInt32 canvasWidth;
	wxInt32 canvasHeight;

};

#endif
