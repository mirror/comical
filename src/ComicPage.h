/*
 * ComicPage.h
 * Copyright (c) 2006-2011, James Athey
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

#ifndef _ComicPage_h_
#define _ComicPage_h_

#include <wx/string.h>
#include <wx/image.h>
#include <wx/dynarray.h>
#include <wx/thread.h>
#include <wx/stream.h>
#include <wx/bitmap.h>

#include "enums.h"

class ComicPage
{
public:

	ComicPage(wxString &filename, wxInputStream *headerStream);
	~ComicPage();
	void Rotate(COMICAL_ROTATE direction);
	
	void DestroyAll();
	void DestroyOriginal();
	void DestroyResample();
	void DestroyThumbnail();

	wxUint32 GetWidth() const { return m_uiWidth; }
	wxUint32 GetHeight() const { return m_uiHeight; }
	
	wxBitmap& GetPage();
	wxBitmap& GetPageLeftHalf();
	wxBitmap& GetPageRightHalf();
	wxBitmap& GetThumbnail();

	bool IsLandscape();

	COMICAL_ROTATE GetOrientation() const { return orientation; }

	wxBitmapType GetBitmapType() const { return m_bitmapType; }
	
	const wxString Filename;

	wxImage Original;
	wxImage Resample;
	wxImage Thumbnail;
	wxMutex OriginalLock;
	wxMutex ResampleLock;
	wxMutex ThumbnailLock;

private:
	void extractDimensions(wxInputStream *stream);

	COMICAL_ROTATE orientation;

	wxUint32 m_uiWidth;
	wxUint32 m_uiHeight;

	wxBitmapType m_bitmapType;

	wxBitmap m_bitmapLeft;
	wxBitmap m_bitmapRight;
	wxBitmap m_bitmapFull;
	wxBitmap m_bitmapThumb;
};

#endif
