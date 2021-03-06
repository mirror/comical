/*
 * ComicBook.h
 * Copyright (c) 2003-2011, James Athey
 */

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
#include <wx/stream.h>
#include <wx/thread.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/timer.h>
#include <wx/config.h>
#include <wx/arrstr.h>
#include <wx/event.h>

#include "Resize.h"
#include "enums.h"

#include "ComicPage.h"

#include <vector>

class ComicBook : public wxThread {

public:
	// Constructors / Destructors
	ComicBook(wxString filename, wxUint32 cacheLen, COMICAL_ZOOM, long zoomLevel, bool fitOnlyOversize, COMICAL_MODE, FREE_IMAGE_FILTER, COMICAL_DIRECTION, wxInt32 scrollbarThickness);
	virtual ~ComicBook();
	
	// wxThread required functions
	virtual void * Entry();

	void RotatePage(wxUint32 pagenumber, COMICAL_ROTATE direction);
	wxUint32 GetPageCount() const { return Pages.size(); }
	bool SetZoom(COMICAL_ZOOM zoom);
	bool SetZoomLevel(long zoomLevel);
	bool SetFitOnlyOversize(bool fitOnlyOversize);
	bool SetMode(COMICAL_MODE mode);
	bool SetFilter(FREE_IMAGE_FILTER filter);
	bool SetDirection(COMICAL_DIRECTION direction);
	bool SetCanvasSize(wxSize size);
	bool SetScrollbarThickness(wxInt32 scrollbarThickness);
	wxUint32 GetCacheLen() const { return cacheLen; }
	void SetCacheLen(wxUint32 newCacheLen) { cacheLen = newCacheLen; }
	wxBitmap& GetPage(wxUint32 pagenumber);
	wxBitmap& GetPageLeftHalf(wxUint32 pagenumber);
	wxBitmap& GetPageRightHalf(wxUint32 pagenumber);
	wxBitmap& GetThumbnail(wxUint32 pagenumber);
	COMICAL_ROTATE GetPageOrientation(wxUint32 pagenumber);
	bool IsPageLandscape(wxUint32 pagenumber);
	bool IsPageReady(wxUint32 pagenumber);
	bool IsThumbReady(wxUint32 pagenumber);
	wxInt32 GetThumbnailMaxWidth() const { return m_iThumbMaxWidth; }
	void SetThumbnailMaxWidth(wxInt32 maxWidthPixels);
	
	wxUint32 GetCurrentPage() const { return currentPage; }
	void SetCurrentPage(wxUint32 pagenumber);

	// Instead of insane multiple inheritance, put the evtHandler inside this
	// class and provide passthroughs to its methods
	wxEvtHandler * GetEventHandler() const { return evtHandler; }
	void Connect(int id, int lastId, wxEventType eventType, wxObjectEventFunction function, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ evtHandler->Connect(id, lastId, eventType, function, userData, eventSink); }
	void Connect(int id, wxEventType eventType, wxObjectEventFunction function, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ evtHandler->Connect(id, eventType, function, userData, eventSink); }
	void Connect(wxEventType eventType, wxObjectEventFunction function, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ evtHandler->Connect(eventType, function, userData, eventSink); }
	bool Disconnect(wxEventType eventType = wxEVT_NULL, wxObjectEventFunction function = NULL, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ return evtHandler->Disconnect(eventType, function, userData, eventSink); }
	bool Disconnect(int id = wxID_ANY, wxEventType eventType = wxEVT_NULL, wxObjectEventFunction function = NULL, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ return evtHandler->Disconnect(id, eventType, function, userData, eventSink); }
	bool Disconnect(int id, int lastId = wxID_ANY, wxEventType eventType = wxEVT_NULL, wxObjectEventFunction function = NULL, wxObject* userData = NULL, wxEvtHandler* eventSink = NULL)
		{ return evtHandler->Disconnect(id, lastId, eventType, function, userData, eventSink); }

	virtual wxInputStream * ExtractStream(wxUint32 pageindex) = 0;
	virtual wxInputStream * ExtractStream(wxString) = 0;

	const wxString filename;

protected:
	char* password;
	virtual bool TestPassword() { return true; }
	
	void SetPassword(const char* new_password);
	void postCtor();

	std::vector<ComicPage*> Pages;

private:
	void LoadOriginal(wxUint32 pagenumber);
	void ScaleImage(wxUint32 pagenumber);
	void ScaleThumbnail(wxUint32 pagenumber);
	// Returns true if the page can fit well in the current zoom mode, i.e., if
	// squeezing the page into the frame doesn't leave more than
	// scrollBarThickness pixels of black space on any side.  Returns the
	// scalingFactor to make the page fit without scrollbars in the parameter.
	bool FitWithoutScrollbars(wxUint32 pagenumber, float *scalingFactor);
	bool FitWithoutScrollbars(wxUint32 pagenumber);
	
	bool IsOversize(wxUint32 pagenumber);
	
	void SendScaledEvent(wxUint32 pagenumber);
	void SendThumbnailedEvent(wxUint32 pagenumber);
	void SendCurrentPageChangedEvent();
	void SendPageErrorEvent(wxUint32 pagenumber, wxString message);

	wxEvtHandler *evtHandler;

	/* Used to prefetch nearby pages and discard distant pages. 
	 * when mode = TWOPAGE, currentPage is the pagenumber of the page on the left.
	 * when mode = ONEPAGE, currentPage is the pagenumber of the displayed page. */
	wxUint32 currentPage;
	
	// window parameters
	wxUint32 cacheLen;
	COMICAL_ZOOM zoom;
	long zoomLevel;
	bool fitOnlyOversize;
	COMICAL_MODE mode;
	FREE_IMAGE_FILTER filter;
	COMICAL_DIRECTION direction;
	wxInt32 scrollbarThickness;
	
	wxInt32 canvasWidth;
	wxInt32 canvasHeight;

	wxInt32 m_iThumbMaxWidth;
};

DECLARE_EVENT_TYPE(EVT_PAGE_SCALED, -1)
DECLARE_EVENT_TYPE(EVT_PAGE_THUMBNAILED, -1)
DECLARE_EVENT_TYPE(EVT_CURRENT_PAGE_CHANGED, -1)
DECLARE_EVENT_TYPE(EVT_PAGE_ERROR, -1)

#endif
