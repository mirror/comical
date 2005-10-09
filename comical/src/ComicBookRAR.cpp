/***************************************************************************
                              ComicBookRAR.cpp
                             -------------------
    begin                : Wed Oct 29 2003
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

#include "ComicBookRAR.h"

ComicBookRAR::ComicBookRAR(wxString file, uint cachelen) : ComicBook()
{
	HANDLE RarFile;
	int RHCode,PFCode;
	char CmtBuf[16384];
	struct RARHeaderDataEx HeaderData;
	struct RAROpenArchiveDataEx OpenArchiveData;
	wxString page;
	
	filename = file;
	cacheLen = cachelen;
	Current = 0;
	
	memset(&OpenArchiveData,0,sizeof(OpenArchiveData));
	OpenArchiveData.ArcName = (char *) filename.c_str();
	OpenArchiveData.CmtBuf=CmtBuf;
	OpenArchiveData.CmtBufSize=sizeof(CmtBuf);
	OpenArchiveData.OpenMode=RAR_OM_LIST;
	RarFile=RAROpenArchiveEx(&OpenArchiveData);

	if (OpenArchiveData.OpenResult!=0) {
		OpenArchiveError(OpenArchiveData.OpenResult, filename);
		return;  // Time to throw an exception?
	}

	HeaderData.CmtBuf=CmtBuf;
	HeaderData.CmtBufSize=sizeof(CmtBuf);
	
	while ((RHCode=RARReadHeaderEx(RarFile,&HeaderData))==0) {
		page = _T(HeaderData.FileName);
		if(page.Right(5).Upper() == ".JPEG" || page.Right(4).Upper() == ".JPG" ||
		page.Right(5).Upper() == ".TIFF" || page.Right(4).Upper() == ".TIF" ||
		page.Right(4).Upper() == ".GIF" ||
		page.Right(4).Upper() == ".PNG" )
			Filenames.push_back(page);
		
		if ((PFCode=RARProcessFile(RarFile,RAR_SKIP,NULL,NULL))!=0) {
			ProcessFileError(PFCode);
			break;
		}
	}

	if (RHCode==ERAR_BAD_DATA)
		wxLogError(wxT("libunrar: \nFile header broken"));

	vector<wxString>::iterator begin = Filenames.begin();
	vector<wxString>::iterator end = Filenames.end();
	sort(begin, end);  // I love the STL!

	pageCount = Filenames.size();
	
	originals = new wxImage[pageCount];
	resamples = new wxImage[pageCount];
	Orientations = new COMICAL_ROTATE[pageCount]; // NORTH == 0
	for (uint i = 0; i < pageCount; i++)
		Orientations[i] = NORTH;
	
	RARCloseArchive(RarFile);
	
	Create(); // create the wxThread
}

wxInputStream * ComicBookRAR::ExtractStream(unsigned int pageindex)
{
	return new wxRarInputStream(filename, Filenames[pageindex]);
}

void ComicBookRAR::OpenArchiveError(int Error, wxString ArcName)
{
	switch(Error) {
		case ERAR_NO_MEMORY:
			wxLogError(wxT("libunrar: Not enough memory"));
			break;
		case ERAR_EOPEN:
			wxLogError(wxT("libunrar: Cannot open " + ArcName));
			break;
		case ERAR_BAD_ARCHIVE:
			wxLogError(wxT("libunrar: " + ArcName + " is not RAR archive"));
			break;
		case ERAR_BAD_DATA:
			wxLogError(wxT("libunrar: " + ArcName + " archive header broken"));
			break;
		case ERAR_UNKNOWN:
			wxLogError(wxT("libunrar: Unknown error"));
			break;
	}
}

void ComicBookRAR::ProcessFileError(int Error)
{
	switch(Error) {
		case ERAR_UNKNOWN_FORMAT:
			wxLogError(wxT("libunrar: Unknown archive format"));
			break;
		case ERAR_BAD_ARCHIVE:
			wxLogError(wxT("libunrar: Bad volume"));
			break;
		case ERAR_ECREATE:
			wxLogError(wxT("libunrar: File create error"));
			break;
		case ERAR_EOPEN:
			wxLogError(wxT("libunrar: Volume open error"));
			break;
		case ERAR_ECLOSE:
			wxLogError(wxT("libunrar: File close error"));
			break;
		case ERAR_EREAD:
			wxLogError(wxT("libunrar: Read error"));
			break;
		case ERAR_EWRITE:
			wxLogError(wxT("libunrar: Write error"));
			break;
		case ERAR_BAD_DATA:
			wxLogError(wxT("libunrar: CRC error"));
			break;
		case ERAR_UNKNOWN:
			wxLogError(wxT("libunrar: Unknown error"));
			break;
	}
}
