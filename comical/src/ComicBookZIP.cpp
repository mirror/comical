/***************************************************************************
                          ComicBookZIP.cpp  -  description
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

#include "ComicBookZIP.h"

ComicBookZIP::ComicBookZIP(wxString file) {

  unsigned int i;
  unsigned long size;
  char line[256];
  wxString command;
  vector<wxString> entries;
  filename = file;
  current = -1;
  command = "unzip -l -qq \"" + filename + "\"";
  redi::ipstream list(command.c_str());
  for (i = 0; list.getline(line, 256) && line[0] != '\0'; i++) {
    wxString wline = line;
    wxStringTokenizer tokens(wline);
    wxString size_str = tokens.GetNextToken();
    size_str.ToULong(&size, 10);
    tokens.GetNextToken(); // date field
    tokens.GetNextToken(); // time field
    wxString page = line + tokens.GetPosition() + 2;

    /* Now some CBR and CBZ contributors are lazy and don't order their pages
       inside the archive.  By putting the filename and the size into the same
       entry, we can use the STL's sort() algorithm and split them up later. */
    if (	page.Right(5).Upper() == ".JPEG" || page.Right(4).Upper() == ".JPG" ||
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

ComicBookZIP::~ComicBookZIP() {


}

bool ComicBookZIP::Extract(unsigned int pageindex, char *data) {

  wxString command;

  if (pageindex >= pages.size()) return false;
  command = "unzip -p \"" + filename + "\" \"" + pages[pageindex] + "\"";
  redi::ipstream pipe(command.c_str());
  pipe.read(data, sizes[pageindex]);
  pipe.close();
  return true;

}
