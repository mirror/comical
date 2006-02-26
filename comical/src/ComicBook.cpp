/***************************************************************************
                ComicBook.cpp - ComicBook class and its children
                             -------------------
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

#include "ComicBook.h"
#include "Exceptions.h"
#include <cstring>
#include <wx/datetime.h>
#include <wx/mstream.h>
#include <wx/utils.h>
#include <wx/textdlg.h>

DEFINE_EVENT_TYPE(EVT_PAGE_SCALED)
DEFINE_EVENT_TYPE(EVT_PAGE_THUMBNAILED)
DEFINE_EVENT_TYPE(EVT_CURRENT_PAGE_CHANGED)
DEFINE_EVENT_TYPE(EVT_PAGE_ERROR)

ComicBook::ComicBook(wxString _filename, wxUint32 _cacheLen, COMICAL_ZOOM _zoom, long _zoomLevel, bool _fitOnlyOversize, COMICAL_MODE _mode, FREE_IMAGE_FILTER _filter, COMICAL_DIRECTION _direction, wxInt32 _scrollbarThickness) : wxThread(wxTHREAD_JOINABLE), filename(_filename), cacheLen(_cacheLen), zoom(_zoom), zoomLevel(_zoomLevel), fitOnlyOversize(_fitOnlyOversize), mode(_mode), filter(_filter), direction(_direction), scrollbarThickness(_scrollbarThickness)
{
	pageCount = 0;
	currentPage = 0;
	evtHandler = new wxEvtHandler();
	// Each of the long values is followed by the letter L not the number one
	Filenames = new wxArrayString();
	originals = NULL;
	resamples = NULL;
	thumbnails = NULL;
	Orientations = NULL;
	originalLockers = NULL;
	resampleLockers = NULL;
	thumbnailLockers = NULL;
	password = NULL;
}

ComicBook::~ComicBook()
{
	for(wxUint32 i = 0; i < pageCount; i++) {
		if (originals[i].Ok())
			originals[i].Destroy();
		if (resamples[i].Ok())
			resamples[i].Destroy();
		if (thumbnails[i].Ok())
			thumbnails[i].Destroy();
	}
	if (originals)
		delete[] originals;
	if (resamples)
		delete[] resamples;
	if (thumbnails)
		delete[] thumbnails;
	if (Orientations)
		delete[] Orientations;
	if (originalLockers)
		delete[] originalLockers;
	if (resampleLockers)
		delete[] resampleLockers;
	if (thumbnailLockers)
		delete[] thumbnailLockers;
	if (Filenames)
		delete Filenames;
	if (password)
		delete[] password;
	if (evtHandler)
		delete evtHandler;
}

void ComicBook::RotatePage(wxUint32 pagenumber, COMICAL_ROTATE direction)
{
	if (Orientations[pagenumber] != direction) {
		wxMutexLocker rlock(resampleLockers[pagenumber]);
		wxMutexLocker tlock(thumbnailLockers[pagenumber]);
		Orientations[pagenumber] = direction;
		resamples[pagenumber].Destroy();
		thumbnails[pagenumber].Destroy();
	}
}

/* returns TRUE if newMode is different, FALSE if the same and no changes were made */
bool ComicBook::SetMode(COMICAL_MODE newMode)
{
	if(mode != newMode) {
		wxUint32 i;
		for (i = 0; i < pageCount; i++) {
			resampleLockers[i].Lock();
			if (resamples[i].Ok())
				resamples[i].Destroy();
		}
		mode = newMode;
		for (i = 0; i < pageCount; i++)
			resampleLockers[i].Unlock();
		return TRUE;
	} else
		return FALSE;
}

/* returns TRUE if newFilter is different, FALSE if the same and no changes were made */
bool ComicBook::SetFilter(FREE_IMAGE_FILTER newFilter)
{
	if(filter != newFilter) {
		wxUint32 i;
		for (i = 0; i < pageCount; i++) {
			resampleLockers[i].Lock();
			if (resamples[i].Ok())
				resamples[i].Destroy();
		}
		filter = newFilter;
		for (i = 0; i < pageCount; i++)
			resampleLockers[i].Unlock();
		return TRUE;
	} else
		return FALSE;
}

/* Makes pages zoom to newZoom level, and will zoom pages that are smaller than the canvas only if newFitOnlyIfOversize is FALSE.
 * returns TRUE if parameters were different, FALSE if parameters were the same and no changes were made */
