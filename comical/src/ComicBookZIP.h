/***************************************************************************
                          ComicBookZIP.h  -  description
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
 ***************************************************************************/

#ifndef _ComicBookZIP_h_
#define _ComicBookZIP_h_

#include "ComicBook.h"

class ComicBookZIP : public ComicBook {

public:
  ComicBookZIP(wxString file);
  ~ComicBookZIP();
  virtual bool Extract(unsigned int pageindex, char *data_ptr);

};

#endif
