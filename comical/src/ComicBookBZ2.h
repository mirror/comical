/***************************************************************************
                               ComicBookBZ2.h
                             -------------------
    begin                : Tue Dec 30 2003
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
 ***************************************************************************/

#ifndef _ComicBookBZ2_h_
#define _ComicBookBZ2_h_

#include "ComicBook.h"

class ComicBookBZ2 : public ComicBook {

public:
  ComicBookBZ2(wxString file);
  ~ComicBookBZ2();
  virtual bool Extract(unsigned int pageindex, char *data_ptr);

};

#endif

