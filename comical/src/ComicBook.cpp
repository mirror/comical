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

ComicBook::ComicBook(wxString file) : wxThread(wxTHREAD_JOINABLE)
{
	pageCount = 0;
	Current = 0;
	filename = file;
	wxConfigBase *config = wxConfigBase::Get();
	// Each of the long values is followed by the letter L not the number one
	cacheLen = (wxUint32) config->Read(wxT("/Comical/CacheLength"), 10l); // Fit-to-Width is default
}

ComicBook::~ComicBook()
{
	wxConfigBase *config = wxConfigBase::Get();
	config->Write(wxT("/Comical/CacheLength"), (int) cacheLen);
	for(wxUint32 i = 0; i < pageCount; i++) {
		if (originals[i].Ok())
			originals[i].Destroy();
		if (resamples[i].Ok())
			resamples[i].Destroy();
	}
	delete[] originals;
	delete[] resamples;
	delete[] Orientations;
	delete[] imageLockers;
}

void ComicBook::RotatePage(wxUint32 pagenumber, COMICAL_ROTATE direction)
{
	if (Orientations[pagenumber] != direction) {
		wxMutexLocker lock(imageLockers[pagenumber]);
		Orientations[pagenumber] = direction;
		resamples[pagenumber].Destroy();
	}
}

/* returns TRUE if paramaters were different, FALSE if parameters were the same and no changes were made */
bool ComicBook::SetParams(COMICAL_MODE newMode, FREE_IMAGE_FILTER newFilter, COMICAL_ZOOM newZoom, wxInt32 newWidth, wxInt32 newHeight, wxInt32 newScrollBarThickness)
{
	if(mode != newMode || fiFilter != newFilter || zoom != newZoom || canvasWidth != newWidth || canvasHeight != newHeight || scrollBarThickness != newScrollBarThickness) {
		wxUint32 i;
		for (i = 0; i < pageCount; i++) {
			imageLockers[i].Lock();
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
			imageLockers[i].Unlock();
		return TRUE;
	} else
		return FALSE;

}

wxBitmap * ComicBook::GetPage(wxUint32 pagenumber)
{
wxLog::FlushActive();
	wxBusyCursor busy;
	while (!resamples[pagenumber].Ok()) {
		Sleep(50);
	}
	return new wxBitmap(resamples[pagenumber]);
}

wxBitmap * ComicBook::GetPageLeftHalf(wxUint32 pagenumber)
{
	wxBusyCursor busy;
	while (!resamples[pagenumber].Ok()) {
		Sleep(50);
	}
	wxInt32 rWidth = resamples[pagenumber].GetWidth();
	wxInt32 rHeight = resamples[pagenumber].GetHeight();
	return new wxBitmap(resamples[pagenumber].GetSubImage(wxRect(0, 0, rWidth / 2, rHeight)));
}

wxBitmap * ComicBook::GetPageRightHalf(wxUint32 pagenumber)
{
	wxBusyCursor busy;
	while (!resamples[pagenumber].Ok()) {
		Sleep(50);
	}
	wxInt32 rWidth = resamples[pagenumber].GetWidth();
	wxInt32 rHeight = resamples[pagenumber].GetHeight();
	return new wxBitmap(resamples[pagenumber].GetSubImage(wxRect(rWidth / 2, 0, (rWidth / 2) + (rWidth % 2), rHeight)));
}

bool ComicBook::IsPageLandscape(wxUint32 pagenumber)
{
	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);
	wxBusyCursor busy;
	while (!resamples[pagenumber].Ok()) {
		Sleep(50);
	}
	wxInt32 rWidth = resamples[pagenumber].GetWidth();
	wxInt32 rHeight = resamples[pagenumber].GetHeight();
	if ((float(rWidth)/float(rHeight)) > 1.0f)
		return true;
	else
		return false;
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
	wxInt32 low, high, target, currentPage, pageBytes;

	while (!TestDestroy()) {
		currentPage = wxInt32(Current); // in case this value changes midloop
		
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
			
			high = currentPage + high - 1;
			low = currentPage - low;

			/* Keep the window within 0 and pageCount. */
			if (high >= wxInt32(pageCount)) {
				low -= (high - pageCount) + 1;
				high = pageCount - 1;
			} else if (low < 0) {
				high += low * -1;
				low = 0;
			}
		}

		for (i = 0; i < cacheLen && i < pageCount; i++) {
			if (TestDestroy())
				break;
			target = currentPage + i;
			if (target > high)
				target = currentPage - (target - high);
			
			imageLockers[target].Lock();
			
			if (!resamples[target].Ok()) {
				try {
					if (!originals[target].Ok()) {
						wxInputStream * is = ExtractStream(target);
						pageBytes = is->GetSize();
						if (is->IsOk() && is->GetSize() > 0)
							originals[target].LoadFile(*is);
						else {
							wxLogError(wxT("Failed to extract page %d."), target);
							originals[target] = wxImage(1, 1);
						}
						if (!originals[target].Ok()) {
							wxLogError(wxT("Failed to extract page %d."), target);
							originals[target] = wxImage(1, 1);
						}
					}
				} catch (ArchiveException &ae) {
					wxLogError(ae.Message);
					wxLog::FlushActive();
				}
				ScaleImage(target);
				if (!resamples[target].Ok())
					wxLogError(wxT("Failed to scale page %d."), target);
				imageLockers[target].Unlock();
				break;
			}

			imageLockers[target].Unlock();
		}

		if (i < cacheLen && i < pageCount) {
			if (cacheLen < pageCount) {
				// Delete pages outside of the cache's range.
				for (i = 0; wxInt32(i) < low; i++) {
					
					imageLockers[i].Lock();
					
					if(resamples[i].Ok())
						resamples[i].Destroy();
					
					imageLockers[i].Unlock();
					
					if(originals[i].Ok())
						originals[i].Destroy();
				}
				
				for (i = pageCount - 1; wxInt32(i) > high; i--) {

					imageLockers[i].Lock();

					if(resamples[i].Ok())
						resamples[i].Destroy();
					
					imageLockers[i].Unlock();
					
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
	default:
		break;
	}

}
