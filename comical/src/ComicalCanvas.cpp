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
#include "ComicalApp.h"

IMPLEMENT_DYNAMIC_CLASS(ComicalCanvas, wxScrolledWindow)

#if wxCHECK_VERSION(2, 5, 1)
ComicalCanvas::ComicalCanvas(wxWindow *prnt, const wxPoint &pos, const wxSize &size) : wxScrolledWindow(prnt, -1, pos, size, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE)
#else
ComicalCanvas::ComicalCanvas(wxWindow *prnt, const wxPoint &pos, const wxSize &size) : wxScrolledWindow(prnt, -1, pos, size, wxNO_BORDER)
#endif
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
	EVT_PAINT(ComicalCanvas::OnPaint)
	EVT_KEY_DOWN(ComicalCanvas::OnKeyDown)
	EVT_SIZE(ComicalCanvas::OnSize)
END_EVENT_TABLE()

ComicalCanvas::~ComicalCanvas()
{
	clearBitmaps();
	wxConfigBase *config = wxConfigBase::Get();
	config->Write("/Comical/Zoom", zoom);
	config->Write("/Comical/Mode", mode);
	config->Write("/Comical/Filter", filter);
}

void ComicalCanvas::clearBitmap(wxBitmap *&bitmap)
{
	if (bitmap && bitmap->Ok())
	{
		delete bitmap;
		bitmap = NULL;
	}
}

void ComicalCanvas::clearBitmaps()
{
	clearBitmap(leftPage);
	clearBitmap(centerPage);
	clearBitmap(rightPage);
}

void ComicalCanvas::FirstPage()
{
	wxBitmap *bitmap;
	int xImage, yImage;
	float rImage;
	
	if (theBook == NULL)
		return;
	setPage(0);
	
	bitmap = theBook->GetPage(0);

	if (bitmap->Ok())
	{
		clearBitmaps();
		xImage = bitmap->GetWidth();
		yImage = bitmap->GetHeight();

		// Here we assume that portrait pages are single pages and that landscape
		// pages are double pages.
		rImage = float(xImage)/float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
			centerPage = bitmap;
		else
			rightPage = bitmap;
		
		createBitmaps();
	}
}

void ComicalCanvas::LastPage()
{
	wxBitmap *bitmap;
	int xImage, yImage;
	float rImage;
	
	if (theBook == NULL) return;
	setPage(theBook->pagecount - 1);
	
	bitmap = theBook->GetPage(theBook->current);

	if (bitmap->Ok())
	{
		clearBitmaps();
		xImage = bitmap->GetWidth();
		yImage = bitmap->GetHeight();

		// Here we assume that portrait pages are single pages and that landscape
		// pages are double pages.
		rImage = float(xImage)/float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
			centerPage = bitmap;
		else
			leftPage = bitmap;

		createBitmaps();

	}
}

