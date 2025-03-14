/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_indexed_renderer_h
#define ps2gl_indexed_renderer_h

#include "ps2gl/base_renderer.h"

class CIndexedRenderer : public CBaseRenderer {
protected:
    cpu_vec_4 ConstantVertColor;

public:
    CIndexedRenderer(void* packet, int packetSize, CRendererProps caps, CRendererProps reqs,
        int inQuadsPerVert, int outQuadsPerVert,
        const char* name);

    virtual void InitContext(GLenum primType, uint32_t rcChanges, bool userRcChanged);
    virtual void DrawIndexedArrays(CGeometryBlock& block);
    virtual int GetPacketQwordSize(const CGeometryBlock& geometry);
    virtual CRendererProps GetRenderContextDeps();
    virtual bool GetCachePackets(const CGeometryBlock& geometry);
};

#endif // ps2gl_indexed_renderer_h
