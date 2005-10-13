// This file has been modified to work with 24-bit wxImages instead of
// 32-bit FreeImage objects.  Modifications (c) 2003 James Athey

// ==========================================================
// Upsampling / downsampling classes
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

#ifndef _RESIZE_H_
#define _RESIZE_H_

#include <math.h> // Has to come before "Filters.h"
#include "Filters.h"
#include <wx/image.h>

/// Max function
template <class T> T MAX(T a, T b) {
	return (a > b) ? a: b;
}

/// Min function
template <class T> T MIN(T a, T b) {
	return (a < b) ? a: b;
}

/** Upsampling / downsampling filters.
Constants used in FreeImage_Rescale.
*/
enum FREE_IMAGE_FILTER {
	FILTER_BOX	  = 0,	// Box, pulse, Fourier window, 1st order (constant) b-spline
	FILTER_BICUBIC	  = 1,	// Mitchell & Netravali's two-param cubic filter
	FILTER_BILINEAR   = 2,	// Bilinear filter
	FILTER_BSPLINE	  = 3,	// 4th order (cubic) b-spline
	FILTER_CATMULLROM = 4,	// Catmull-Rom spline, Overhauser spline
	FILTER_LANCZOS3	  = 5	// Lanczos3 filter
};

wxImage FreeImage_Rescale(wxImage dib, wxInt32 dst_width, wxInt32 dst_height, FREE_IMAGE_FILTER filter);

/**
  Filter weights table.
  This class stores contribution information for an entire line (row or column).
*/
class CWeightsTable
{
/**
  Sampled filter weight table.
  Contribution information for a single pixel
*/
typedef struct {
	/// Normalized weights of neighboring pixels
	double *Weights;
	/// Bounds of source pixels window
	wxInt32 Left, Right;
} Contribution;

public:
	/// Row (or column) of contribution weights
	Contribution *m_WeightTable;
	/// Filter window size (of affecting source pixels)
	unsigned long m_WindowSize;
	/// Length of line (no. or rows / cols)
	unsigned long m_LineLength;

public:
	/**
	Constructor
	Allocate and compute the weights table
	@param pFilter Filter used for upsampling or downsampling
	@param uLineSize Length (in pixels) of the destination line buffer
	@param uSrcSize Length (in pixels) of the source line buffer
	@param dScale Scale factor (should be equal to uDstSize / uSrcSize)
	*/
	CWeightsTable(CGenericFilter *pFilter, unsigned long uDstSize, unsigned long uSrcSize, double dScale) {
		unsigned long u;
		double dWidth;
		double dFScale = 1.0;
		double dFilterWidth = pFilter->GetWidth();

		if(dScale < 1.0) {
			// Minification
			dWidth = dFilterWidth / dScale;
			dFScale = dScale;
		} else {
			// Magnification
			dWidth= dFilterWidth;
		}

		// Allocate a new line contributions structure
		//
		// Window size is the number of sampled pixels
		m_WindowSize = 2 * (wxInt32)ceil(dWidth) + 1;
		m_LineLength = uDstSize;
		 // Allocate list of contributions
		m_WeightTable = new Contribution[m_LineLength];
		for(u = 0 ; u < m_LineLength ; u++) {
			// Allocate contributions for every pixel
			m_WeightTable[u].Weights = new double[m_WindowSize];
		}


		for(u = 0; u < m_LineLength; u++) {
			// Scan through line of contributions
			double dCenter = (double)u / dScale;   // Reverse mapping
			// Find the significant edge points that affect the pixel
			wxInt32 iLeft = MAX (0, (wxInt32)floor (dCenter - dWidth));
			wxInt32 iRight = MIN ((wxInt32)ceil (dCenter + dWidth), wxInt32(uSrcSize) - 1);

			// Cut edge points to fit in filter window in case of spill-off
			if((iRight - iLeft + 1) > wxInt32(m_WindowSize)) {
				if(iLeft < (wxInt32(uSrcSize) - 1 / 2)) {
					iLeft++;
				} else {
					iRight--;
				}
			}

			m_WeightTable[u].Left = iLeft;
			m_WeightTable[u].Right = iRight;

			wxInt32 iSrc = 0;
			double dTotalWeight = 0.0;  // Zero sum of weights
			for(iSrc = iLeft; iSrc <= iRight; iSrc++) {
				// Calculate weights
				double weight = dFScale * pFilter->Filter(dFScale * (dCenter - (double)iSrc));
				m_WeightTable[u].Weights[iSrc-iLeft] = weight;
				dTotalWeight += weight;
			}
			if((dTotalWeight > 0) && (dTotalWeight != 1)) {
				// Normalize weight of neighbouring points
				for(iSrc = iLeft; iSrc <= iRight; iSrc++) {
					// Normalize point
					m_WeightTable[u].Weights[iSrc-iLeft] /= dTotalWeight;
				}
			}
	   }
	}

