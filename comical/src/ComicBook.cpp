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
	delete[] Originals;
	delete[] Resamples;
	delete[] Orientations;
	delete[] imageProtectors;
}

void ComicBook::RotatePage(uint pagenumber, COMICAL_ROTATE direction)
{
	if (Orientations[pagenumber] != direction)
	{
		wxMutexLocker lock(imageProtectors[pagenumber]);
		Resamples[pagenumber].Destroy();
		Orientations[pagenumber] = direction;
	}
}

void ComicBook::SetParams(COMICAL_MODE newMode, FREE_IMAGE_FILTER newFilter, COMICAL_ZOOM newZoom, uint newWidth, uint newHeight)
{
	if(mode != newMode || fiFilter != newFilter || zoom != newZoom || width != newWidth || height != newHeight)
	{
		for (uint i = 0; i < pagecount; i++)
		{
			imageProtectors[i].Lock(); // race condition possible?
			Resamples[i].Destroy();
		}
		mode = newMode;
		fiFilter = newFilter;
		zoom = newZoom;
		width = newWidth;
		height = newHeight;
		for (uint i = 0; i < pagecount; i++)
			imageProtectors[i].Unlock();
	}
}

wxBitmap * ComicBook::GetPage(uint pagenumber)
{
	wxBusyCursor busy;
	wxLogVerbose("GetPage %i", pagenumber);
	// do I need to lock/condition the mutex somewhere in here?
	while (!Resamples[pagenumber].Ok())
		wxUsleep(50); // is this a good number (or a good idea?)
	return new wxBitmap(Resamples[pagenumber]);
}

void * ComicBook::Entry()
{
	uint i;
	int low, high, target, currentPage, pageBytes;

	wxLogVerbose("ComicBook thread started.");
	while (!TestDestroy())
	{
		currentPage = int(current); // in case this value changes midloop
		
		// The caching algorithm.  First calculates next highest
		// priority page, then checks to see if that page needs
		// updating.  If no pages need updating, yield the timeslice.
		
		if (cacheLen >= pagecount)
		{
			low = 0;
			high = pagecount - 1;
		}
		else
		{
			/* A moving window of size cacheLen.  2/3 of the pages in the
			 * cache are after the current page, the other 1/3 are before
			 * it. */
			high = 2 * cacheLen / 3;
			low = cacheLen - high;
			
			high = currentPage + high - 1;
			low = currentPage - low;

			/* Keep the window within 0 and pagecount. */
			if (high >= int(pagecount))
			{
				low -= (high - pagecount) + 1;
				high = pagecount - 1;
			}
			else if (low < 0)
			{
				high += low * -1;
				low = 0;
			}
		}

		for (i = 0; i < cacheLen && i < pagecount; i++)
		{
			if (TestDestroy())
				break;
			target = currentPage + i;
			if (target > high)
				target = currentPage - (target - high);
			
			imageProtectors[target].Lock();
			if (!Resamples[target].Ok())
			{
				if (!Originals[target].Ok())
				{
					wxLogVerbose("Loading page %i.", target);
					wxInputStream * is = ExtractStream(target);
					pageBytes = is->GetSize();
					if (is->IsOk() && is->GetSize() > 0)
					{
						wxLogVerbose("Got page %i, size %d bytes.", target, pageBytes);
						Originals[target].LoadFile(*is);
					}
					else
					{
						wxLogError("Couldn't extract page %i.", target);
						Originals[target] = new wxImage(1,1);
					}
				}
				ScaleImage(target);
				imageProtectors[target].Unlock();
				break;
			}
			imageProtectors[target].Unlock();
		}

		if (i < cacheLen && i < pagecount)
		{
			if (cacheLen < pagecount)
			{
				// Delete pages outside of the cache's range.
				for (i = 0; int(i) < low; i++)
				{
					imageProtectors[i].Lock();
					if(Resamples[i].Ok())
						Resamples[i].Destroy();
					if(Originals[i].Ok())
						wxLogVerbose("Destroying page %i.", i);
						Originals[i].Destroy();
					imageProtectors[i].Unlock();
				}
				
				for (i = pagecount - 1; int(i) > high; i--)
				{
					imageProtectors[i].Lock();
					if(Resamples[i].Ok())
						Resamples[i].Destroy();
					if(Originals[i].Ok())
						wxLogVerbose("Destroying page %i.", i);
						Originals[i].Destroy();
					imageProtectors[i].Unlock();
				}
			}
		}

		Sleep(20);
	}
	wxLogVerbose("ComicBook thread stopped.");
	return 0;
}