bool ComicBook::SetZoom(COMICAL_ZOOM newZoom)
{
	if(zoom != newZoom) {
		wxUint32 i;
		for (i = 0; i < pageCount; i++) {
			resampleLockers[i].Lock();
			if (resamples[i].Ok())
				resamples[i].Destroy();
		}

		zoom = newZoom;
		for (i = 0; i < pageCount; i++)
			resampleLockers[i].Unlock();
		return TRUE;
	} else
		return FALSE;
}

/* Makes pages zoom to newZoomLevel%.
 * returns TRUE if parameters were different, FALSE if parameters were the same and no changes were made */
bool ComicBook::SetZoomLevel(long newZoomLevel)
{
	if(zoomLevel != newZoomLevel) {
		wxUint32 i;
		if (zoom == ZOOM_CUSTOM) {
			for (i = 0; i < pageCount; i++) {
				resampleLockers[i].Lock();
				if (resamples[i].Ok())
					resamples[i].Destroy();
			}
		}
		zoomLevel = newZoomLevel;
		if (zoom == ZOOM_CUSTOM) {
			for (i = 0; i < pageCount; i++)
				resampleLockers[i].Unlock();
		}
		return TRUE;
	} else
		return FALSE;
}

bool ComicBook::SetFitOnlyOversize(bool newFitOnlyOversize)
{
	if (fitOnlyOversize != newFitOnlyOversize) {
		wxUint32 i;
		for (i = 0; i < pageCount; i++) {
			resampleLockers[i].Lock();
			if (resamples[i].Ok())
				resamples[i].Destroy();
		}
		
		fitOnlyOversize = newFitOnlyOversize;
		for (i = 0; i < pageCount; i++)
			resampleLockers[i].Unlock();
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
		wxUint32 i;
		for (i = 0; i < pageCount; i++) {
			resampleLockers[i].Lock();
			if (resamples[i].Ok())
				resamples[i].Destroy();
		}
		canvasWidth = canvasSize.x;
		canvasHeight = canvasSize.y;
		for (i = 0; i < pageCount; i++)
			resampleLockers[i].Unlock();
		return TRUE;
	} else
		return FALSE;
}

bool ComicBook::SetScrollbarThickness(wxInt32 newScrollbarThickness)
{
	if(scrollbarThickness != newScrollbarThickness) {
		wxUint32 i;
		for (i = 0; i < pageCount; i++) {
			resampleLockers[i].Lock();
			if (resamples[i].Ok())
				resamples[i].Destroy();
		}
		scrollbarThickness = newScrollbarThickness;
		for (i = 0; i < pageCount; i++)
			resampleLockers[i].Unlock();
		return TRUE;
	} else
		return FALSE;
}

wxBitmap * ComicBook::GetPage(wxUint32 pagenumber)
{
	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);
	wxMutexLocker lock(resampleLockers[pagenumber]);
	if (!resamples[pagenumber].Ok())
		return NULL;
	return new wxBitmap(resamples[pagenumber]);
}

wxBitmap * ComicBook::GetPageLeftHalf(wxUint32 pagenumber)
{
	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);
	wxMutexLocker lock(resampleLockers[pagenumber]);
	if (!resamples[pagenumber].Ok())
		return NULL;
	wxInt32 rWidth = resamples[pagenumber].GetWidth();
	wxInt32 rHeight = resamples[pagenumber].GetHeight();
	if (direction == COMICAL_RTL) {
		wxInt32 offset = rWidth / 2;
		wxInt32 remainder = rWidth % 2;
		return new wxBitmap(resamples[pagenumber].GetSubImage(wxRect(offset, 0, offset + remainder, rHeight)));
	} else
		return new wxBitmap(resamples[pagenumber].GetSubImage(wxRect(0, 0, rWidth / 2, rHeight)));
}

wxBitmap * ComicBook::GetPageRightHalf(wxUint32 pagenumber)
{
	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);
	wxMutexLocker lock(resampleLockers[pagenumber]);
	if (!resamples[pagenumber].Ok())
		return NULL;
	wxInt32 rWidth = resamples[pagenumber].GetWidth();
	wxInt32 rHeight = resamples[pagenumber].GetHeight();
	if (direction == COMICAL_LTR) {
		wxInt32 offset = rWidth / 2;
		wxInt32 remainder = rWidth % 2;
		return new wxBitmap(resamples[pagenumber].GetSubImage(wxRect(offset, 0, offset + remainder, rHeight)));
	} else
		return new wxBitmap(resamples[pagenumber].GetSubImage(wxRect(0, 0, rWidth / 2, rHeight)));
}

