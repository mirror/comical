/***************************************************************************
 *   Copyright (C) 2005 by James Leighton Athey                            *
 *   jathey@comcast.net                                                    *
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
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef __RARSTREAM_H__
#define __RARSTREAM_H__

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "rarstrm.h"
#endif

#include "wx/defs.h"

#if wxUSE_STREAMS

#include "wx/stream.h"
#include "wx/log.h"
#include "wx/intl.h"
#include "wx/stream.h"
#include "wx/wfstream.h"
#include "wx/utils.h"
#include <string>
#include <iostream>

#include "dll.hpp"
#include "Exceptions.h"

//--------------------------------------------------------------------------------
// wxRarInputStream
//                  This class is input stream from RAR archive. The archive
//                  must be local file (accessible via FILE*)
//--------------------------------------------------------------------------------

class wxRarInputStream : public wxInputStream
{
public:
    wxRarInputStream(const wxString& archive, const wxString& file);
    ~wxRarInputStream();

    virtual size_t GetSize() const { return m_Size; }
    virtual wxFileOffset GetLength() const { return m_Size; }
    virtual bool IsSeekable() const { return TRUE; }

    virtual bool Eof() const;
    virtual char Peek();
    virtual wxInputStream& Read(void *buffer, size_t size);
    virtual wxFileOffset SeekI(wxFileOffset pos, wxSeekMode mode);
    virtual wxFileOffset TellI() const { return m_Pos; }
    
protected:
    virtual size_t OnSysRead(void *buffer, size_t bufsize);
    virtual off_t OnSysSeek(off_t seek, wxSeekMode mode);
    virtual wxFileOffset OnSysTell() const { return m_Pos; }

private:
    wxString ProcessFileError(int Error, wxString compressedFile);
    wxString OpenArchiveError(int Error);
    
    size_t m_Size;
    off_t m_Pos;
    char *m_Buffer; // contains the stream data
    wxString m_ArcName;
};

int CALLBACK CallbackProc(wxUint32 msg, long UserData, long P1, long P2);

#endif // wxUSE_STREAMS
#endif // __RARSTREAM_H__
