
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

    virtual size_t GetSize() const {return m_Size;}
    virtual bool Eof() const;

protected:
    virtual size_t OnSysRead(void *buffer, size_t bufsize);
    virtual off_t OnSysSeek(off_t seek, wxSeekMode mode);
    virtual off_t OnSysTell() const {return m_Pos;}

private:
    void ProcessFileError(int Error);
    void OpenArchiveError(int Error);
    
    size_t m_Size;
    off_t m_Pos;
    char *m_Buffer;
    wxString m_ArcName;
};

int CallbackProc(uint msg, long UserData, long P1, long P2);

#endif 
   // wxUSE_STREAMS

#endif 
   // __RARSTREAM_H__