/* Resizes an image to fit within half of the Comical window. */
void ComicBook::ScaleImage(uint pagenumber)
{
	int xImage, yImage;
	float rCanvas, rImage;  // width/height ratios
	float scalingFactor;
  
	wxImage &orig = Originals[pagenumber];

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
		wxLogVerbose("scalingFactor = 0.75");
		scalingFactor = 0.75f;
		break;

	case HALF:
		wxLogVerbose("scalingFactor = 0.5");
		scalingFactor = 0.5f;
		break;

	case ONEQ:
		wxLogVerbose("scalingFactor = 0.25");
		scalingFactor = 0.25f;
		break;

	case FIT:
		rImage = float(xImage) / float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
		{
			rCanvas = float(width) / float(height);
			if (rCanvas > rImage)
				scalingFactor = float(height) / float(yImage);
			else
				scalingFactor = float(width) / float(xImage);
		}
		else
		{
			rCanvas = (float(width)/2.0f) / float(height);
			if (rCanvas > rImage)
				scalingFactor = float(height) / float(yImage);
			else
				scalingFactor = (float(width)/2.0f) / float(xImage);
		}

		wxLogVerbose("Image:%ix%i, Canvas:%ix%i, rCanvas:%f, rImage:%f, scalingFactor=%f", xImage, yImage, width, height, rCanvas, rImage, scalingFactor);
		break;

	case FITH: // fit horizontally

		rImage = float(xImage) / float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
			scalingFactor = float(width) / float(xImage);
		else
			scalingFactor = (float(width)/2.0f) / float(xImage);
		wxLogVerbose("Image:%ix%i, rImage:%f, scalingFactor=%f", xImage, yImage, rImage, scalingFactor);
		break;

	case FITV: // fit vertically

		scalingFactor = float(height) / float(yImage);
		wxLogVerbose("scalingFactor = %f", scalingFactor);
		break;

	case FULL: // no resize
	default:
		scalingFactor = 1.0f;
		wxLogVerbose("scalingFactor = %f", scalingFactor);
		break;
	}
	
	wxStartTimer();
	switch (Orientations[pagenumber])
	{
	case NORTH:
		Resamples[pagenumber] = FreeImage_Rescale(orig, int(xImage * scalingFactor), int(yImage * scalingFactor), fiFilter);
		break;
	case EAST:
		Resamples[pagenumber] = FreeImage_Rescale(orig, int(yImage * scalingFactor), int(xImage * scalingFactor), fiFilter).Rotate90(false);
		break;
	case SOUTH:
		Resamples[pagenumber] = FreeImage_Rescale(orig, int(xImage * scalingFactor), int(yImage * scalingFactor), fiFilter).Rotate90().Rotate90();
		break;
	case WEST:
		Resamples[pagenumber] = FreeImage_Rescale(orig, int(yImage * scalingFactor), int(xImage * scalingFactor), fiFilter).Rotate90(true);
		break;
	default:
		wxLogError("Undefined orientation %i.", Orientations[pagenumber]);
		break;
	}
	wxLogVerbose("Scaled image %i with filter %i in %ld milliseconds.", pagenumber, fiFilter, wxGetElapsedTime());

}
