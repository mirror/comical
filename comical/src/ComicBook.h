/***************************************************************************
                ComicBook.h - ComicBook class and its children
                             -------------------
    begin                : Mon Sep 29 2003
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

#ifndef _ComicBook_h_
#define _ComicBook_h_

//#include "ComicalApp.h"
// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/string.h>
#include <wx/tokenzr.h>
#endif

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include "pstream.h"

using namespace std;

class ComicBook {

public:

  virtual ~ComicBook() = 0;
  virtual bool Extract(unsigned int pageindex, char *data) = 0;
  
  wxString filename;
  vector<wxString> pages;
  vector<unsigned long> sizes;
  int current;  // -1 means no pages open
};

extern ComicBook *theBook;

#endif