void ComicalCanvas::GoToPage(int pagenumber)
{
	wxBitmap *bitmap;
	int xImage, yImage;
	float rImage;
	
	if (theBook == NULL) return;
	if (pagenumber >= int(theBook->pagecount) || pagenumber < 0) return;
	setPage(pagenumber);
	
	bitmap = theBook->GetPage(theBook->current);

	if (bitmap)	if (bitmap->Ok())
	{
		clearBitmaps();
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

		createBitmaps();

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
		setPage(theBook->current - 1);
	else
		setPage(theBook->current - 2);

	bitmap = theBook->GetPage(theBook->current);

	if (bitmap->Ok())
	{
		clearBitmaps();
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
						clearBitmap(bitmap);
				}
			}
		}

		createBitmaps();

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

	setPage(theBook->current + 1);
	bitmap = theBook->GetPage(theBook->current);

	if (bitmap->Ok())
	{
		clearBitmaps();
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
				setPage(theBook->current + 1);
				bitmap = theBook->GetPage(theBook->current);
				if (bitmap->Ok())
				{
					xImage = bitmap->GetWidth();
					yImage = bitmap->GetHeight();
					rImage = float(xImage)/float(yImage);
					if (rImage < 1.0f) // Only if this page is also not a double do we display it
						rightPage = bitmap;
					else
					{
						setPage(theBook->current - 1);
						clearBitmap(bitmap);
					}
				}
			}
		}

		createBitmaps();

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
	if (theBook->current == 1)
	{
		FirstPage();
		return;
	}
	if (centerPage)
	{
		PrevPageTurn();
		return;
	}

	setPage(theBook->current - 1);

	if (mode == SINGLE)
		bitmap = theBook->GetPage(theBook->current);
	else
		bitmap = theBook->GetPage(theBook->current - 1);

	if (bitmap->Ok())
	{

		clearBitmap(rightPage);
		clearBitmap(centerPage);

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
				setPage(theBook->current - 1);
				clearBitmap(leftPage);
			}
		}
		else if (leftPage && leftPage->Ok())
		{
			rightPage = leftPage;
			leftPage = bitmap;
		}

		createBitmaps();

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
	setPage(theBook->current + 1);
	
	bitmap = theBook->GetPage(theBook->current);
	
	if (bitmap->Ok())
	{
		clearBitmap(leftPage);
		clearBitmap(centerPage);

		xImage = bitmap->GetWidth();
		yImage = bitmap->GetHeight();

		// Here we assume that portrait pages are single pages and that landscape
		// pages are double pages.
		rImage = float(xImage)/float(yImage);
		if (rImage >= 1.0f || mode == SINGLE)
		{
			centerPage = bitmap;
			clearBitmap(rightPage);
		}
		else if (rightPage && rightPage->Ok())
		{
			leftPage = rightPage;
			rightPage = bitmap;
		}

		createBitmaps();

	}
}

void ComicalCanvas::createBitmaps()
{
	int xScroll = 0, yScroll = 0, xWindow, yWindow;
	bool left = false, right = false;
	
	GetClientSize(&xWindow, &yWindow);

	ComicalFrame *cParent = (ComicalFrame *) parent;

	if (centerPage && centerPage->Ok())
	{
		xScroll = centerPage->GetWidth();
		yScroll = centerPage->GetHeight();

		cParent->menuView->FindItem(ID_RotateLeft)->Enable(false);
		cParent->menuView->FindItem(ID_RotateRight)->Enable(false);
		cParent->menuView->FindItem(ID_Rotate)->Enable(true);
		cParent->menuRotate->FindItemByPosition(theBook->Orientations[theBook->current])->Check();
		cParent->toolBarNav->EnableTool(ID_CCWL, false);
		cParent->toolBarNav->EnableTool(ID_CWL, false);
		cParent->toolBarNav->EnableTool(ID_CCW, true);
		cParent->toolBarNav->EnableTool(ID_CW, true);
	}
	else
	{
		cParent->menuView->FindItem(ID_Rotate)->Enable(false);

		if (rightPage && (right = rightPage->Ok()))
		{
			xScroll += rightPage->GetWidth();
			yScroll = (rightPage->GetHeight() > yScroll) ? rightPage->GetHeight() : yScroll;

			cParent->menuView->FindItem(ID_RotateRight)->Enable(true);
			cParent->menuRotateRight->FindItemByPosition(theBook->Orientations[theBook->current])->Check();
			cParent->toolBarNav->EnableTool(ID_CCW, true);
			cParent->toolBarNav->EnableTool(ID_CW, true);
		}
		else
		{
			cParent->menuView->FindItem(ID_RotateRight)->Enable(false);
			cParent->toolBarNav->EnableTool(ID_CCW, false);
			cParent->toolBarNav->EnableTool(ID_CW, false);
		}
	
		if (leftPage && (left = leftPage->Ok()))
		{
			xScroll += leftPage->GetWidth();
			yScroll = leftPage->GetHeight();

			cParent->menuView->FindItem(ID_RotateLeft)->Enable(true);
			cParent->menuRotateLeft->FindItemByPosition(theBook->Orientations[theBook->current - 1])->Check();
			cParent->toolBarNav->EnableTool(ID_CCWL, true);
			cParent->toolBarNav->EnableTool(ID_CWL, true);
		}
		else
		{
			cParent->menuView->FindItem(ID_RotateLeft)->Enable(false);
			cParent->toolBarNav->EnableTool(ID_CCWL, false);
			cParent->toolBarNav->EnableTool(ID_CWL, false);
		}

		if (!left || !right) // if only one page is active
			xScroll *= 2;
	}

	cParent->toolBarNav->Realize();
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
	GetClientSize(&xCanvas, &yCanvas); // Client Size is the visible area
	x = xCanvas;
	y = yCanvas;
	theBook->SetParams(mode, filter, zoom, xCanvas, yCanvas);
}

