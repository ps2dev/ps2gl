/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef displaycontext_h
#define displaycontext_h

#include "ps2gl/glcontext.h"

namespace GS {
class CMemArea;
class CDisplayEnv;
}

class CDisplayContext {
    CGLContext& GLContext;

    // frames, interlacing, etc.
    GS::CMemArea *Frame0Mem, *Frame1Mem;
    GS::CMemArea *CurFrameMem, *LastFrameMem;
    GS::CDisplayEnv* DisplayEnv;

    bool DisplayIsDblBuffered;
    bool DisplayIsInterlaced;

public:
    CDisplayContext(CGLContext& context);
    ~CDisplayContext();

    GS::CDisplayEnv& GetDisplayEnv() { return *DisplayEnv; }
    void SetDisplayBuffers(bool interlaced, GS::CMemArea* frame0Mem, GS::CMemArea* frame1Mem);
    void SwapBuffers();
};

#endif // displaycontext_h
