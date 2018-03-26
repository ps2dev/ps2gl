/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

// **** absolute offsets ****

#define kContextStart 0

#include "vu1_context.h"

#define kDoubleBufBase (kContextStart + kContextLength)
#define kDoubleBufOffset ((1024 - kDoubleBufBase) / 2)
#define kDoubleBufSize kDoubleBufOffset

// **** buffer-relative offsets ****

#define kNumVertices 0

//#define kInputStrides	(kNumVertices + 1)
//#define kQuadADCs	(kInputStrides + 1)
//// these are temporary till I think of something better..
//#define kStripADCs	(kQuadADCs + 1)

// these are temporary till I think of something better..
#define kStripADCs (kNumVertices + 1)

#define kInputStart (kStripADCs + 4)

#define kInputBufSize (kDoubleBufSize / 2)
#define kOutputStart (0 + kInputBufSize)
#define kOutputBufSize (kDoubleBufSize - kOutputStart)
