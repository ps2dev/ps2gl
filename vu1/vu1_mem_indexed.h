/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

// Memory looks like this:
//
//   <context information>
//     <output buffer>	    18/26
//   <first input buffer>   3/26
//   <first color buffer>   1/26
//  <second input buffer>   3/26
//  <second color buffer>   1/26
//
// the output buffer needs to be about 6 times the size of the
// input buffers for triangles.  The "color buffers" are for
// accumulating the lit color of each vertex, so they need to
// be at most 1/3 the size of the input buffers (which have 3 or 4
// quads per vertex).  That means that if the free space is S and
// the size of each input buffer is x:  6x + 1x + 1/3x + 1x + 1/3x = S.
// x works out to be 3/26, which gives the numbers above
//
// for output = 3 * input:
//
// 3x + 1x + 1/3x + 1x + 1/3x = S
// x = 3/17S
//

// **** absolute offsets ****

#define kContextStart 0

#include "vu1_context.h"

#define kOutputBufStart (kContextStart + kContextLength)
#define kOutputStart kOutputBufStart // temp
#define kOutputBufSize ((1024 - kOutputBufStart) * 9 / 17)

#define kOutputGeomStart (kOutputBufStart + 1) // 1 quad for giftag

#define kDoubleBufBase (kOutputBufStart + kOutputBufSize)
#define kDoubleBufOffset ((1024 - kDoubleBufBase) / 2)
#define kDoubleBufSize kDoubleBufOffset

#define kInputBufSize ((1024 - kOutputBufStart) * 3 / 17)

// **** buffer-relative offsets ****

// input buffer

#define kNumVertices 0  // x field
#define kNumIndicesD2 0 // y field
#define kNumIndices 0   // z field
#define kStripADCs (kNumVertices + 1)
#define kInputGeomStart (kStripADCs + 4)
#define kInputStart kInputGeomStart // temp

#define kTempAreaStart (kInputGeomStart + kInputBufSize)
