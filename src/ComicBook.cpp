/*
 * ComicBook.cpp
 * Copyright (c) 2003-2011, James Athey. 2012, John Peterson.
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

#include <wx/datetime.h>
#include <wx/mstream.h>
#include <wx/utils.h>
#include <wx/textdlg.h>

#include <algorithm>
#include <cstring>

#include "ComicBook.h"
#include "ComicalFrame.h"
#include "enums.h"
#include "Exceptions.h"

DEFINE_EVENT_TYPE(EVT_PAGE_SCALED)
DEFINE_EVENT_TYPE(EVT_PAGE_THUMBNAILED)
DEFINE_EVENT_TYPE(EVT_CURRENT_PAGE_CHANGED)
DEFINE_EVENT_TYPE(EVT_CUSTOM_EVENT)

ComicBook::ComicBook(ComicalFrame *_parent, wxString _filename, wxUint32 _cacheLen, COMICAL_ZOOM _zoom, long _zoomLevel, bool _fitOnlyOversize, COMICAL_MODE _mode, FREE_IMAGE_FILTER _filter, COMICAL_DIRECTION _direction):
wxThread(wxTHREAD_JOINABLE),
bookReady(false),
parent(_parent),
filename(_filename),
password(_("")),
evtHandler(new wxEvtHandler()),
currentPage(0),
cacheLen(_cacheLen),
zoom(_zoom),
zoomLevel(_zoomLevel),
fitOnlyOversize(_fitOnlyOversize),
mode(_mode),
filter(_filter),
direction(_direction)
{
	if (!cacheLen) cacheLen = 3;
}


ComicBook::~ComicBook()
{
	for (unsigned i = 0; i < Pages.size(); i++)
		delete Pages[i];

	delete evtHandler;
}

void ComicBook::AddPage(ComicPage *page)
{
	Pages.push_back(page);
	SendCustomEvent(ID_PageAdded);
}

void ComicBook::RotatePage(wxUint32 pagenumber, COMICAL_ROTATE direction)
{
	Pages.at(pagenumber)->Rotate(direction);
}

wxUint32 ComicBook::GetPageTotalCount() const
{
	wxUint32 count = 0;
	for (wxUint32 i = 0; i < Pages.size(); i++)
		count += Pages.at(i)->IsLandscape() ? 2 : 1;
	return count;
}

wxRect ComicBook::DoGetSize(wxInt32 pagenumber, wxInt32 pos)
{
	wxSize size;
	float x, y, xr, yr, r;
	
	if (pagenumber < 0) pagenumber = GetPageCount()  - 1; 
	
	for (wxInt32 i = 0; i <= pagenumber; i++) {		
		ComicPage *page = Pages.at(i);
		
		if (page->GetOrientation() == NORTH || page->GetOrientation() == SOUTH) {
			x = page->GetWidth(); y = page->GetHeight();
		} else {
			x = page->GetHeight(); y = page->GetWidth();
		}

		xr = (parent->canvasSize.x / GetPageCount()) / x;
		yr = parent->canvasSize.y / y;
		
		switch (zoom) {
		case ZOOM_FIT:			
			r = wxMin(xr, yr);
			break;
		case ZOOM_HEIGHT: r = yr; break;
		case ZOOM_WIDTH: r = xr; break;
		case ZOOM_CUSTOM: r = zoomLevel / 100.0f; break;
		case ZOOM_FULL: r = 1; break;
		}
			
		x *= r;
		y *= r;
		
		size.x += x;
		if (y > size.y) size.y = y;
		
		if (pos > -1 && (size.x > pos || i == pagenumber)) {
			return wxRect(i, -1, -1, -1);
		}
	}

	return wxRect(size.x, size.y, x, y);
}

wxSize ComicBook::GetSize()
{
	wxRect rect = DoGetSize();
	return wxSize(rect.x, rect.y);
}

wxUint32 ComicBook::GetPageNum(wxInt32 pos)
{	
	return DoGetSize(-1, pos).x;
}

wxUint32 ComicBook::GetPagePos(wxInt32 pagenumber)
{
	if (pagenumber < 0) pagenumber = 0;
	if (pagenumber >= GetPageCount()) pagenumber = GetPageCount() - 1;

	wxRect rect = DoGetSize(pagenumber);
	return rect.x - rect.width;
}

wxSize ComicBook::GetPageSize(wxInt32 pagenumber)
{
	if (pagenumber < 0) pagenumber = 0;
	if (pagenumber >= GetPageCount()) pagenumber = GetPageCount() - 1;

	wxRect rect = DoGetSize(pagenumber);
	return wxSize(rect.width, rect.height);
}

wxSize ComicBook::GetThumbnailSize(wxInt32 pagenumber)
{
	wxSize scaled, size = GetComicPageSize(pagenumber);
	float scalingFactor;
	if (float(size.y) / float(size.x) > 0.6f) {
		scaled.y = thumbMaxWidth * 0.6f;
		scalingFactor = scaled.y / float(size.y);
		scaled.x = wxInt32(float(size.x) * scalingFactor);
	} else {
		scaled.x = thumbMaxWidth;
		scalingFactor = float(scaled.x) / float(size.x);
		scaled.y = wxInt32(float(size.y) * scalingFactor);
	}
	return scaled;
}

/* returns TRUE if newMode is different, FALSE if the same and no changes were made */
bool ComicBook::SetMode(COMICAL_MODE newMode)
{
	if(mode != newMode) {
		for (wxUint32 i = 0; i < Pages.size(); i++) {
			Pages.at(i)->DestroyResample();
		}
		mode = newMode;
		return TRUE;
	} else
		return FALSE;
}