wxBitmap * ComicBook::GetThumbnail(wxUint32 pagenumber)
{
	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);
	wxMutexLocker lock(thumbnailLockers[pagenumber]);
	if (!thumbnails[pagenumber].Ok())
		return NULL;	
	return new wxBitmap(thumbnails[pagenumber]);
}

bool ComicBook::IsPageLandscape(wxUint32 pagenumber)
{
	wxInt32 rWidth, rHeight;
	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);
	
	resampleLockers[pagenumber].Lock();
	if (resamples[pagenumber].Ok()) {
		rWidth = resamples[pagenumber].GetWidth();
		rHeight = resamples[pagenumber].GetHeight();
		resampleLockers[pagenumber].Unlock();
		goto IsPageLandscapeRatioCalculate;
	} 
	resampleLockers[pagenumber].Unlock();
	
	thumbnailLockers[pagenumber].Lock();
	if (thumbnails[pagenumber].Ok()) {
		rWidth = thumbnails[pagenumber].GetWidth();
		rHeight = thumbnails[pagenumber].GetHeight();
		thumbnailLockers[pagenumber].Unlock();
		goto IsPageLandscapeRatioCalculate;
	}
	thumbnailLockers[pagenumber].Unlock();

	wxBeginBusyCursor();
	
	// If the page has never been loaded, even in thumbnail form, the display
	// will really screw up if the page IS landscape and this function says
	// it's not.  Therefore, we wait until it is ready.
	while (!originals[pagenumber].Ok()) {
		wxMilliSleep(10);
	}
	
	originalLockers[pagenumber].Lock();
	if (Orientations[pagenumber] == NORTH ||
			Orientations[pagenumber] == SOUTH) {
		rWidth = originals[pagenumber].GetWidth();
		rHeight = originals[pagenumber].GetHeight();
	} else {
		rHeight = originals[pagenumber].GetWidth();
		rWidth = originals[pagenumber].GetHeight();
	}
	originalLockers[pagenumber].Unlock();
	
	wxEndBusyCursor();
	
	IsPageLandscapeRatioCalculate:
	if ((float(rWidth)/float(rHeight)) > 1.0f)
		return true;
	else
		return false;
}

bool ComicBook::IsPageReady(wxUint32 pagenumber)
{
	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);
	return resamples[pagenumber].Ok();
}

bool ComicBook::IsThumbReady(wxUint32 pagenumber)
{
	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);
	return thumbnails[pagenumber].Ok();
}

