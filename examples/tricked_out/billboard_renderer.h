/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef bb_renderer_h
#define bb_renderer_h

//#include "ps2s/matrix.h"
#include "ps2s/packet.h"

#include "GL/gl.h"
#include "ps2gl/immgmanager.h"
#include "ps2gl/linear_renderer.h"
#include "ps2gl/renderer.h"

#define kBillboardPrimType (((tU32)1 << 31) | 1)
#define kBillboardPrimTypeFlag ((tU64)1 << 32)

class CBillboardRenderer : public CLinearRenderer {
    static void* Microcode;

public:
    CBillboardRenderer();

    static CBillboardRenderer* Register();

    virtual void InitContext(GLenum primType, tU32 rcChanges, bool userRcChanged);
    virtual void DrawLinearArrays(CGeometryBlock& block);
};

#endif // bb_renderer_h