/* returns TRUE if newFilter is different, FALSE if the same and no changes were made */
bool ComicBook::SetFilter(FREE_IMAGE_FILTER newFilter)
{
	if(filter != newFilter) {
		for (wxUint32 i = 0; i < Pages.size(); i++) {
			Pages.at(i)->DestroyResample();
		}
		filter = newFilter;
		return TRUE;
	} else
		return FALSE;
}

/* Makes pages zoom to newZoom level, and will zoom pages that are smaller than the canvas only if newFitOnlyIfOversize is FALSE.
 * returns TRUE if parameters were different, FALSE if parameters were the same and no changes were made */
bool ComicBook::SetZoom(COMICAL_ZOOM newZoom)
{
	if(zoom != newZoom) {
		for (wxUint32 i = 0; i < Pages.size(); i++) {
			Pages.at(i)->DestroyResample();
		}
		zoom = newZoom;
		return TRUE;
	} else
		return FALSE;
}

/* Makes pages zoom to newZoomLevel%.
 * returns TRUE if parameters were different, FALSE if parameters were the same and no changes were made */
bool ComicBook::SetZoomLevel(long newZoomLevel)
{
	if(zoomLevel != newZoomLevel) {
		if (zoom == ZOOM_CUSTOM) {
			for (wxUint32 i = 0; i < Pages.size(); i++) {
				Pages.at(i)->DestroyResample();
			}
		}
		zoomLevel = newZoomLevel;
		return TRUE;
	} else
		return FALSE;
}

bool ComicBook::SetFitOnlyOversize(bool newFitOnlyOversize)
{
	if (fitOnlyOversize != newFitOnlyOversize) {
		for (wxUint32 i = 0; i < Pages.size(); i++) {
			Pages.at(i)->DestroyResample();
		}
		fitOnlyOversize = newFitOnlyOversize;
		return TRUE;
	} else
		return FALSE;
}

/* returns TRUE if direction is different, FALSE if same and no changes were made */
bool ComicBook::SetDirection(COMICAL_DIRECTION newDirection)
{
	if(direction != newDirection) {
		direction = newDirection;
		return TRUE;
	} else
		return FALSE;
}

void ComicBook::SetThumbnailMaxWidth(wxInt32 maxWidthPixels)
{
	if (thumbMaxWidth == maxWidthPixels)
		return;

	thumbMaxWidth = maxWidthPixels;
	for (wxUint32 i = 0; i < Pages.size(); i++) {
		Pages.at(i)->DestroyThumbnail();
	}
}

void ComicBook::SetReady()
{
	bookReady = true;
}

ComicPage* ComicBook::GetComicPage(wxUint32 pagenumber)
{
	return Pages.at(pagenumber);
}

wxSize ComicBook::GetComicPageSize(wxUint32 pagenumber)
{
	ComicPage *page = Pages.at(pagenumber);
	wxSize size;
	if (page->GetOrientation() == NORTH || page->GetOrientation() == SOUTH) {
		size.x = page->GetWidth();
		size.y = page->GetHeight();
	} else {// EAST || WEST
		size.y = page->GetWidth();
		size.x = page->GetHeight();
	}
	return size;
}

wxBitmap& ComicBook::GetPage(wxUint32 pagenumber)
{
	return Pages.at(pagenumber)->GetPage();
}

