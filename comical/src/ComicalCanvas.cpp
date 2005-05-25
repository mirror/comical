/***************************************************************************
              ComicalCanvas.cpp - ComicalCanvas implementation
                             -------------------
    begin                : Thu Dec 18 2003
    copyright            : (C) 2005 by James Athey
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

#include "ComicalCanvas.h"

IMPLEMENT_DYNAMIC_CLASS(ComicalCanvas, wxScrolledWindow)

ComicalCanvas::ComicalCanvas( wxWindow *prnt, wxWindowID id, const wxPoint &pos, const wxSize &size ) : wxScrolledWindow( prnt, id, pos, size, wxSUNKEN_BORDER )
{
	parent = prnt;
	SetBackgroundColour(* wxBLACK);
	wxConfigBase *config = wxConfigBase::Get();
	// Each of the long values is followed by the letter L not the number one
	zoom = (COMICAL_ZOOM) config->Read("/Comical/Zoom", 2l); // Fit-to-Width is default
	mode = (COMICAL_MODE) config->Read("/Comical/Mode", 1l); // Double-Page is default
	filter = (FREE_IMAGE_FILTER) config->Read("/Comical/Filter", 4l); // Catmull-Rom is default
	leftPage = rightPage = centerPage = NULL;
	x = -1; y = -1;
}

BEGIN_EVENT_TABLE(ComicalCanvas, wxScrolledWindow)
	EVT_SIZE(ComicalCanvas::OnSize)
	EVT_PAINT(ComicalCanvas::OnPaint)
	EVT_KEY_DOWN(ComicalCanvas::OnKeyDown)
END_EVENT_TABLE()

ComicalCanvas::~ComicalCanvas()
{
	ClearBitmaps();
	wxConfigBase *config = wxConfigBase::Get();
	config->Write("/Comical/Zoom", zoom);
	config->Write("/Comical/Mode", mode);
	config->Write("/Comical/Filter", filter);
}

void ComicalCanvas::ClearBitmap(wxBitmap *&bitmap)
{
	if (bitmap && bitmap->Ok())
	{
		delete bitmap;
		bitmap = NULL;
	}
}

void ComicalCanvas::ClearBitmaps()
{
	ClearBitmap(leftPage);
	ClearBitmap(centerPage);
	ClearBitmap(rightPage);
}

void ComicalCanvas::FirstPage()
{
	wxBitmap *bitmap;
	int xImage, yImage;
	float rImage;
	
	if (theBook == NULL)
		return;
	theBook->current = 0;
	
	bitmap = theBook->GetPage(0);

	if (bitmap->Ok())
	{
		ClearBitmaps();
		xImage = bitmap->GetWidth();
		yImage = bitmap->GetHeight();

		// Here we assume that portrait pages are single pages and that landscape
		// pages are double pages.
		rImage = float(xImage)/float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
			centerPage = bitmap;
		else
			rightPage = bitmap;
		
		CreateBitmaps();
	}
}

void ComicalCanvas::LastPage()
{
	wxBitmap *bitmap;
	int xImage, yImage;
	float rImage;
	
	if (theBook == NULL) return;
	theBook->current = theBook->pagecount - 1;
	
	bitmap = theBook->GetPage(theBook->current);

	if (bitmap->Ok())
	{
		ClearBitmaps();
		xImage = bitmap->GetWidth();
		yImage = bitmap->GetHeight();

		// Here we assume that portrait pages are single pages and that landscape
		// pages are double pages.
		rImage = float(xImage)/float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
			centerPage = bitmap;
		else
			leftPage = bitmap;

		CreateBitmaps();

	}
}

void ComicalCanvas::GoToPage(int pagenumber)
{
	wxBitmap *bitmap;
	int xImage, yImage;
	float rImage;
	
	if (theBook == NULL) return;
	if (pagenumber >= int(theBook->pagecount) || pagenumber < 0) return;
	theBook->current = pagenumber;
	
	bitmap = theBook->GetPage(theBook->current);

	if (bitmap)	if (bitmap->Ok())
	{
		ClearBitmaps();
		xImage = bitmap->GetWidth();
		yImage = bitmap->GetHeight();

		// Here we assume that portrait pages are single pages and that landscape
		// pages are double pages.
		rImage = float(xImage)/float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
			centerPage = bitmap;
		else
		{
			rightPage = bitmap;
			if (theBook->current > 0)
			{
				bitmap = theBook->GetPage(theBook->current - 1);
				if (bitmap->Ok())
				{
					xImage = bitmap->GetWidth();
					yImage = bitmap->GetHeight();
					rImage = float(xImage)/float(yImage);
					if (rImage < 1.0f) // Only if this page is also not a double do we display it
						leftPage = bitmap;
				}
			}
		}

		CreateBitmaps();

	}
}

void ComicalCanvas::PrevPageTurn()
{
	wxBitmap *bitmap;
	int xImage, yImage;
	float rImage;

	if (theBook == NULL) return;
	if (theBook->current <= 0) return;
	if (!leftPage || !rightPage || theBook->current == 1)
		theBook->current -= 1;
	else
		theBook->current -= 2;

	bitmap = theBook->GetPage(theBook->current);

	if (bitmap->Ok())
	{
		ClearBitmaps();
		xImage = bitmap->GetWidth();
		yImage = bitmap->GetHeight();

		// Here we assume that portrait pages are single pages and that landscape
		// pages are double pages.

		rImage = float(xImage)/float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
		{
			centerPage = bitmap;
		}
		else
		{
			rightPage = bitmap;
			if (theBook->current > 0)
			{
				bitmap = theBook->GetPage(theBook->current - 1);
				if (bitmap->Ok())
				{
					xImage = bitmap->GetWidth();
					yImage = bitmap->GetHeight();
					rImage = float(xImage)/float(yImage);
					if (rImage < 1.0f) // Only if this page is also not a double do we display it
						leftPage = bitmap;
					else
						ClearBitmap(bitmap);
				}
			}
		}

		CreateBitmaps();

	}
}

void ComicalCanvas::NextPageTurn()
{
	wxBitmap *bitmap;
	int xImage, yImage;
	float rImage;

	if (theBook == NULL)
		return;
	if (theBook->current >= theBook->pagecount - 1)
		return;
	if (theBook->current == theBook->pagecount - 2)
	{
		NextPageSlide();
		return;
	}

	theBook->current++;
	bitmap = theBook->GetPage(theBook->current);

	if (bitmap->Ok())
	{
		ClearBitmaps();
		xImage = bitmap->GetWidth();
		yImage = bitmap->GetHeight();

		// Here we assume that portrait pages are single pages and that landscape
		// pages are double pages.

		rImage = float(xImage)/float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
			centerPage = bitmap;
		else
		{
			leftPage = bitmap;
			if (theBook->current + 1 < theBook->pagecount)
			{
				bitmap = theBook->GetPage(++theBook->current);
				if (bitmap->Ok())
				{
					xImage = bitmap->GetWidth();
					yImage = bitmap->GetHeight();
					rImage = float(xImage)/float(yImage);
					if (rImage < 1.0f) // Only if this page is also not a double do we display it
						rightPage = bitmap;
					else
					{
						theBook->current--;
						ClearBitmap(bitmap);
					}
				}
			}
		}

		CreateBitmaps();

	}
}

void ComicalCanvas::PrevPageSlide()
{
	wxBitmap *bitmap;
	int xImage, yImage;
	float rImage;

	if (theBook == NULL)
		return;
	if (theBook->current <= 0)
		return;
	if (centerPage)
	{
		PrevPageTurn();
		return;
	}

	theBook->current--;

	if (mode == SINGLE)
		bitmap = theBook->GetPage(theBook->current);
	else
		bitmap = theBook->GetPage(theBook->current - 1);

	if (bitmap->Ok())
	{

		ClearBitmap(rightPage);
		ClearBitmap(centerPage);

		xImage = bitmap->GetWidth();
		yImage = bitmap->GetHeight();

		// Here we assume that portrait pages are single pages and that landscape
		// pages are double pages.
		rImage = float(xImage)/float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
		{
			centerPage = bitmap;
			if (mode == DOUBLE)
			{
				theBook->current--;
				ClearBitmap(leftPage);
			}
		}
		else if (leftPage && leftPage->Ok())
		{
			rightPage = leftPage;
			leftPage = bitmap;
		}

		CreateBitmaps();

	}
}

void ComicalCanvas::NextPageSlide()
{
	wxBitmap *bitmap;
	int xImage, yImage;
	float rImage;

	if (theBook == NULL)
		return;
	if (theBook->current >= theBook->pagecount - 1)
		return;
	if (centerPage && centerPage->Ok() && theBook->current < theBook->pagecount - 1)
	{
			NextPageTurn();
			return;
	}
	theBook->current++;
	
	bitmap = theBook->GetPage(theBook->current);
	
	if (bitmap->Ok())
	{
		ClearBitmap(leftPage);
		ClearBitmap(centerPage);

		xImage = bitmap->GetWidth();
		yImage = bitmap->GetHeight();

		// Here we assume that portrait pages are single pages and that landscape
		// pages are double pages.
		rImage = float(xImage)/float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
		{
			centerPage = bitmap;
			ClearBitmap(rightPage);
		}
		else if (rightPage && rightPage->Ok())
		{
			leftPage = rightPage;
			rightPage = bitmap;
		}

		CreateBitmaps();

	}
}

void ComicalCanvas::CreateBitmaps()
{
	int xScroll = 0, yScroll = 0, xWindow, yWindow;
	bool left = false, right = false;
	
	GetClientSize(&xWindow, &yWindow);
	
	if (centerPage && centerPage->Ok())
	{
		xScroll = centerPage->GetWidth();
		yScroll = centerPage->GetHeight();
	}
	else
	{
		if (leftPage) if ((left = leftPage->Ok()))
		{
			xScroll += leftPage->GetWidth();
			yScroll = leftPage->GetHeight();
		}
		if (rightPage) if ((right = rightPage->Ok()))
		{
			xScroll += rightPage->GetWidth();
			yScroll = (rightPage->GetHeight() > yScroll) ? rightPage->GetHeight() : yScroll;
		}
		if (!left || !right) // if only one page is active
			xScroll *= 2;
	}

	SetScrollbars(10, 10, xScroll / 10, yScroll / 10);
	Scroll((xScroll / 20) - (xWindow / 20), 0);
	Refresh();

}

void ComicalCanvas::Zoom(COMICAL_ZOOM value)
{
	zoom = value;
	if (zoom == FITH)
		EnableScrolling(false, true); // Horizontal fit, no horizontal scrolling
	else if (zoom == FITV)
		EnableScrolling(true, false); // Vertical fit, no vertical scrolling
	else if (zoom == FIT)
		EnableScrolling(false, false); // Fit, no scrolling
	else
		EnableScrolling(true, true);	
	if (theBook)
	{
		SetParams();
		GoToPage(theBook->current);
	}
}

void ComicalCanvas::Filter(FREE_IMAGE_FILTER value)
{
	filter = value;
	if (theBook)
	{
		SetParams();
		GoToPage(theBook->current);
	}
}

void ComicalCanvas::Mode(COMICAL_MODE newmode)
{
	mode = newmode;
	if (theBook)
	{
		SetParams();
		GoToPage(theBook->current);
	}
}

void ComicalCanvas::SetParams()
{
	int xCanvas, yCanvas;
	wxLogMessage("SetParams");
	GetClientSize(&xCanvas, &yCanvas); // Client Size is the visible area
	x = xCanvas;
	y = yCanvas;
	theBook->SetParams(mode, filter, zoom, xCanvas, yCanvas);
}

void ComicalCanvas::Rotate(COMICAL_ROTATE rotate)
{
	switch (rotate)
	{
	case NORTH:
	
	case EAST:
	
	case SOUTH:
	
	case WEST:
	
	default:
		break;
	}
}

void ComicalCanvas::OnSize(wxSizeEvent &event)
{
	if (theBook != NULL)
	{
		SetParams();
		GoToPage(theBook->current);
	}
}

void ComicalCanvas::OnPaint(wxPaintEvent &WXUNUSED(event))
{
	int xCanvas, yCanvas;

	wxPaintDC dc(this);
	PrepareDC(dc);

	GetClientSize(&xCanvas, &yCanvas);
	/* I can't properly initialize x and y until the constructors are done, but
		 if I just leave the values uninitialized, the very first image will be
		 resized twice.	Here, when OnPaint is called for the first time, the values
		 will be initialized.	*/
	if (x == -1 && y == -1)
	{
		x = xCanvas;
		y = yCanvas;
	}

	if (zoom == FIT || zoom == FITH || zoom == FITV)
	{
		if (xCanvas != x || yCanvas != y)
		{
			x = xCanvas;
			y = yCanvas;
			SetParams();
			GoToPage(theBook->current);
			return;
		}
	}

	if (centerPage && centerPage->Ok())
			dc.DrawBitmap(*centerPage, (xCanvas/2) - centerPage->GetWidth()/2, 0, false);
	else
	{
		if (leftPage && leftPage->Ok())
			dc.DrawBitmap(*leftPage, xCanvas/2 - leftPage->GetWidth(), 0, false);
		if (rightPage && rightPage->Ok())
			dc.DrawBitmap(*rightPage, xCanvas/2, 0, false);
	}

	SetFocus(); // This is so we can grab keydown events

}

void ComicalCanvas::OnKeyDown(wxKeyEvent& event)
{
	switch(event.GetKeyCode())
	{

	case WXK_PRIOR:
		PrevPageTurn();
		break;

	case WXK_NEXT:
		NextPageTurn();
		break;

	case WXK_LEFT:
		PrevPageSlide();
		break;

	case WXK_RIGHT:
	case WXK_SPACE:
		NextPageSlide();
		break;

	case WXK_HOME:
		FirstPage();
		break;

	case WXK_END:
		LastPage();
		break;

	default:
		event.Skip();
	}
}
