// (C) John Peterson. License GNU GPL 3.
#include "Common.h"
// log
void log(int i, wxString f, ...) {
	int c = 37, a = 0;
	switch (i) {
	case 1: c = 30; a = 1; break;
	case 2: c = 36; a = 0; break;
	case 3: c = 34; a = 1; break;
	case 4: c = 35; a = 1; break;
	case 5: c = 32; a = 0; break;
	case 6: c = 33; a = 0; break;
	case 7: c = 31; a = 1; break;
	}
	const int len = 0xfff;
	wchar_t buf[len];
	va_list l;
	va_start(l, f.wc_str());
	#ifdef __MINGW32__
	_vsnwprintf(buf, len, f.wc_str(), l);
	#else
	vswprintf(buf, len, f.wc_str(), l);
	#endif
	va_end(l);
	#ifdef __MINGW32__
	OutputDebugString(buf);
	#endif
	fwprintf(stderr, L"\033[%d;%dm%s\033[0m\n", a, c, buf);
	fflush(stderr);
}

string hexdump(const wxUint8 *buf, int len, int o) {	
	string tmp;
	char tmp_c[16];
	for (int i = 0; i < len; i++) {
		sprintf(tmp_c, "%02x%s", buf[i+o], i>1 && (i+1)%16 == 0 ? "": " ");
		tmp.append(tmp_c);
		if(i>1 && (i+1)%8 == 0) tmp.append(" ");
		if(i>1 && (i+1)%16 == 0) tmp.append("\n");
	}
	if (len%16 != 0) tmp.append("\n");
	cerr << tmp;
	return tmp;
}

wxArrayString wxArrayStringEx::FromString(wxString str, wxString delimiter) {
    wxArrayString arr;

	int strleng = str.length();
	int delleng = delimiter.length();
	if (!delleng)
		return arr;

	int i = 0;
	int k = 0;
	while (i < strleng) {
		int j = 0;
		while (i+j < strleng && j < delleng && str.GetChar(i+j) == delimiter.GetChar(j))
			j++;
		if (j == delleng) {
			arr.Add(str.substr(k, i-k));
			i += delleng;
			k = i;
		} else {
			i++;
		}
	}
	arr.Add(str.substr(k, i-k));
	return arr;
}

wxString wxArrayStringEx::ToString(wxString glue, wxArrayString pieces) {
	wxString a;
	int leng = pieces.GetCount();
	for (int i = 0; i < leng; i++) {
		a.Append(pieces.Item(i));
		if (i < leng-1)
			a += glue;
	}
	return a;
}