wxBitmap& ComicBook::GetPageLeftHalf(wxUint32 pagenumber)
{
	if (direction == COMICAL_LTR)
		return Pages.at(pagenumber)->GetPageLeftHalf();
	else
		return Pages.at(pagenumber)->GetPageRightHalf();
}

wxBitmap& ComicBook::GetPageRightHalf(wxUint32 pagenumber)
{
	if (direction == COMICAL_LTR)
		return Pages.at(pagenumber)->GetPageRightHalf();
	else
		return Pages.at(pagenumber)->GetPageLeftHalf();
}

wxBitmap& ComicBook::GetThumbnail(wxUint32 pagenumber)
{
	return Pages.at(pagenumber)->GetThumbnail();
}

COMICAL_ROTATE ComicBook::GetPageOrientation(wxUint32 pagenumber)
{
	return Pages.at(pagenumber)->GetOrientation();
}

bool ComicBook::IsPageLandscape(wxUint32 pagenumber)
{
	return Pages.at(pagenumber)->IsLandscape();
}

bool ComicBook::IsReady()
{
	return bookReady;
}

bool ComicBook::IsPageReady(wxUint32 pagenumber)
{
	return Pages.at(pagenumber)->Resample.Ok();
}

bool ComicBook::IsThumbReady(wxUint32 pagenumber)
{
	return Pages.at(pagenumber)->Thumbnail.Ok();
}

bool ComicBook::IsFit(wxUint32 pagenumber, float &scalingFactor)
{
	wxSize size;
	wxInt32 usableWidth;

	ComicPage *page = Pages.at(pagenumber);

	if (page->GetOrientation() == NORTH || page->GetOrientation() == SOUTH)		
		size = wxSize(page->GetWidth(), page->GetHeight());
	else
		size = wxSize(page->GetHeight(), page->GetWidth());

	switch(zoom) {
	case ZOOM_WIDTH:	
		switch (mode) {
		case ONEPAGE:
			usableWidth = parent->canvasSize.x;
			break;
			
		case TWOPAGE:
			if (IsPageLandscape(pagenumber))
				usableWidth = parent->canvasSize.x;
			else
				usableWidth = parent->canvasSize.x / 2;
			break;
			
		case CONTINUOUS:
				usableWidth = parent->canvasSize.x / GetPageCount();
			break;
		}

		scalingFactor = float(usableWidth) / float(size.x);
		return true;
		
	case ZOOM_HEIGHT:	
		scalingFactor = float(parent->canvasSize.y) / float(size.y);
		return true;
		
	case ZOOM_FIT:
		return true;
		
	default:
		// this function should not be called for other zooms
		break;
	}
	return false;
}

bool ComicBook::IsFit(wxUint32 pagenumber)
{
	float throwaway = 0.0f;
	return IsFit(pagenumber, throwaway);
}

bool ComicBook::IsOversize(wxUint32 pagenumber)
{
	wxSize size;
	wxInt32 usableWidth, usableHeight;

	ComicPage *page = Pages.at(pagenumber);

	if (page->GetOrientation() == NORTH || page->GetOrientation() == SOUTH)		
		size = wxSize(page->GetWidth(), page->GetHeight());
	else
		size = wxSize(page->GetHeight(), page->GetWidth());

	float ratio = float (size.x) / float(size.y);
	
	switch(zoom) {
	
	case ZOOM_FIT:
		usableHeight = parent->canvasSize.y;
		switch (mode) {
		case ONEPAGE:
			usableWidth = parent->canvasSize.x;
			break;
			
		case TWOPAGE:
			if (page->IsLandscape())
				usableWidth = parent->canvasSize.x;
			else
				usableWidth = parent->canvasSize.x / 2;
			break;
			
		case CONTINUOUS:
			usableWidth = parent->canvasSize.x / GetPageCount();
			break;
		}
		break;
		
	case ZOOM_WIDTH:
		usableHeight = parent->canvasSize.y;
		// For now, to be safe, assume vertical scrollbar is present thanks to neighboring page.
		switch (mode) {
		case ONEPAGE:
			usableWidth = parent->canvasSize.x;
			break;
			
		case TWOPAGE:
			if (page->IsLandscape())
				usableWidth = parent->canvasSize.x;
			else
				usableWidth = parent->canvasSize.x / 2;
			break;
			
		case CONTINUOUS:
			usableWidth = parent->canvasSize.x / GetPageCount();
			break;
		}
		break;
	
	case ZOOM_HEIGHT:
		usableWidth = parent->canvasSize.x;
		usableHeight = parent->canvasSize.y;
		break;
	
	default:
		return true;
	}
	
	if (size.x > usableWidth || size.y > usableHeight)
		return true;
	else
		return false;
}

