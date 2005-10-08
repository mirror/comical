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

ComicBook::~ComicBook()
{
	delete[] originals;
	delete[] resamples;
	delete[] Orientations;
}

void ComicBook::RotatePage(uint pagenumber, COMICAL_ROTATE direction)
{
	if (Orientations[pagenumber] != direction)
	{
		if (IsRunning())
			Pause(); // pause the thread
		Orientations[pagenumber] = direction;
		resamples[pagenumber].Destroy();
		if (IsPaused())
			Resume();
	}
}

void ComicBook::SetParams(COMICAL_MODE newMode, FREE_IMAGE_FILTER newFilter, COMICAL_ZOOM newZoom, int newWidth, int newHeight, int newScrollBarThickness)
{
	if(mode != newMode || fiFilter != newFilter || zoom != newZoom || canvasWidth != newWidth || canvasHeight != newHeight)
	{
		if (IsRunning())
			Pause(); // pause the thread
		for (uint i = 0; i < pageCount; i++)
		{
			resamples[i].Destroy();
		}
		mode = newMode;
		fiFilter = newFilter;
		zoom = newZoom;
		canvasWidth = newWidth;
		canvasHeight = newHeight;
		scrollBarThickness = newScrollBarThickness;
		if (IsPaused())
			Resume();
	}
}

wxBitmap * ComicBook::GetPage(uint pagenumber)
{
	wxBusyCursor busy;
	while (!resamples[pagenumber].Ok())
	{
		Sleep(50);
	}
	return new wxBitmap(resamples[pagenumber]);
}

wxBitmap * ComicBook::GetPageLeftHalf(uint pagenumber)
{
	wxBusyCursor busy;
	while (!resamples[pagenumber].Ok())
	{
		Sleep(50);
	}
	int rWidth = resamples[pagenumber].GetWidth();
	int rHeight = resamples[pagenumber].GetHeight();
	return new wxBitmap(resamples[pagenumber].GetSubImage(wxRect(0, 0, rWidth / 2, rHeight)));
}

wxBitmap * ComicBook::GetPageRightHalf(uint pagenumber)
{
	wxBusyCursor busy;
	while (!resamples[pagenumber].Ok())
	{
		Sleep(50);
	}
	int rWidth = resamples[pagenumber].GetWidth();
	int rHeight = resamples[pagenumber].GetHeight();
	return new wxBitmap(resamples[pagenumber].GetSubImage(wxRect(rWidth / 2, 0, (rWidth / 2) + (rWidth % 2), rHeight)));
}

bool ComicBook::IsPageLandscape(uint pagenumber)
{
	if (pagenumber > pageCount)
		throw new PageOutOfRangeException(pagenumber, pageCount);
	wxBusyCursor busy;
	while (!resamples[pagenumber].Ok())
	{
		Sleep(50);
	}
	int rWidth = resamples[pagenumber].GetWidth();
	int rHeight = resamples[pagenumber].GetHeight();
	if ((float(rWidth)/float(rHeight)) > 1.0f)
		return true;
	else
		return false;
}

