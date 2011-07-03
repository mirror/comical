/*
 * ComicBook.cpp
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

#include "ComicBook.h"
#include "Exceptions.h"
#include <algorithm>
#include <cstring>
#include <wx/datetime.h>
#include <wx/mstream.h>
#include <wx/utils.h>
#include <wx/textdlg.h>

DEFINE_EVENT_TYPE(EVT_PAGE_SCALED)
DEFINE_EVENT_TYPE(EVT_PAGE_THUMBNAILED)
DEFINE_EVENT_TYPE(EVT_CURRENT_PAGE_CHANGED)
DEFINE_EVENT_TYPE(EVT_PAGE_ERROR)

ComicBook::ComicBook(wxString _filename, wxUint32 _cacheLen, COMICAL_ZOOM _zoom, long _zoomLevel, bool _fitOnlyOversize, COMICAL_MODE _mode, FREE_IMAGE_FILTER _filter, COMICAL_DIRECTION _direction, wxInt32 _scrollbarThickness):
wxThread(wxTHREAD_JOINABLE),
filename(_filename),
password(NULL),
evtHandler(new wxEvtHandler()),
currentPage(0),
cacheLen(_cacheLen),
zoom(_zoom),
zoomLevel(_zoomLevel),
fitOnlyOversize(_fitOnlyOversize),
mode(_mode),
filter(_filter),
direction(_direction),
scrollbarThickness(_scrollbarThickness)
{
}


ComicBook::~ComicBook()
{
	for (unsigned i = 0; i < Pages.size(); i++)
		delete Pages[i];

	delete[] password;
	delete evtHandler;
}


void ComicBook::RotatePage(wxUint32 pagenumber, COMICAL_ROTATE direction)
{
	Pages.at(pagenumber)->Rotate(direction);
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

/* returns TRUE if parameters were different, FALSE if parameters were the same and no changes were made */
bool ComicBook::SetCanvasSize(wxSize canvasSize)
{
	if(canvasWidth != canvasSize.x || canvasHeight != canvasSize.y) {
		for (wxUint32 i = 0; i < Pages.size(); i++) {
			Pages.at(i)->DestroyResample();
		}
		canvasWidth = canvasSize.x;
		canvasHeight = canvasSize.y;
		return TRUE;
	} else
		return FALSE;
}