void * ComicBook::Entry()
{
	wxUint32 i, target, high, curr, count;
	wxInt32 low;
	bool scalingHappened;
	ComicPage *page;
	
	while (!TestDestroy()) {
		scalingHappened = false;
		curr = GetCurrentPage(); // in case this value changes midloop
		count = GetPageCount();

		// The caching algorithm.  First calculates next highest
		// priority page, then checks to see if that page needs
		// updating.  If no pages need updating, yield the timeslice.
		
		if (cacheLen >= count) {
			low = 0;
			high = count - 1;
		} else {
			/* A moving window of size cacheLen.  2/3 of the pages in the
			 * cache are after the Current page, the other 1/3 are before
			 * it. */
			high = 2 * cacheLen / 3;
			low = cacheLen - high;
			
			high = curr + high - 1;
			low = curr - low;

			/* Keep the window within 0 and Pages.size(). */
			if (high >= count) {
				low -= (high - count) + 1;
				high = count - 1;
			} else if (low < 0) {
				high -= low; // low is negative, this is an addition
				low = 0;
			}
		}

		for (i = 0; i < cacheLen && i < count; i++) {
			if (TestDestroy())
				goto thread_end;
			target = curr + i;
			if (target > high)
				target = curr - (target - high);			
			
			page = Pages.at(target);
			page->ResampleLock.Lock();
			if (page->Resample.Ok()) {
				page->ResampleLock.Unlock();
				if (page->Thumbnail.Ok()) continue;
			}
			
			page->OriginalLock.Lock();
			try {
				if (!page->Original.Ok())
					LoadOriginal(target);
			} catch (ArchiveException *ae) {
				SendCustomEvent(ID_Error, ae->Message);
			}
			
			ScaleImage(target);
			scalingHappened = true;
			
			page->ResampleLock.Unlock();

			page->ThumbnailLock.Lock();
			if (!page->Thumbnail.Ok()) {
				ScaleThumbnail(target);
				if (!page->Thumbnail.Ok()) { // let's only see one error if things go wrong
					SendCustomEvent(ID_Error, wxString::Format(wxT("Could not create thumbnail for page %d."), target));
				}
			}
			page->ThumbnailLock.Unlock();
			page->OriginalLock.Unlock();
			if (page->Thumbnail.Ok()) page->DestroyOriginal();
			break;
		}
		
		if (!scalingHappened) {
			// if the cache is full, then use this iteration to fetch a
			// thumbnail, if needed.
			for (wxUint32 j = 0; j < count; j++) {
				page = Pages.at(j);
				
				page->ThumbnailLock.Lock();
				if (page->Thumbnail.Ok()) {
					page->ThumbnailLock.Unlock();
					continue;
				}
				
				page->OriginalLock.Lock();
				try {
					if (!page->Original.Ok())
						LoadOriginal(j);
				} catch (ArchiveException *ae) {
					SendCustomEvent(ID_Error, ae->Message);
				}
				
				ScaleThumbnail(j);
				page->ThumbnailLock.Unlock();
				page->OriginalLock.Unlock();

				page->DestroyOriginal();
				break;
			}
		}

		if (i < cacheLen && i < count) {
			if (cacheLen < count) {
				// Delete pages outside of the cache's range.
				for (i = 0; wxInt32(i) < low; i++) {
					page = Pages.at(i);
					page->DestroyResample();
				}
				
				for (i = count - 1; i > high; i--) {

					page = Pages.at(i);
					page->DestroyResample();
				}
			}
		}

		Yield();
		Sleep(20);
	}
	thread_end:
	return 0;
}

