// This file has been modified to work with 24-bit wxImages instead of
// 32-bit FreeImage objects.  Modifications (c) 2003 James Athey

// ==========================================================
// Upsampling / downsampling routine
//
// Design and implementation by
// - Hervé Drolon (drolon@infonie.fr)
//
// This file is part of FreeImage 3
//
// COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
// THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
// OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
// CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
// THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
// SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
// PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
// THIS DISCLAIMER.
//
// Use at your own risk!
// ==========================================================

#include "Resize.h"

wxImage
FreeImage_Rescale(wxImage src, wxInt32 dst_width, wxInt32 dst_height, FREE_IMAGE_FILTER filter) {

	wxImage dst;

	if ((src.Ok()) && (dst_width > 0) && (dst_height > 0)) {

		dst.Create(dst_width, dst_height);

		// select the filter
		CGenericFilter *pFilter = NULL;
		switch(filter) {
			case FILTER_BOX:
				pFilter = new CBoxFilter();
				break;
			case FILTER_BICUBIC:
				pFilter = new CBicubicFilter();
				break;
			case FILTER_BILINEAR:
				pFilter = new CBilinearFilter();
				break;
			case FILTER_BSPLINE:
				pFilter = new CBSplineFilter();
				break;
			case FILTER_CATMULLROM:
				pFilter = new CCatmullRomFilter();
				break;
			case FILTER_LANCZOS3:
				pFilter = new CLanczos3Filter();
				break;
		}
		CResizeEngine Engine(pFilter);

		// perform upsampling or downsampling
		unsigned char *pSrc = src.GetData();
		unsigned char *pDst = dst.GetData();
		wxInt32 src_width  = src.GetWidth();
		wxInt32 src_height = src.GetHeight();

		Engine.Scale(pSrc, src_width, src_height, pDst, dst_width, dst_height);

		delete pFilter;

	}

	return dst;

}