bool ComicBook::FitWithoutScrollbars(wxUint32 pagenumber, float *scalingFactor)
{
	wxSize size;
	wxInt32 usableWidth, usableWidthScrollbar, usableHeightScrollbar, withoutScrollBarHeight, withoutScrollBarWidth;

	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);

	if (Orientations[pagenumber] == NORTH || Orientations[pagenumber] == SOUTH)		
		size = wxSize(originals[pagenumber].GetWidth(), originals[pagenumber].GetHeight());
	else
		size = wxSize(originals[pagenumber].GetHeight(), originals[pagenumber].GetWidth());

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

	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);

	if (Orientations[pagenumber] == NORTH || Orientations[pagenumber] == SOUTH)		
		size = wxSize(originals[pagenumber].GetWidth(), originals[pagenumber].GetHeight());
	else
		size = wxSize(originals[pagenumber].GetHeight(), originals[pagenumber].GetWidth());

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
	wxInputStream *stream;
	wxMemoryInputStream *mstream;

	while (!TestDestroy()) {
		scalingHappened = false;
		curr = currentPage; // in case this value changes midloop
		
		// The caching algorithm.  First calculates next highest
		// priority page, then checks to see if that page needs
		// updating.  If no pages need updating, yield the timeslice.
		
		if (cacheLen >= pageCount) {
			low = 0;
			high = pageCount - 1;
		} else {
			/* A moving window of size cacheLen.  2/3 of the pages in the
			 * cache are after the Current page, the other 1/3 are before
			 * it. */
			high = 2 * cacheLen / 3;
			low = cacheLen - high;
			
			high = curr + high - 1;
			low = curr - low;

			/* Keep the window within 0 and pageCount. */
			if (high >= pageCount) {
				low -= (high - pageCount) + 1;
				high = pageCount - 1;
			} else if (low < 0) {
				high -= low; // low is negative, this is an addition
				low = 0;
			}
		}

		for (i = 0; i < cacheLen && i < pageCount; i++) {
			if (TestDestroy())
				goto thread_end;
			target = curr + i;
			if (curr > 0) {	// ftw and fth scaling requires previous page to
				target--;	// do FitWithoutScrollbars.
				if (target > high)
					target = curr - (++target - high);
			} else if (target > high)
				target = curr - (target - high);
			try {
				originalLockers[target].Lock();
				if (originals[target].Ok()) {
					originalLockers[target].Unlock();
					continue;
				}
				stream = ExtractStream(target);
				if (stream->IsOk() && stream->GetSize() > 0) {
					originals[target].LoadFile(*stream);
					// Memory Input Streams don't take ownership of the buffer
					mstream = dynamic_cast<wxMemoryInputStream*>(stream);
					if (mstream)
						delete[] (wxUint8 *) mstream->GetInputStreamBuffer()->GetBufferStart();
				} else {
					SendPageErrorEvent(target, wxString::Format(wxT("Failed to extract page %d."), target));
					originals[target] = wxImage(1, 1);
				}
				if (!originals[target].Ok()) {
					SendPageErrorEvent(target, wxString::Format(wxT("Failed to extract page %d."), target));
					originals[target] = wxImage(1, 1);
				}
				wxDELETE(stream);
			} catch (ArchiveException *ae) {
				SendPageErrorEvent(target, ae->Message);
			}
				
			thumbnailLockers[target].Lock();
			if (!thumbnails[target].Ok())
				ScaleThumbnail(target);
			if (!thumbnails[target].Ok()) // let's only see one error if things go wrong
				SendPageErrorEvent(target, wxString::Format(wxT("Could not create thumbnail for page %d."), target));

			thumbnailLockers[target].Unlock();
			originalLockers[target].Unlock();
			break; // extraction happened, we should try a rescale now
		}
		
		for (i = 0; i < cacheLen && i < pageCount; i++) {
			if (TestDestroy())
				goto thread_end;
			target = curr + i;
			if (target > high)
				target = curr - (target - high);
			
			resampleLockers[target].Lock();
			
			if (resamples[target].Ok() || !originals[target].Ok() ||
					// Only try scaling if the neighbors are extracted.  Otherwise,
					// we can't test whether the neighbors will fit without scrollbars.
					(target > 0 && !originals[target - 1].Ok()) ||
					(target < (pageCount - 1) && !originals[target + 1].Ok())) {
				resampleLockers[target].Unlock();
				continue;
			}
			
			ScaleImage(target);
			scalingHappened = true;
			resampleLockers[target].Unlock();
			break;
		}
		
		if (!scalingHappened) {
			// if the cache is full, then use this iteration to fetch a
			// thumbnail, if needed.
			for (wxUint32 j = 0; j < pageCount; j++) {
				thumbnailLockers[j].Lock();
				
				if (thumbnails[j].Ok()) {
					thumbnailLockers[j].Unlock();
					continue;
				}
				try {
					originalLockers[j].Lock();
					if (!originals[j].Ok()) {
						stream = ExtractStream(j);
						if (stream->IsOk() && stream->GetSize() > 0) {
							originals[j].LoadFile(*stream);
							// Memory Input Streams don't take ownership of the buffer
							mstream = dynamic_cast<wxMemoryInputStream*>(stream);
							if (mstream)
								delete[] (wxUint8 *) mstream->GetInputStreamBuffer()->GetBufferStart();
						} else {
							SendPageErrorEvent(j, wxString::Format(wxT("Failed to extract page %d."), j));
							originals[j] = wxImage(1, 1);
						}
						if (!originals[j].Ok()) {
							SendPageErrorEvent(j, wxString::Format(wxT("Failed to extract page %d."), j));
							originals[j] = wxImage(1, 1);
						}
						wxDELETE(stream);
					}
				} catch (ArchiveException *ae) {
					SendPageErrorEvent(j, wxString::Format(wxT("Failed to extract page %d."), j));
				}
				ScaleThumbnail(j);
				originalLockers[j].Unlock();
				thumbnailLockers[j].Unlock();
				break;
			}
		}

		if (i < cacheLen && i < pageCount) {
			if (cacheLen < pageCount) {
				// Delete pages outside of the cache's range.
				for (i = 0; wxInt32(i) < low; i++) {

					resampleLockers[i].Lock();
					if(resamples[i].Ok())
						resamples[i].Destroy();
					resampleLockers[i].Unlock();
					
					originalLockers[i].Lock();
					if(originals[i].Ok())
						originals[i].Destroy();
					originalLockers[i].Unlock();
				}
				
				for (i = pageCount - 1; i > high; i--) {

					resampleLockers[i].Lock();
					if(resamples[i].Ok())
						resamples[i].Destroy();
					resampleLockers[i].Unlock();
					
					originalLockers[i].Lock();
					if(originals[i].Ok())
						originals[i].Destroy();
					originalLockers[i].Unlock();
				}
			}
		}

		Yield();
		Sleep(20);
	}
	thread_end:
	return 0;
}

