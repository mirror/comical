/***************************************************************************
 *   Copyright (C) 2004 by James Leighton Athey                            *
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

#ifndef _ComicPage_h_
#define _ComicPage_h_

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/image.h>
#include <wx/bitmap.h>
#endif

#include "Resize.h"

enum COMICAL_ROTATE {NORTH, EAST, SOUTH, WEST};

class ComicPage
{

public:
	ComicPage();
	ComicPage(wxInputStream * src);
	~ComicPage();
	
	void Scale(COMICAL_ROTATE direction, FREE_IMAGE_FILTER fi_filter, uint width, uint height);
	bool IsReady() {return ready};
	wxBitmap *GetPage(COMICAL_ROTATE direction, FREE_IMAGE_FITER fi_filter, uint width, uint height);
	
	wxImage original;
	
protected:
	wxImage resampled;
	COMICAL_ROTATE direction;
	FREE_IMAGE_FILTER filter;
	bool ready;
}

#endif
