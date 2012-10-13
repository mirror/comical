/*
 * ComicBookZIP.h
 * Copyright (c) 2003-2011 James Athey. 2012, John Peterson.
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

#ifndef _ComicBookZIP_h_
#define _ComicBookZIP_h_

#include "ComicalFrame.h"
#include "ComicBook.h"

class ComicBookZIPOpen;

class ComicBookZIP : public ComicBook {
	friend class ComicBookZIPOpen;
public:
	ComicBookZIP(wxString _filename, wxUint32 _cacheLen, COMICAL_ZOOM _zoom, long _zoomLevel, bool _fitOnlyOversize, COMICAL_MODE _mode, FREE_IMAGE_FILTER _filter, COMICAL_DIRECTION _direction);
	~ComicBookZIP();
	wxString ArchiveError(int Error);

protected:
	ComicBookZIPOpen *Open;
	virtual wxInputStream * ExtractStream(wxUint32 pageindex);
	virtual wxInputStream * ExtractStream(wxString path);
	virtual bool TestPassword();
	virtual void ResumeOpen();
};

class ComicBookZIPOpen : public ComicBookOpen {
	friend class ComicBookZIP;
	ComicBookZIP *p;

public:
	ComicBookZIPOpen(ComicBookZIP *parent) : p(parent) {}
	virtual ExitCode Entry();
};

#endif