void ComicBook::LoadOriginal(wxUint32 pagenumber)
{
	ComicPage *page = Pages.at(pagenumber);
	wxInputStream *stream = ExtractStream(pagenumber);
	if (stream->IsOk() && stream->GetSize() > 0) {
		page->Original.LoadFile(*stream, page->GetBitmapType());
		// Memory Input Streams don't take ownership of the buffer
		wxMemoryInputStream *mstream = dynamic_cast<wxMemoryInputStream*>(stream);
		if (mstream)
			delete[] (wxUint8 *) mstream->GetInputStreamBuffer()->GetBufferStart();
	} else {
		SendCustomEvent(ID_Error, wxString::Format(wxT("Failed to extract page %d."), pagenumber));
		page->Original = wxImage(1, 1);
	}
	if (!page->Original.Ok()) {
		SendCustomEvent(ID_Error, wxString::Format(wxT("Failed to extract page %d."), pagenumber));
		page->Original = wxImage(1, 1);
	}
	wxDELETE(stream);
	// TEMPORARY TODO
	if (page->Original.GetWidth() != page->GetWidth() || page->Original.GetHeight() != page->GetHeight())
		SendCustomEvent(ID_Error, wxString::Format(wxT("%s is %d x %d, not %d x %d"), page->Filename.c_str(), page->Original.GetWidth(), page->Original.GetHeight(), page->GetWidth(), page->GetHeight()));
}

/* Resizes an image to fit. */
void ComicBook::ScaleImage(wxUint32 pagenumber)
{
	wxSize image;
	float rCanvas, rImage;  // width/height ratios
	float scalingFactor;

	ComicPage *page = Pages.at(pagenumber);
	if (!page->Original.Ok()) return;
	
	image = GetComicPageSize(pagenumber);
	
	switch(zoom) {

	case ZOOM_CUSTOM:
		scalingFactor = zoomLevel / 100.0f;
		break;

	case ZOOM_FIT:
		if (fitOnlyOversize && !IsOversize(pagenumber))
			goto zoom_original;

		rImage = float(image.x) / float(image.y);
		if ((IsPageLandscape(pagenumber) || mode == ONEPAGE) && mode != CONTINUOUS) {
			rCanvas = float(parent->canvasSize.x) / float(parent->canvasSize.y);
			if (rCanvas > rImage)
				scalingFactor = float(parent->canvasSize.y) / float(image.y);
			else
				scalingFactor = float(parent->canvasSize.x) / float(image.x);
				
		} else if (mode == TWOPAGE) {
			rCanvas = (float(parent->canvasSize.x)/2.0f) / float(parent->canvasSize.y);
			if (rCanvas > rImage)
				scalingFactor = float(parent->canvasSize.y) / float(image.y);
			else
				scalingFactor = (float(parent->canvasSize.x)/2.0f) / float(image.x);
				
		} else if (mode == CONTINUOUS) {
			rCanvas = (float(parent->canvasSize.x)/float(GetPageCount())) / float(parent->canvasSize.y);
			if (rCanvas > rImage)
				scalingFactor = float(parent->canvasSize.y) / float(image.y);
			else
				scalingFactor = (float(parent->canvasSize.x)/float(GetPageCount())) / float(image.x);
		}
		break;

	case ZOOM_WIDTH:
		if (fitOnlyOversize && !IsOversize(pagenumber))
			goto zoom_original;
		if (IsFit(pagenumber, scalingFactor)) {
			if (mode == TWOPAGE) {
				// check neighbor pages to see if they fit too
				if (pagenumber > 0) {
					if (!IsFit(pagenumber - 1))
						goto fith_nofit;
				}
				if (pagenumber < (Pages.size() - 1)) {
					if (!IsFit(pagenumber + 1))
						goto fith_nofit;
				}
			}
		} else {
			fith_nofit:
			rImage = float(image.x) / float(image.y);
			// fit with scrollbars
			if (IsPageLandscape(pagenumber) || mode == ONEPAGE) {
				scalingFactor = float(parent->canvasSize.x) / float(image.x);
			} else {
				scalingFactor = float(parent->canvasSize.x / 2) / float(image.x);
			}
		}
		break;

	case ZOOM_HEIGHT: // fit to height
		if (fitOnlyOversize && !IsOversize(pagenumber))
			goto zoom_original;
		if (IsFit(pagenumber, scalingFactor)) {
			if (mode == TWOPAGE) {
				// check neighbor pages to see if they fit too
				if (pagenumber > 0) {
					if (!IsFit(pagenumber - 1))
						goto fitv_nofit;
				}
				if (pagenumber < Pages.size() - 1) {
					if (!IsFit(pagenumber + 1))
						goto fitv_nofit;
				}
			}
		} else {
			fitv_nofit:
			// fit with scrollbars
			scalingFactor = float(parent->canvasSize.y) / float(image.y);
		}
		break;
		
	case ZOOM_FULL: // no resize
	default:
	zoom_original:
		switch (page->GetOrientation()) {
		case NORTH:
			page->Resample = wxImage(page->Original);
			break;
		case EAST:
			page->Resample = wxImage(page->Original).Rotate90(true);
			break;
		case SOUTH:
			page->Resample = wxImage(page->Original).Rotate90().Rotate90();
			break;
		case WEST:
			page->Resample = wxImage(page->Original).Rotate90(false);
			break;
		default:
			break;
		}
		SendScaledEvent(pagenumber);
		return;
	}
	
	switch (page->GetOrientation()) {
	case NORTH:
		page->Resample = FreeImage_Rescale(page->Original, wxInt32(image.x * scalingFactor), wxInt32(image.y * scalingFactor), filter);
		break;
	case EAST:
		page->Resample = FreeImage_Rescale(page->Original, wxInt32(image.y * scalingFactor), wxInt32(image.x * scalingFactor), filter).Rotate90(true);
		break;
	case SOUTH:
		page->Resample = FreeImage_Rescale(page->Original, wxInt32(image.x * scalingFactor), wxInt32(image.y * scalingFactor), filter).Rotate90().Rotate90();
		break;
	case WEST:
		page->Resample = FreeImage_Rescale(page->Original, wxInt32(image.y * scalingFactor), wxInt32(image.x * scalingFactor), filter).Rotate90(false);
		break;
	}
	SendScaledEvent(pagenumber);
}

