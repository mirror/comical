/***************************************************************************
                          ComicBookRAR.h  -  description
                             -------------------
    begin                : Wed Oct 29 2003
    copyright            : (C) 2003 by James Athey
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

#ifndef _ComicBookRAR_h_
#define _ComicBookRAR_h_

#include "ComicBook.h"
#include "rarstrm.h"
#include "dll.hpp"
 
class ComicBookRAR : public ComicBook {

public:
	ComicBookRAR(wxString file, wxUint32 cachelen);
	virtual ~ComicBookRAR() {};

protected:
	wxInputStream * ExtractStream(wxUint32 pageindex);

private:
	void OpenArchiveError(int Error, wxString ArcName);
	void ProcessFileError(int Error);
};

#endif