void ComicalCanvas::Rotate(bool clockwise)
{
	COMICAL_ROTATE direction = theBook->Orientations[theBook->current];
	if (clockwise)
	{
		switch (direction)
		{
		case NORTH:
			theBook->RotatePage(theBook->current, EAST);
			break;
		case EAST:
			theBook->RotatePage(theBook->current, SOUTH);
			break;
		case SOUTH:
			theBook->RotatePage(theBook->current, WEST);
			break;
		case WEST:
			theBook->RotatePage(theBook->current, NORTH);
			break;
		}
	}
	else
	{
		switch (direction)
		{
		case NORTH:
			theBook->RotatePage(theBook->current, WEST);
			break;
		case EAST:
			theBook->RotatePage(theBook->current, NORTH);
			break;
		case SOUTH:
			theBook->RotatePage(theBook->current, EAST);
			break;
		case WEST:
			theBook->RotatePage(theBook->current, SOUTH);
			break;
		}
	}
	GoToPage(theBook->current);
}

void ComicalCanvas::Rotate(COMICAL_ROTATE direction)
{
	theBook->RotatePage(theBook->current, direction);
	GoToPage(theBook->current);
}

void ComicalCanvas::RotateLeft(bool clockwise)
{
	if(theBook->current > 0)
	{
		COMICAL_ROTATE direction = theBook->Orientations[theBook->current - 1];
		if (clockwise)
		{
			switch (direction)
			{
			case NORTH:
				theBook->RotatePage(theBook->current - 1, EAST);
				break;
			case EAST:
				theBook->RotatePage(theBook->current - 1, SOUTH);
				break;
			case SOUTH:
				theBook->RotatePage(theBook->current - 1, WEST);
				break;
			case WEST:
				theBook->RotatePage(theBook->current - 1, NORTH);
				break;
			}
		}
		else
		{
			switch (direction)
			{
			case NORTH:
				theBook->RotatePage(theBook->current - 1, WEST);
				break;
			case EAST:
				theBook->RotatePage(theBook->current - 1, NORTH);
				break;
			case SOUTH:
				theBook->RotatePage(theBook->current - 1, EAST);
				break;
			case WEST:
				theBook->RotatePage(theBook->current - 1, SOUTH);
				break;
			}
		}
		GoToPage(theBook->current - 1);
	}
}

void ComicalCanvas::RotateLeft(COMICAL_ROTATE direction)
{
	if(theBook->current > 0)
	{
		theBook->RotatePage(theBook->current - 1, direction);
		GoToPage(theBook->current - 1);
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
	{
		dc.DrawBitmap(*centerPage, (xCanvas/2) - centerPage->GetWidth()/2, 0, false);
	}
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

	case WXK_BACK:
		PrevPageSlide();
		break;

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

void ComicalCanvas::OnSize(wxSizeEvent& event)
{
	ComicalFrame *cParent = (ComicalFrame *) parent;
	wxSize canvasSize = GetClientSize();
	wxSize toolBarSize = cParent->toolBarNav->GetSize();
	wxSize progressSize = cParent->progress->GetSize();
	int tbxPos = (canvasSize.x - toolBarSize.x) / 2;
	cParent->toolBarNav->SetSize(tbxPos, canvasSize.y + progressSize.y, toolBarSize.x, -1);
	cParent->progress->SetSize(0, canvasSize.y, canvasSize.x, 10);
}

void ComicalCanvas::setPage(int pagenumber)
{
	theBook->current = pagenumber;
	ComicalFrame *cParent = (ComicalFrame *) parent;
	cParent->progress->SetValue(pagenumber);
}