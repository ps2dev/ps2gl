/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

// **** absolute offsets ****

#define kContextStart	0

// we'll use the default rendering context, as our new bb renderer
// will be basically the same as a default renderer, with a little
// bit of extra setup.
#include "vu1_context.h"

// this is the end of the rendering context, where we will put
// double-buffered (quad-buffered) data
#define kDoubleBufBase	(kContextStart + kContextLength)

// the offset into the double-buffered area (starting at
// kDoubleBufBase) that points to the second buffer
#define kDoubleBufOffset	((1024 - kDoubleBufBase)/2)

// the size of each buffer
#define kDoubleBufSize		kDoubleBufOffset

// **** buffer-relative offsets ****

#define kNumVertices	0
#define kNumBillboards	kNumVertices
// we don't use the strip adcs, but linear_renderer always xfers them
#define kStripADCs	(kNumVertices + 1)
#define kInputStart	(kStripADCs + 4)


// we will be outputting about 8 times as much data as we input
// (one center position -> 4 vertices, 4 tex coords)

#define kInputBufSize	(kDoubleBufSize/9)
#define kOutputStart	(0 + kInputBufSize)
#define kOutputBufSize	(kDoubleBufSize - kOutputStart)