void * ComicBook::Entry()
{
	uint i;
	int low, high, target, currentPage, pageBytes;

	while (!TestDestroy())
	{
		currentPage = int(Current); // in case this value changes midloop
		
		// The caching algorithm.  First calculates next highest
		// priority page, then checks to see if that page needs
		// updating.  If no pages need updating, yield the timeslice.
		
		if (cacheLen >= pageCount)
		{
			low = 0;
			high = pageCount - 1;
		}
		else
		{
			/* A moving window of size cacheLen.  2/3 of the pages in the
			 * cache are after the Current page, the other 1/3 are before
			 * it. */
			high = 2 * cacheLen / 3;
			low = cacheLen - high;
			
			high = currentPage + high - 1;
			low = currentPage - low;

			/* Keep the window within 0 and pageCount. */
			if (high >= int(pageCount))
			{
				low -= (high - pageCount) + 1;
				high = pageCount - 1;
			}
			else if (low < 0)
			{
				high += low * -1;
				low = 0;
			}
		}

		for (i = 0; i < cacheLen && i < pageCount; i++)
		{
			if (TestDestroy())
				break;
			target = currentPage + i;
			if (target > high)
				target = currentPage - (target - high);
			
			if (!resamples[target].Ok())
			{
				if (!originals[target].Ok())
				{
					wxInputStream * is = ExtractStream(target);
					pageBytes = is->GetSize();
					if (is->IsOk() && is->GetSize() > 0)
					{
						originals[target].LoadFile(*is);
					}
					else
					{
						originals[target] = new wxImage(1,1);
					}
				}
				ScaleImage(target);
				if (!resamples[target].Ok())
					wxLogError("Failed to scale page %d.", target);
				break;
			}
		}

		if (i < cacheLen && i < pageCount)
		{
			if (cacheLen < pageCount)
			{
				// Delete pages outside of the cache's range.
				for (i = 0; int(i) < low; i++)
				{
					if(resamples[i].Ok())
						resamples[i].Destroy();
					if(originals[i].Ok())
						originals[i].Destroy();
				}
				
				for (i = pageCount - 1; int(i) > high; i--)
				{
					if(resamples[i].Ok())
						resamples[i].Destroy();
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
void ComicBook::ScaleImage(uint pagenumber)
{
	int xImage, yImage;
	float rCanvas, rImage;  // width/height ratios
	float scalingFactor;
  
	wxImage &orig = originals[pagenumber];

	if (Orientations[pagenumber] == NORTH || Orientations[pagenumber] == SOUTH)
	{
		xImage = orig.GetWidth();
		yImage = orig.GetHeight();
	}
	else // EAST || WEST
	{
		yImage = orig.GetWidth();
		xImage = orig.GetHeight();
	}
	
	switch(zoom)
	{

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
		if (rImage >= 1.0f || mode == SINGLE)
		{
			rCanvas = float(canvasWidth) / float(canvasHeight);
			if (rCanvas > rImage)
				scalingFactor = float(canvasHeight) / float(yImage);
			else
				scalingFactor = float(canvasWidth) / float(xImage);
		}
		else
		{
			rCanvas = (float(canvasWidth)/2.0f) / float(canvasHeight);
			if (rCanvas > rImage)
				scalingFactor = float(canvasHeight) / float(yImage);
			else
				scalingFactor = (float(canvasWidth)/2.0f) / float(xImage);
		}

		break;

	case FITH: // fit to width
		rImage = float(xImage) / float(yImage);
		// The page will have to be made narrower if it will not fit on the canvas without vertical scrolling
		int withoutScrollBarHeight;
		if (rImage >= 1.0f || mode == SINGLE) {
			scalingFactor = float(canvasWidth) / float(xImage);
			withoutScrollBarHeight = int(float(yImage) * scalingFactor);
			if (withoutScrollBarHeight > canvasHeight)
				scalingFactor = float(canvasWidth - scrollBarThickness) / float(xImage);
		}
		else {
			scalingFactor = float(canvasWidth/2) / float(xImage);
			withoutScrollBarHeight = int(float(yImage) * scalingFactor);
			if (withoutScrollBarHeight > canvasHeight)
				scalingFactor = float((canvasWidth - scrollBarThickness)/2) / float(xImage);
		}
		break;

	case FITV: // fit to height
		rImage = float(xImage) / float(yImage);
		scalingFactor = float(canvasHeight) / float(yImage);
		int withoutScrollBarWidth;
		// The page will have to be made shorter if it will not fit on the canvas without horizontal scrolling
		if (rImage >= 1.0f || mode == SINGLE) {
			withoutScrollBarWidth = int(float(xImage) * scalingFactor);
			if (withoutScrollBarWidth > canvasWidth)
				scalingFactor = float(canvasHeight - scrollBarThickness) / float(yImage);
		}
		else {
			withoutScrollBarWidth = int(float(xImage) * scalingFactor);
			if (withoutScrollBarWidth > (canvasWidth / 2))
				scalingFactor = float(canvasHeight - scrollBarThickness) / float(yImage);
		}
		break;

	case FULL: // no resize
	default:
		switch (Orientations[pagenumber])
		{
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

	switch (Orientations[pagenumber])
	{
	case NORTH:
		resamples[pagenumber] = FreeImage_Rescale(orig, int(xImage * scalingFactor), int(yImage * scalingFactor), fiFilter);
		break;
	case EAST:
		resamples[pagenumber] = FreeImage_Rescale(orig, int(yImage * scalingFactor), int(xImage * scalingFactor), fiFilter).Rotate90(true);
		break;
	case SOUTH:
		resamples[pagenumber] = FreeImage_Rescale(orig, int(xImage * scalingFactor), int(yImage * scalingFactor), fiFilter).Rotate90().Rotate90();
		break;
	case WEST:
		resamples[pagenumber] = FreeImage_Rescale(orig, int(yImage * scalingFactor), int(xImage * scalingFactor), fiFilter).Rotate90(false);
		break;
	default:
		break;
	}

}
