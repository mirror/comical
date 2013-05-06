/*
 * ComicBookDir.h
 * Copyright (c) 2005-2011, James Athey
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

#ifndef _ComicBookDir_h_
#define _ComicBookDir_h_

#include "ComicBook.h"
#include "Exceptions.h"
 
#include <wx/wfstream.h>
#include <wx/dir.h>

class ComicBookDir : public ComicBook {

public:
	ComicBookDir(ComicalFrame *_parent, wxString _filename, wxUint32 _cacheLen, COMICAL_ZOOM _zoom, long _zoomLevel, bool _fitOnlyOversize, COMICAL_MODE _mode, FREE_IMAGE_FILTER _filter, COMICAL_DIRECTION _direction);
	~ComicBookDir() {};

protected:
	wxInputStream * ExtractStream(wxUint32 pageindex);
	wxInputStream * ExtractStream(wxString path);
};

#endif