void ComicBook::ScaleThumbnail(wxUint32 pagenumber)
{
	ComicPage *page = Pages.at(pagenumber);
	if (!page->Original.Ok()) return;
	
	wxSize scaled = GetThumbnailSize(pagenumber);
	
	switch (page->GetOrientation()) {
	case NORTH:
		page->Thumbnail = FreeImage_Rescale(page->Original, scaled.x, scaled.y, FILTER_BILINEAR);
		break;
	case EAST:
		page->Thumbnail = FreeImage_Rescale(page->Original, scaled.y, scaled.x, FILTER_BILINEAR).Rotate90(true);
		break;
	case SOUTH:
		page->Thumbnail = FreeImage_Rescale(page->Original, scaled.x, scaled.y, FILTER_BILINEAR).Rotate90().Rotate90();
		break;
	case WEST:
		page->Thumbnail = FreeImage_Rescale(page->Original, scaled.y, scaled.x, FILTER_BILINEAR).Rotate90(false);
		break;
	}
	SendThumbnailedEvent(pagenumber);
}

void ComicBook::SetCurrentPage(wxUint32 pagenumber)
{
	currentPage = pagenumber;
	SendCurrentPageChangedEvent();
}

void ComicBook::SendScaledEvent(wxUint32 pagenumber)
{
	wxCommandEvent event(EVT_PAGE_SCALED, -1);
	event.SetInt(pagenumber);
	GetEventHandler()->AddPendingEvent(event);
}

void ComicBook::SendThumbnailedEvent(wxUint32 pagenumber)
{
	wxCommandEvent event(EVT_PAGE_THUMBNAILED, -1);
	event.SetInt(pagenumber);
	GetEventHandler()->AddPendingEvent(event);
}

void ComicBook::SendCurrentPageChangedEvent()
{
	wxCommandEvent event(EVT_CURRENT_PAGE_CHANGED, -1);
	event.SetInt(this->GetCurrentPage());
	GetEventHandler()->AddPendingEvent(event);
}

void ComicBook::SendCustomEvent(wxUint32 id, wxString message)
{
	wxCommandEvent event(EVT_CUSTOM_EVENT, -1);
	event.SetInt(id);
	event.SetString(message);
	GetEventHandler()->AddPendingEvent(event);
}

bool ComicBook::SetPassword()
{
	password = wxGetPasswordFromUser(
			wxT("This archive is password-protected. Please enter the password."),
			wxT("Enter Password"));
	return !password.IsEmpty();
}

void ComicBook::NoPasswordProvided()
{
	wxMessageWarning(NULL, _("No password provided, password protected pages will not be opened."));
}

static bool sortPageFunc(ComicPage* left, ComicPage* right)
{
	// wxWidget's wxString::Cmp() function is actually just a dumb memchr!
	// strcoll / wcscoll does a proper string comparison using the user's
	// current locale
#if _WIN32
	// _WIN32 is really just a shortcut for "fn_str() returns wchar_t*"
	return (wcscoll(left->Filename.fn_str(), right->Filename.fn_str()) <= 0);
#else
	return (strcoll(left->Filename.fn_str(), right->Filename.fn_str()) <= 0);
#endif
}


void ComicBook::postCtor()
{
	// Sort the pages
	std::sort(Pages.begin(), Pages.end(), sortPageFunc);
}