	/**
	Destructor
	Destroy the weights table
	*/
	~CWeightsTable() {
		for(unsigned long u = 0; u < m_LineLength; u++) {
			// Free contributions for every pixel
			delete [] m_WeightTable[u].Weights;
		}
		// Free list of pixels contributions
		delete [] m_WeightTable;
	}

	/** Retrieve a filter weight, given source and destination positions
	@param dst_pos Pixel position in destination line buffer
	@param src_pos Pixel position in source line buffer
	@return Returns the filter weight
	*/
	double GetWeight(wxInt32 dst_pos, wxInt32 src_pos) {
		return m_WeightTable[dst_pos].Weights[src_pos];
	}

	/** Retrieve left boundary of source line buffer
	@param dst_pos Pixel position in destination line buffer
	@return Returns the left boundary of source line buffer
	*/
	wxInt32 GetLeftBoundary(wxInt32 dst_pos) {
		return m_WeightTable[dst_pos].Left;
	}

	/** Retrieve right boundary of source line buffer
	@param dst_pos Pixel position in destination line buffer
	@return Returns the right boundary of source line buffer
	*/
	wxInt32 GetRightBoundary(wxInt32 dst_pos) {
		return m_WeightTable[dst_pos].Right;
	}
};

// ---------------------------------------------

/**
 CResizeEngine
 This class performs filtered zoom. It scales an image to the desired dimensions with
 any of the CGenericFilter derived filter class.
 It only works with 32-bit buffers of RGBQUAD type (i.e. color images whose color model follows
 the ARGB convention).

 <b>References</b> :
 [1] Paul Heckbert, C code to zoom raster images up or down, with nice filtering.
 UC Berkeley, August 1989. [online] http://www-2.cs.cmu.edu/afs/cs.cmu.edu/Web/People/ph/heckbert.html
 [2] Eran Yariv, Two Pass Scaling using Filters. The Code Project, December 1999.
 [online] http://www.codeproject.com/bitmap/2_pass_scaling.asp

*/
class CResizeEngine
{
private:
	/// Pointer to the FIR / IIR filter
	CGenericFilter* m_pFilter;

public:

    /// Constructor
	CResizeEngine(CGenericFilter* filter):m_pFilter(filter) {}

    /// Destructor
	virtual ~CResizeEngine() {}

    /** Scale an image to the desired dimensions
	@param pOrigImage Pointer to the 32-bit source image pixels
	@param uOrigWidth Source image width
	@param uOrigHeight Source image height
	@param pDstImage Pointer to the 32-bit destination image pixels
	@param uNewWidth Destination image width
	@param uNewHeight Destination image height
	*/
	void Scale(unsigned char *pOrigImage, unsigned long uOrigWidth, unsigned long uOrigHeight, unsigned char *pDstImage, unsigned long uNewWidth, unsigned long uNewHeight) {
		// decide which filtering order (xy or yx) is faster for this mapping by
		// counting convolution multiplies

		if(uNewWidth*uOrigHeight <= uNewHeight*uOrigWidth) {
			// xy filtering
			//
			// scale source image horizontally into temporary image
			unsigned char *pTemp = new unsigned char[uNewWidth * uOrigHeight * 3];
			HorizontalFilter(pOrigImage, uOrigWidth, uOrigHeight, pTemp, uNewWidth, uOrigHeight);
			// scale temporary image vertically into result image
			VerticalFilter(pTemp, uNewWidth, uOrigHeight, pDstImage, uNewWidth, uNewHeight);
			delete [] pTemp;
		} else {
			// yx filtering
			//
			// scale source image vertically into temporary image
			unsigned char *pTemp = new unsigned char[uOrigWidth * uNewHeight * 3];
			VerticalFilter(pOrigImage, uOrigWidth, uOrigHeight, pTemp, uOrigWidth, uNewHeight);

			// scale temporary image horizontally into result image
			HorizontalFilter(pTemp, uOrigWidth, uNewHeight, pDstImage, uNewWidth, uNewHeight);
			delete [] pTemp;
		}
	}


private:

    /// Performs horizontal image filtering
	void HorizontalFilter(unsigned char *pSrc, unsigned long uSrcWidth, unsigned long uSrcHeight, unsigned char *pDst, unsigned long uDstWidth, unsigned long uDstHeight) {
		if(uDstWidth == uSrcWidth) {
			// no scaling required, just copy
			memcpy (pDst, pSrc, sizeof (unsigned char) * 3 * uSrcHeight * uSrcWidth);
		}
		// allocate and calculate the contributions
		CWeightsTable weightsTable(m_pFilter, uDstWidth, uSrcWidth, double(uDstWidth) / double(uSrcWidth));

		// step through rows
		for(unsigned long uRow = 0; uRow < uDstHeight; uRow++) {
			// scale each row
			unsigned char *pSrcRow = &(pSrc[uRow * uSrcWidth * 3]);
			unsigned char *pDstRow = &(pDst[uRow * uDstWidth * 3]);
			for(unsigned long x = 0; x < uDstWidth; x++) {
				// loop through row
				double r = 0;
				double g = 0;
				double b = 0;
				//double a = 0;
				wxInt32 iLeft = weightsTable.GetLeftBoundary(x);    // retrieve left boundary
				wxInt32 iRight = weightsTable.GetRightBoundary(x);  // retrieve right boundary
				for(wxInt32 i = iLeft; i <= iRight; i++) {
					// scan between boundaries
					// accumulate weighted effect of each neighboring pixel
					double weight = weightsTable.GetWeight(x, i-iLeft);
					unsigned long iReal = i * 3;
					r += (weight * (double)(pSrcRow[iReal]));
					g += (weight * (double)(pSrcRow[iReal + 1]));
					b += (weight * (double)(pSrcRow[iReal + 2]));
					//a += (weight * (double)(pSrcRow[i].rgbReserved));
				}
				// place result in destination pixel
				unsigned long xReal = x * 3;
				pDstRow[xReal]     = MIN(MAX((wxInt32)0, (wxInt32)(r + 0.5)), (wxInt32)255); // red
				pDstRow[xReal + 1] = MIN(MAX((wxInt32)0, (wxInt32)(g + 0.5)), (wxInt32)255); // green
				pDstRow[xReal + 2] = MIN(MAX((wxInt32)0, (wxInt32)(b + 0.5)), (wxInt32)255); // blue
				//pDstRow[x].rgbReserved = (BYTE)MIN(MAX((wxInt32)0, (wxInt32)(a + 0.5)), (wxInt32)255);
			}
		}
	}

    /// Performs vertical image filtering
    void VerticalFilter(unsigned char *pSrc, unsigned long uSrcWidth, unsigned long uSrcHeight, unsigned char *pDst, unsigned long uDstWidth, unsigned long uDstHeight) {
		if(uSrcHeight == uDstHeight) {
			// no scaling required, just copy
			memcpy (pDst, pSrc, sizeof (unsigned char) * 3 * uSrcHeight * uSrcWidth);
		}
		// allocate and calculate the contributions
		CWeightsTable weightsTable(m_pFilter, uDstHeight, uSrcHeight, double(uDstHeight) / double(uSrcHeight));

		// step through columns
		for(unsigned long uCol = 0; uCol < uDstWidth; uCol++) {
			// scale each column
			for(unsigned long y = 0; y < uDstHeight; y++) {
				// loop through column
				double r = 0;
				double g = 0;
				double b = 0;
				//double a = 0;
				wxInt32 iLeft = weightsTable.GetLeftBoundary(y);    // retrieve left boundary
				wxInt32 iRight = weightsTable.GetRightBoundary(y);  // retrieve right boundary

				for(wxInt32 i = iLeft; i <= iRight; i++) {
					// scan between boundaries
					// accumulate weighted effect of each neighboring pixel
					//unsigned char pCurSrc = pSrc[i * uSrcWidth * 3 + uCol];
					unsigned long index = i * uSrcWidth * 3 + uCol * 3;
					double weight = weightsTable.GetWeight(y, i-iLeft);
					r += (weight * (double)(pSrc[index]));
					g += (weight * (double)(pSrc[index + 1]));
					b += (weight * (double)(pSrc[index + 2]));
					//a += (weight * (double)(pCurSrc.rgbReserved));
				}
				// clamp and place result in destination pixel
				unsigned long index = y * uDstWidth * 3 + uCol * 3;
				pDst[index]	= MIN(MAX((wxInt32)0, (wxInt32)(r + 0.5)), (wxInt32)255);
				pDst[index + 1]	= MIN(MAX((wxInt32)0, (wxInt32)(g + 0.5)), (wxInt32)255);
				pDst[index + 2]	= MIN(MAX((wxInt32)0, (wxInt32)(b + 0.5)), (wxInt32)255);
				//pDst[index].rgbReserved = (BYTE)MIN(MAX((wxInt32)0, (wxInt32)(a + 0.5)), (wxInt32)255);
			}
		}
	}
};

#endif //   _RESIZE_H_
