/***************************************************************************
 *   Copyright (C) 2005 by James Leighton Athey                            *
 *   jathey@comcast.net                                                    *
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
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ComicBook.h"
#include "Exceptions.h"
#include <cstring>
#include <wx/mstream.h>

DEFINE_EVENT_TYPE(EVT_PAGE_SCALED)
DEFINE_EVENT_TYPE(EVT_PAGE_THUMBNAILED)
DEFINE_EVENT_TYPE(EVT_CURRENT_PAGE_CHANGED)

ComicBook::ComicBook(wxString file) : wxThread(wxTHREAD_JOINABLE)
{
	pageCount = 0;
	currentPage = 0;
	evtHandler = new wxEvtHandler();
	filename = file;
	wxConfigBase *config = wxConfigBase::Get();
	// Each of the long values is followed by the letter L not the number one
	cacheLen = (wxUint32) config->Read(wxT("CacheLength"), 10l); // Fit-to-Width is default
	Filenames = new wxArrayString();
	originals = NULL;
	resamples = NULL;
	thumbnails = NULL;
	Orientations = NULL;
	resampleLockers = NULL;
	thumbnailLockers = NULL;
	password = NULL;
}

ComicBook::~ComicBook()
{
	wxConfigBase *config = wxConfigBase::Get();
	config->Write(wxT("CacheLength"), (int) cacheLen);
	for(wxUint32 i = 0; i < pageCount; i++) {
		if (originals[i].Ok())
			originals[i].Destroy();
		if (resamples[i].Ok())
			resamples[i].Destroy();
	}
	if (originals)
		delete[] originals;
	if (resamples)
		delete[] resamples;
	if (thumbnails)
		delete[] thumbnails;
	if (Orientations)
		delete[] Orientations;
	if (resampleLockers)
		delete[] resampleLockers;
	if (thumbnailLockers)
		delete[] thumbnailLockers;
	if (Filenames)
		delete Filenames;
	if (password) {
		delete[] password;
		password = NULL;
	}
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

/* returns TRUE if paramaters were different, FALSE if parameters were the same and no changes were made */
bool ComicBook::SetParams(COMICAL_MODE newMode, FREE_IMAGE_FILTER newFilter, COMICAL_ZOOM newZoom, wxInt32 newWidth, wxInt32 newHeight, wxInt32 newScrollBarThickness)
{
	if(mode != newMode || fiFilter != newFilter || zoom != newZoom || canvasWidth != newWidth || canvasHeight != newHeight || scrollBarThickness != newScrollBarThickness) {
		wxUint32 i;
		for (i = 0; i < pageCount; i++) {
			resampleLockers[i].Lock();
			if (resamples[i].Ok())
				resamples[i].Destroy();
		}
		mode = newMode;
		fiFilter = newFilter;
		zoom = newZoom;
		canvasWidth = newWidth;
		canvasHeight = newHeight;
		scrollBarThickness = newScrollBarThickness;
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
	wxInt32 offset = rWidth / 2;
	wxInt32 remainder = rWidth % 2;
	return new wxBitmap(resamples[pagenumber].GetSubImage(wxRect(offset, 0, offset + remainder, rHeight)));
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
	wxMutexLocker lock(resampleLockers[pagenumber]);
	if (resamples[pagenumber].Ok()) {
		rWidth = resamples[pagenumber].GetWidth();
		rHeight = resamples[pagenumber].GetHeight();
	} else if (thumbnails[pagenumber].Ok()) {
		rWidth = thumbnails[pagenumber].GetWidth();
		rHeight = thumbnails[pagenumber].GetHeight();
	} else if (originals[pagenumber].Ok()) {
		if (Orientations[pagenumber] == NORTH ||
				Orientations[pagenumber] == SOUTH) {
			rWidth = originals[pagenumber].GetWidth();
			rHeight = originals[pagenumber].GetHeight();
		} else {
			rHeight = originals[pagenumber].GetWidth();
			rWidth = originals[pagenumber].GetHeight();
		}
	} else
		return false;
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
	case FITWIDTH:
		if (ratio >= 1.0f || mode == ONEPAGE) {
			usableWidth = canvasWidth;
			usableWidthScrollbar = canvasWidth - scrollBarThickness;
		} else {
			usableWidth = canvasWidth / 2;
			usableWidthScrollbar = (canvasWidth - scrollBarThickness) / 2;
		}
		*scalingFactor = float(usableWidth) / float(size.x);
		withoutScrollBarHeight = wxInt32(float(size.y) * *scalingFactor);
		if (withoutScrollBarHeight > canvasHeight) {
			// it's possible the page will fit without scrollbars at some
			// width in between canvasWidth - scrollBarThickness and
			// canvasWidth.
			// scaling factor if height is maximized
			*scalingFactor = float(canvasHeight) / float(size.y);
			// which will fit best?
			if ((*scalingFactor * size.x) > usableWidthScrollbar)
				return true;
			else
				return false;
		} else
			return true;
	case FITHEIGHT:
		if (ratio >= 1.0f || mode == ONEPAGE) {
			usableWidth = canvasWidth;
		} else {
			usableWidth = canvasWidth / 2;
		}
		*scalingFactor = float(canvasHeight) / float(size.y);
		withoutScrollBarWidth = wxInt32(float(size.x) * *scalingFactor);
		if (withoutScrollBarWidth > usableWidth) {
			// it's possible the page will fit without scrollbars at some
			// width in between canvasHeight - scrollBarThickness and
			// canvasHeight.
			// scaling factor if width is maximized
			*scalingFactor = float(usableWidth) / float(size.x);
			// which will fit best?
			usableHeightScrollbar = canvasHeight - scrollBarThickness;
			if ((*scalingFactor * size.y) > usableHeightScrollbar)
				return true;
			else
				return false;
		} else
			return true;
	case FIT:
		return true;
	case ONEQ:
	case HALF:
	case THREEQ:
	case FULL:
		// this function should not be called in these cases
		break;
	}
	return false;
}