bool ComicBook::SetScrollbarThickness(wxInt32 newScrollbarThickness)
{
	if(scrollbarThickness != newScrollbarThickness) {
		for (wxUint32 i = 0; i < Pages.size(); i++) {
			Pages.at(i)->DestroyResample();
		}
		scrollbarThickness = newScrollbarThickness;
		return TRUE;
	} else
		return FALSE;
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

bool ComicBook::IsPageReady(wxUint32 pagenumber)
{
	return Pages.at(pagenumber)->Resample.Ok();
}

bool ComicBook::IsThumbReady(wxUint32 pagenumber)
{
	return Pages.at(pagenumber)->Thumbnail.Ok();
}

bool ComicBook::FitWithoutScrollbars(wxUint32 pagenumber, float *scalingFactor)
{
	wxSize size;
	wxInt32 usableWidth, usableWidthScrollbar, usableHeightScrollbar, withoutScrollBarHeight, withoutScrollBarWidth;

	ComicPage *page = Pages.at(pagenumber);

	if (page->GetOrientation() == NORTH || page->GetOrientation() == SOUTH)		
		size = wxSize(page->GetWidth(), page->GetHeight());
	else
		size = wxSize(page->GetHeight(), page->GetWidth());

	float ratio = float (size.x) / float(size.y);

	switch(zoom) {
	case ZOOM_WIDTH:
		if (ratio >= 1.0f || mode == ONEPAGE) {
			usableWidth = canvasWidth;
			usableWidthScrollbar = canvasWidth - scrollbarThickness;
		} else {
			usableWidth = canvasWidth / 2;
			usableWidthScrollbar = (canvasWidth - scrollbarThickness) / 2;
		}
		*scalingFactor = float(usableWidth) / float(size.x);
		withoutScrollBarHeight = wxInt32(float(size.y) * *scalingFactor);
		if (withoutScrollBarHeight > canvasHeight) {
			// it's possible the page will fit without scrollbars at some
			// width in between canvasWidth - scrollbarThickness and
			// canvasWidth.
			// scaling factor if height is maximized
			*scalingFactor = float(canvasHeight) / float(size.y);
			// which will fit best?
			if (wxInt32(*scalingFactor * float(size.x)) > usableWidthScrollbar)
				return true;
			else
				return false;
		} else
			return true;
	case ZOOM_HEIGHT:
		if (ratio >= 1.0f || mode == ONEPAGE) {
			usableWidth = canvasWidth;
		} else {
			usableWidth = canvasWidth / 2;
		}
		*scalingFactor = float(canvasHeight) / float(size.y);
		withoutScrollBarWidth = wxInt32(float(size.x) * *scalingFactor);
		if (withoutScrollBarWidth > usableWidth) {
			// it's possible the page will fit without scrollbars at some
			// width in between canvasHeight - scrollbarThickness and
			// canvasHeight.
			// scaling factor if width is maximized
			*scalingFactor = float(usableWidth) / float(size.x);
			// which will fit best?
			usableHeightScrollbar = canvasHeight - scrollbarThickness;
			if ((*scalingFactor * size.y) > usableHeightScrollbar)
				return true;
			else
				return false;
		} else
			return true;
	case ZOOM_FIT:
		return true;
	default:
		// this function should not be called for other zooms
		break;
	}
	return false;
}

bool ComicBook::FitWithoutScrollbars(wxUint32 pagenumber)
{
	float throwaway = 0.0f;
	return FitWithoutScrollbars(pagenumber, &throwaway);
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
		usableHeight = canvasHeight;
		if (ratio > 1.0f || mode == ONEPAGE)
			usableWidth = canvasWidth;
		else
			usableWidth = canvasWidth / 2;
		break;
	
	case ZOOM_WIDTH:
		usableHeight = canvasHeight;
		// For now, to be safe, assume vertical scrollbar is present thanks to neighboring page.
		if (ratio > 1.0f || mode == ONEPAGE)
			usableWidth = canvasWidth - scrollbarThickness;
		else
			usableWidth = (canvasWidth - scrollbarThickness) / 2;
		break;
	
	case ZOOM_HEIGHT:
		usableWidth = canvasWidth;
		// For now, to be safe, assume horizontal scrollbar is present thanks to neighboring page.
		if (ratio > 1.0f || mode == ONEPAGE)
			usableHeight = canvasHeight - scrollbarThickness;
		else
			usableHeight = (canvasHeight - scrollbarThickness) / 2;
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
	wxUint32 i, target, high, curr;
	wxInt32 low;
	bool scalingHappened;
	ComicPage *page;
	
	while (!TestDestroy()) {
		scalingHappened = false;
		curr = currentPage; // in case this value changes midloop
		
		// The caching algorithm.  First calculates next highest
		// priority page, then checks to see if that page needs
		// updating.  If no pages need updating, yield the timeslice.
		
		if (cacheLen >= Pages.size()) {
			low = 0;
			high = Pages.size() - 1;
		} else {
			/* A moving window of size cacheLen.  2/3 of the pages in the
			 * cache are after the Current page, the other 1/3 are before
			 * it. */
			high = 2 * cacheLen / 3;
			low = cacheLen - high;
			
			high = curr + high - 1;
			low = curr - low;

			/* Keep the window within 0 and Pages.size(). */
			if (high >= Pages.size()) {
				low -= (high - Pages.size()) + 1;
				high = Pages.size() - 1;
			} else if (low < 0) {
				high -= low; // low is negative, this is an addition
				low = 0;
			}
		}

		for (i = 0; i < cacheLen && i < Pages.size(); i++) {
			if (TestDestroy())
				goto thread_end;
			target = curr + i;
			if (target > high)
				target = curr - (target - high);
			
			page = Pages.at(target);
			page->ResampleLock.Lock();
			if (page->Resample.Ok()) {
				page->ResampleLock.Unlock();
				continue;
			}
			
			page->OriginalLock.Lock();
			try {
				if (!page->Original.Ok())
					LoadOriginal(target);
			} catch (ArchiveException *ae) {
				SendPageErrorEvent(target, ae->Message);
			}
			
			ScaleImage(target);
			scalingHappened = true;
			
			page->ResampleLock.Unlock();

			page->ThumbnailLock.Lock();
			if (!page->Thumbnail.Ok()) {
				ScaleThumbnail(target);
				if (!page->Thumbnail.Ok()) { // let's only see one error if things go wrong
					SendPageErrorEvent(target, wxString::Format(wxT("Could not create thumbnail for page %d."), target));
					page->Thumbnail = wxImage(1, 1);
				}
			}
			page->ThumbnailLock.Unlock();
			page->OriginalLock.Unlock();

			page->DestroyOriginal();

			
			break;
		}
		
		if (!scalingHappened) {
			// if the cache is full, then use this iteration to fetch a
			// thumbnail, if needed.
			for (wxUint32 j = 0; j < Pages.size(); j++) {
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
					SendPageErrorEvent(j, ae->Message);
				}
				
				ScaleThumbnail(j);
				page->ThumbnailLock.Unlock();
				page->OriginalLock.Unlock();

				page->DestroyOriginal();
				break;
			}
		}

		if (i < cacheLen && i < Pages.size()) {
			if (cacheLen < Pages.size()) {
				// Delete pages outside of the cache's range.
				for (i = 0; wxInt32(i) < low; i++) {
					page = Pages.at(i);
					page->DestroyResample();
				}
				
				for (i = Pages.size() - 1; i > high; i--) {

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
		SendPageErrorEvent(pagenumber, wxString::Format(wxT("Failed to extract page %d."), pagenumber));
		page->Original = wxImage(1, 1);
	}
	if (!page->Original.Ok()) {
		SendPageErrorEvent(pagenumber, wxString::Format(wxT("Failed to extract page %d."), pagenumber));
		page->Original = wxImage(1, 1);
	}
	wxDELETE(stream);
	// TEMPORARY TODO
	if (page->Original.GetWidth() != page->GetWidth() || page->Original.GetHeight() != page->GetHeight())
		SendPageErrorEvent(pagenumber, wxString::Format(wxT("%s is %d x %d, not %d x %d"), page->Filename.c_str(), page->Original.GetWidth(), page->Original.GetHeight(), page->GetWidth(), page->GetHeight()));
}

/* Resizes an image to fit. */
void ComicBook::ScaleImage(wxUint32 pagenumber)
{
	wxInt32 xImage, yImage;
	float rCanvas, rImage;  // width/height ratios
	float scalingFactor;

	ComicPage *page = Pages.at(pagenumber);
	
	if (page->GetOrientation() == NORTH || page->GetOrientation() == SOUTH) {
		xImage = page->GetWidth();
		yImage = page->GetHeight();
	} else {// EAST || WEST
		yImage = page->GetWidth();
		xImage = page->GetHeight();
	}
	
	switch(zoom) {

	case ZOOM_CUSTOM:
		scalingFactor = zoomLevel / 100.0f;
		break;

	case ZOOM_FIT:
		if (fitOnlyOversize && !IsOversize(pagenumber))
			goto zoom_original;
		rImage = float(xImage) / float(yImage);
		if (rImage >= 1.0f || mode == ONEPAGE) {
			rCanvas = float(canvasWidth) / float(canvasHeight);
			if (rCanvas > rImage)
				scalingFactor = float(canvasHeight) / float(yImage);
			else
				scalingFactor = float(canvasWidth) / float(xImage);
		} else {
			rCanvas = (float(canvasWidth)/2.0f) / float(canvasHeight);
			if (rCanvas > rImage)
				scalingFactor = float(canvasHeight) / float(yImage);
			else
				scalingFactor = (float(canvasWidth)/2.0f) / float(xImage);
		}
		break;

	case ZOOM_WIDTH:
		if (fitOnlyOversize && !IsOversize(pagenumber))
			goto zoom_original;
		if (FitWithoutScrollbars(pagenumber, &scalingFactor)) {
			if (mode == TWOPAGE) {
				// check neighbor pages to see if they fit too
				if (pagenumber > 0) {
					if (!FitWithoutScrollbars(pagenumber - 1))
						goto fith_nofit;
				}
				if (pagenumber < (Pages.size() - 1)) {
					if (!FitWithoutScrollbars(pagenumber + 1))
						goto fith_nofit;
				}
			}
		} else {
			fith_nofit:
			rImage = float(xImage) / float(yImage);
			// fit with scrollbars
			if (rImage >= 1.0f || mode == ONEPAGE) {
				scalingFactor = float(canvasWidth - scrollbarThickness) / float(xImage);
			} else {
				scalingFactor = float((canvasWidth - scrollbarThickness) / 2) / float(xImage);
			}
		}
		break;

	case ZOOM_HEIGHT: // fit to height
		if (fitOnlyOversize && !IsOversize(pagenumber))
			goto zoom_original;
		if (FitWithoutScrollbars(pagenumber, &scalingFactor)) {
			if (mode == TWOPAGE) {
				// check neighbor pages to see if they fit too
				if (pagenumber > 0) {
					if (!FitWithoutScrollbars(pagenumber - 1))
						goto fitv_nofit;
				}
				if (pagenumber < Pages.size() - 1) {
					if (!FitWithoutScrollbars(pagenumber + 1))
						goto fitv_nofit;
				}
			}
		} else {
			fitv_nofit:
			// fit with scrollbars
			scalingFactor = float(canvasHeight - scrollbarThickness) / float(yImage);
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
		page->Resample = FreeImage_Rescale(page->Original, wxInt32(xImage * scalingFactor), wxInt32(yImage * scalingFactor), filter);
		break;
	case EAST:
		page->Resample = FreeImage_Rescale(page->Original, wxInt32(yImage * scalingFactor), wxInt32(xImage * scalingFactor), filter).Rotate90(true);
		break;
	case SOUTH:
		page->Resample = FreeImage_Rescale(page->Original, wxInt32(xImage * scalingFactor), wxInt32(yImage * scalingFactor), filter).Rotate90().Rotate90();
		break;
	case WEST:
		page->Resample = FreeImage_Rescale(page->Original, wxInt32(yImage * scalingFactor), wxInt32(xImage * scalingFactor), filter).Rotate90(false);
		break;
	}
	SendScaledEvent(pagenumber);
}

void ComicBook::ScaleThumbnail(wxUint32 pagenumber)
{
	wxInt32 xImage, yImage, xScaled, yScaled;
	float scalingFactor;
	
	ComicPage *page = Pages.at(pagenumber);
	
	COMICAL_ROTATE pageOrientation = page->GetOrientation();
	if (pageOrientation == NORTH || pageOrientation == SOUTH) {
		xImage = page->GetWidth();
		yImage = page->GetHeight();
	} else {// EAST || WEST
		yImage = page->GetWidth();
		xImage = page->GetHeight();
	}
	
	if(float(yImage) / float(xImage) > 0.6f) {
		yScaled = 60;
		scalingFactor = 60.0f / float(yImage);
		xScaled = wxInt32(float(xImage) * scalingFactor);
	} else {
		xScaled = 100;
		scalingFactor = 100.0f / float(xImage);
		yScaled = wxInt32(float(yImage) * scalingFactor);
	}
	
	switch (page->GetOrientation()) {
	case NORTH:
		page->Thumbnail = FreeImage_Rescale(page->Original, xScaled, yScaled, FILTER_BILINEAR);
		break;
	case EAST:
		page->Thumbnail = FreeImage_Rescale(page->Original, yScaled, xScaled, FILTER_BILINEAR).Rotate90(true);
		break;
	case SOUTH:
		page->Thumbnail = FreeImage_Rescale(page->Original, xScaled, yScaled, FILTER_BILINEAR).Rotate90().Rotate90();
		break;
	case WEST:
		page->Thumbnail = FreeImage_Rescale(page->Original, yScaled, xScaled, FILTER_BILINEAR).Rotate90(false);
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

void ComicBook::SendPageErrorEvent(wxUint32 pagenumber, wxString message)
{
	wxCommandEvent event(EVT_PAGE_ERROR, -1);
	event.SetInt(pagenumber);
	event.SetString(message);
	GetEventHandler()->AddPendingEvent(event);
}

void ComicBook::SetPassword(const char* new_password)
{
	if (!new_password)
		return;
	if (password)
		delete[] password;
	password = new char[strlen(new_password) + 1];
	strcpy(password, new_password);
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
	if (!password) { // the password may already have been set if this is a RAR with encrypted headers
		wxString new_password;
		while (!TestPassword()) { // if the password needs to be set
			new_password = wxGetPasswordFromUser(
					wxT("This archive is password-protected.  Please enter the password."),
					wxT("Enter Password"));
			if (new_password.IsEmpty()) // the dialog was cancelled, and the archive cannot be opened
				throw ArchiveException(filename, wxT("Comical could not open this file because it is password-protected."));
			SetPassword(new_password.ToAscii());
		}
	}

	// Sort the pages
	std::sort(Pages.begin(), Pages.end(), sortPageFunc);
}
