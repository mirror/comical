/***************************************************************************
                               ComicBookBZ2.cpp
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

#include "ComicBookBZ2.h"

ComicBookBZ2::ComicBookBZ2(wxString file) {
  unsigned int i;
  unsigned long size;
  char line[256];
  wxString command;
  vector<wxString> entries;
  filename = file;
  current = -1;
  command = "tar -tjvf \"" + filename + "\"";
  redi::ipstream list(command.c_str());
/*
  if (list.exited())
  	cerr << "status = " << list.status() << ", error = " << list.error() << endl;
*/
  for (i = 0; list.getline(line, 256) && line[0] != '\0'; i++) {
    wxString wline = line;
    wxStringTokenizer tokens(wline);
    tokens.GetNextToken(); // attr field
    tokens.GetNextToken(); // owner field
    wxString size_str = tokens.GetNextToken();
    size_str.ToULong(&size, 10);
    tokens.GetNextToken(); // date field
    tokens.GetNextToken(); // time field

    wxString page = line + tokens.GetPosition();

    /* Now some contributors are lazy and don't order their pages inside the
       archive. By putting the filename and the size into the same entry, we
       can use the STL's sort() algorithm and split them up later. */
    if (page.Right(5).Upper() == ".JPEG" || page.Right(4).Upper() == ".JPG" ||
	page.Right(5).Upper() == ".TIFF" || page.Right(4).Upper() == ".TIF" ||
	page.Right(4).Upper() == ".GIF" ||
	page.Right(4).Upper() == ".PNG" ) {
      if (size > 0) {
        entries.push_back(page + "\t" + size_str);
      }
    }
  }

  list.close();

  vector<wxString>::iterator begin = entries.begin();
  vector<wxString>::iterator end = entries.end();
  sort(begin, end);  // I love the STL!

  for (i = 0; i < entries.size(); i++) {
    cerr << "smushed entry " << i << ":" << entries[i] << endl;
    wxStringTokenizer tokens(entries[i], "\t");
    pages.push_back(tokens.GetNextToken());
    tokens.GetNextToken().ToULong(&size, 10);
    sizes.push_back(size); // get the length
  }

}

ComicBookBZ2::~ComicBookBZ2() {

}

bool ComicBookBZ2::Extract(unsigned int pageindex, char *data) {

  wxString command;

  if (pageindex >= pages.size()) return false;
  command = "tar -jOx \"" + pages[pageindex] + "\" -f \"" + filename + "\"";
  redi::ipstream pipe(command.c_str());
  pipe.read(data, sizes[pageindex]);
  pipe.close();
  return true;

}