bool ComicBook::FitWithoutScrollbars(wxUint32 pagenumber)
{
	float throwaway = 0.0f;
	return FitWithoutScrollbars(pagenumber, &throwaway);
}

void * ComicBook::Entry()
{
	wxUint32 i;
	wxInt32 low, high, target, curr;
	bool scalingHappened;
	wxInputStream *stream;
	wxMemoryInputStream *mstream;

	while (!TestDestroy()) {
		scalingHappened = false;
		curr = wxInt32(currentPage); // in case this value changes midloop
		
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
			if (high >= wxInt32(pageCount)) {
				low -= (high - pageCount) + 1;
				high = pageCount - 1;
			} else if (low < 0) {
				high -= low; // low is negative, this is an addition
				low = 0;
			}
		}

		for (i = 0; i < cacheLen && i < pageCount; i++) {
			if (TestDestroy())
				break;
			target = curr + i;
			if (target > high)
				target = curr - (target - high);
			
			resampleLockers[target].Lock();
			
			if (!resamples[target].Ok()) {
				try {
					if (!originals[target].Ok()) {
						stream = ExtractStream(target);
						if (stream->IsOk() && stream->GetSize() > 0) {
							originals[target].LoadFile(*stream);
							// Memory Input Streams don't take ownership of the buffer
							mstream = dynamic_cast<wxMemoryInputStream*>(stream);
							if (mstream) {
								delete[] (wxUint8 *) mstream->GetInputStreamBuffer()->GetBufferStart();
							}
						}
						else {
							wxLogError(wxT("Failed to extract page %d."), target);
							originals[target] = wxImage(1, 1);
						}
						if (!originals[target].Ok()) {
							wxLogError(wxT("Failed to extract page %d."), target);
							originals[target] = wxImage(1, 1);
						}
						delete stream;
					}
				} catch (ArchiveException *ae) {
					wxLogError(ae->Message);
					wxLog::FlushActive();
				}
				ScaleImage(target);
				scalingHappened = true;
				
				thumbnailLockers[target].Lock();
				if (!thumbnails[target].Ok())
					ScaleThumbnail(target);
					
				if (!resamples[target].Ok() || !thumbnails[target].Ok())
					wxLogError(wxT("Could not scale page %d."), target);

				thumbnailLockers[target].Unlock();
				resampleLockers[target].Unlock();
				break;
			}

			resampleLockers[target].Unlock(); // the break above will put execution BELOW the next brace
		}
		
		if (!scalingHappened) {
			// if the cache is full, then use this iteration to fetch a
			// thumbnail, if needed.
			for (wxUint32 j = 0; j < pageCount; j++) {
				thumbnailLockers[j].Lock();
				if (!thumbnails[j].Ok()) {
					if (!originals[j].Ok()) {
						stream = ExtractStream(j);
						if (stream->IsOk() && stream->GetSize() > 0)
							originals[j].LoadFile(*stream);
						else {
							wxLogError(wxT("Failed to extract page %d."), j);
							originals[j] = wxImage(1, 1);
						}
						if (!originals[j].Ok()) {
							wxLogError(wxT("Failed to extract page %d."), j);
							originals[j] = wxImage(1, 1);
						}
						delete stream;
					}
					ScaleThumbnail(j);
					thumbnailLockers[j].Unlock();
					break;
				}
				thumbnailLockers[j].Unlock();
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
					
					if(originals[i].Ok())
						originals[i].Destroy();
				}
				
				for (i = pageCount - 1; wxInt32(i) > high; i--) {

					resampleLockers[i].Lock();

					if(resamples[i].Ok())
						resamples[i].Destroy();
					
					resampleLockers[i].Unlock();
					
					if(originals[i].Ok())
						originals[i].Destroy();
				}
			}
		}

		Yield();
		Sleep(20);
	}
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

	case THREEQ:
		scalingFactor = 0.75f;
		break;

	case HALF:
		scalingFactor = 0.5f;
		break;

	case ONEQ:
		scalingFactor = 0.25f;
		break;

	case FIT:
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

	case FITWIDTH:
		if (FitWithoutScrollbars(pagenumber, &scalingFactor)) {
			if (mode == TWOPAGE) {
				// check neighbor pages to see if they fit too
				if (pagenumber > 0) {
					if (!FitWithoutScrollbars(pagenumber - 1))
						goto fith_nofit;
				}
				if (pagenumber < pageCount - 1) {
					if (!FitWithoutScrollbars(pagenumber + 1))
						goto fith_nofit;
				}
			}
		} else {
			fith_nofit:
			rImage = float(xImage) / float(yImage);
			// fit with scrollbars
			if (rImage >= 1.0f || mode == ONEPAGE) {
				scalingFactor = float(canvasWidth - scrollBarThickness) / float(xImage);
			} else {
				scalingFactor = float ((canvasWidth - scrollBarThickness) / 2) / float(xImage);
			}
		}
		break;

	case FITHEIGHT: // fit to height
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
			scalingFactor = float(canvasHeight - scrollBarThickness) / float(yImage);
		}
		break;
		
	case FULL: // no resize
	default:
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
		return;
	}

	switch (Orientations[pagenumber]) {
	case NORTH:
		resamples[pagenumber] = FreeImage_Rescale(orig, wxInt32(xImage * scalingFactor), wxInt32(yImage * scalingFactor), fiFilter);
		break;
	case EAST:
		resamples[pagenumber] = FreeImage_Rescale(orig, wxInt32(yImage * scalingFactor), wxInt32(xImage * scalingFactor), fiFilter).Rotate90(true);
		break;
	case SOUTH:
		resamples[pagenumber] = FreeImage_Rescale(orig, wxInt32(xImage * scalingFactor), wxInt32(yImage * scalingFactor), fiFilter).Rotate90().Rotate90();
		break;
	case WEST:
		resamples[pagenumber] = FreeImage_Rescale(orig, wxInt32(yImage * scalingFactor), wxInt32(xImage * scalingFactor), fiFilter).Rotate90(false);
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
		thumbnails[pagenumber] = FreeImage_Rescale(orig, xScaled, yScaled, fiFilter);
		break;
	case EAST:
		thumbnails[pagenumber] = FreeImage_Rescale(orig, yScaled, xScaled, fiFilter).Rotate90(true);
		break;
	case SOUTH:
		thumbnails[pagenumber] = FreeImage_Rescale(orig, xScaled, yScaled, fiFilter).Rotate90().Rotate90();
		break;
	case WEST:
		thumbnails[pagenumber] = FreeImage_Rescale(orig, yScaled, xScaled, fiFilter).Rotate90(false);
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
	wxCommandEvent event(EVT_PAGE_SCALED, ID_PageScaled);
	event.SetInt(pagenumber);
	GetEventHandler()->AddPendingEvent(event);
}

void ComicBook::SendThumbnailedEvent(wxUint32 pagenumber)
{
	wxCommandEvent event(EVT_PAGE_THUMBNAILED, ID_PageThumbnailed);
	event.SetInt(pagenumber);
	GetEventHandler()->AddPendingEvent(event);
}

void ComicBook::SendCurrentPageChangedEvent()
{
	wxCommandEvent event(EVT_CURRENT_PAGE_CHANGED, -1);
	event.SetInt(this->GetCurrentPage());
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