/* Resizes an image to fit. */
void ComicBook::ScaleImage(wxUint32 pagenumber)
{
	wxInt32 xImage, yImage;
	float rCanvas, rImage;  // width/height ratios
	float scalingFactor;

	wxImage &orig = originals[pagenumber];

	if (Orientations[pagenumber] == NORTH || Orientations[pagenumber] == SOUTH) {
		xImage = orig.GetWidth();
		yImage = orig.GetHeight();
	} else {// EAST || WEST
		yImage = orig.GetWidth();
		xImage = orig.GetHeight();
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
				if (pagenumber < (pageCount - 1)) {
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
				if (pagenumber < pageCount - 1) {
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
		switch (Orientations[pagenumber]) {
		case NORTH:
			resamples[pagenumber] = wxImage(orig);
			break;
		case EAST:
			resamples[pagenumber] = wxImage(orig).Rotate90(true);
			break;
		case SOUTH:
			resamples[pagenumber] = wxImage(orig).Rotate90().Rotate90();
			break;
		case WEST:
			resamples[pagenumber] = wxImage(orig).Rotate90(false);
			break;
		default:
			break;
		}
		SendScaledEvent(pagenumber);
		return;
	}

	switch (Orientations[pagenumber]) {
	case NORTH:
		resamples[pagenumber] = FreeImage_Rescale(orig, wxInt32(xImage * scalingFactor), wxInt32(yImage * scalingFactor), filter);
		break;
	case EAST:
		resamples[pagenumber] = FreeImage_Rescale(orig, wxInt32(yImage * scalingFactor), wxInt32(xImage * scalingFactor), filter).Rotate90(true);
		break;
	case SOUTH:
		resamples[pagenumber] = FreeImage_Rescale(orig, wxInt32(xImage * scalingFactor), wxInt32(yImage * scalingFactor), filter).Rotate90().Rotate90();
		break;
	case WEST:
		resamples[pagenumber] = FreeImage_Rescale(orig, wxInt32(yImage * scalingFactor), wxInt32(xImage * scalingFactor), filter).Rotate90(false);
		break;
	}
	SendScaledEvent(pagenumber);
}

void ComicBook::ScaleThumbnail(wxUint32 pagenumber)
{
	wxImage &orig = originals[pagenumber];
	wxInt32 xImage, yImage, xScaled, yScaled;
	float scalingFactor;
	
	if (Orientations[pagenumber] == NORTH || Orientations[pagenumber] == SOUTH) {
		xImage = orig.GetWidth();
		yImage = orig.GetHeight();
	} else {// EAST || WEST
		yImage = orig.GetWidth();
		xImage = orig.GetHeight();
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
	
	switch (Orientations[pagenumber]) {
	case NORTH:
		thumbnails[pagenumber] = FreeImage_Rescale(orig, xScaled, yScaled, filter);
		break;
	case EAST:
		thumbnails[pagenumber] = FreeImage_Rescale(orig, yScaled, xScaled, filter).Rotate90(true);
		break;
	case SOUTH:
		thumbnails[pagenumber] = FreeImage_Rescale(orig, xScaled, yScaled, filter).Rotate90().Rotate90();
		break;
	case WEST:
		thumbnails[pagenumber] = FreeImage_Rescale(orig, yScaled, xScaled, filter).Rotate90(false);
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

void ComicBook::postCtor()
{
	Filenames->Sort();
	Filenames->Shrink();
	
	pageCount = Filenames->GetCount();
	
	originals = new wxImage[pageCount];
	resamples = new wxImage[pageCount];
	thumbnails = new wxImage[pageCount];
	originalLockers = new wxMutex[pageCount];
	resampleLockers = new wxMutex[pageCount];
	thumbnailLockers = new wxMutex[pageCount];
	Orientations = new COMICAL_ROTATE[pageCount]; // NORTH == 0
	for (wxUint32 i = 0; i < pageCount; i++)
		Orientations[i] = NORTH;

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
}
