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

#include "ComicPage.h"

ComicPage::ComicPage()
{
	ready = false;
}

ComicPage::ComicPage(wxInputStream * source)
{
	ready = false;
	Create(source);
}

void ComicPage::Create(wxInputStream * source)
{
	wxLogVerbose("Loading image from wxInputStream...");
	if (original.LoadFile(*source))
		ready = true;
	else
		wxLogError("Successfully extracted the file, but could not create an image from it.  It may be corrupted.");
}

wxBitmap * ComicPage::GetPage(COMICAL_ROTATE rotate, FREE_IMAGE_FILTER fi_filter, int width, int height)
{
	// if the requested image has the same properties as the original, just return the original
	if (rotate == NORTH && width == original.GetWidth() && height == original.GetHeight()) {
		return new wxBitmap(original);
	} else if (!resampled.Ok() ||
			resampled.GetWidth() != width ||
			resampled.GetHeight() != height ||
			filter != fi_filter ||
			direction != rotate) {
		switch (rotate) {
		case NORTH:
			ScaleImage(original, width, height, fi_filter);
			break;
		case EAST:
			ScaleImage(original.Rotate90(false), width, height, fi_filter);
			break;
		case SOUTH:
			ScaleImage(original.Rotate90().Rotate90(), width, height, fi_filter);
			break;
		case WEST:
			ScaleImage(original.Rotate90(true), width, height, fi_filter);
			break;
		}
		filter = fi_filter;
		direction = rotate;
		return new wxBitmap(resampled);
	}
	return NULL;
}

void ComicPage::ScaleImage(wxImage src, uint width, uint height, FREE_IMAGE_FILTER fi_filter)
{
	wxStartTimer();
	resampled = FreeImage_Rescale(src, width, height, fi_filter);
	wxLogVerbose("Scaled image with filter %i in %ld milliseconds.", fi_filter, wxGetElapsedTime());
}
