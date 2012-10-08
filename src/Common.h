// (C) John Peterson. License GNU GPL 3.
#pragma once
#include <iostream>
#include <sys/types.h>
#include <wx/wx.h>
using namespace std;
// log
void log(int i, wxString f, ...);
string hexdump(const wxUint8 *buf, int len, int o = 0);
// string
class wxArrayStringEx : public wxArrayString {
public:
	static wxArrayString FromString(wxString str, wxString del);
	static wxString ToString(wxString glue, wxArrayString pieces);
};