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

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
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
#endif

#include <vector>
#include <algorithm>

#include "Resize.h"

using namespace std;

enum COMICAL_MODE {SINGLE, DOUBLE};
enum COMICAL_ZOOM {FIT, FITV, FITH, FULL, THREEQ, HALF, ONEQ};
enum COMICAL_ROTATE {NORTH = 0, EAST, SOUTH, WEST};

class ComicBook : public wxThread {

public:

	// Constructors / Destructors
	ComicBook() : wxThread(wxTHREAD_JOINABLE) {}
	virtual ~ComicBook();
  
	// wxThread required functions
	virtual void * Entry();

	void RotatePage(uint pagenumber, COMICAL_ROTATE direction);
	void SetParams(COMICAL_MODE newMode, FREE_IMAGE_FILTER newFilter, COMICAL_ZOOM newZoom, uint newWidth, uint newHeight);
	void SetCacheLen(uint newCacheLen);
	wxBitmap *GetPage(uint pagenumber);
	vector<wxString> filenames;
	uint current;
	uint pagecount;
	COMICAL_ROTATE *Orientations;
	
protected:
	virtual wxInputStream * ExtractStream(uint pageindex) = 0;
	
	void ScaleImage(uint pagenumber);
	
	wxString filename;
	wxImage *Originals;
	wxImage *Resamples;
	wxMutex *imageProtectors; // only one thread can touch an image at a time
	
	uint cacheLen;
	
	// window parameters
	COMICAL_MODE mode;
	FREE_IMAGE_FILTER fiFilter;
	COMICAL_ZOOM zoom;
	uint width;
	uint height;
  
};

// extern ComicBook *theBook;

#endif
