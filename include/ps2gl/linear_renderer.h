/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef linear_renderer_h
#define linear_renderer_h

#include "ps2s/packet.h"

#include "GL/gl.h"
#include "ps2gl/base_renderer.h"
#include "ps2gl/immgmanager.h"
#include "ps2gl/renderer.h"

class CLinearRenderer : public CBaseRenderer {
protected:
    int InputGeomBufSize;

    // called by DrawArrays
    void DrawBlock(CVifSCDmaPacket& packet, CGeometryBlock& block, int maxVertsPerBuffer);

    // used by DrawBlock
    void FindNumBuffers(int numToAdd, int numVertsToRestart,
        int numVertsAlreadyInFirstBuffer, int maxVertsPerBuffer,
        int& numVertsFirstBuffer, int& numVertsLastBuffer,
        int& numBuffers);

    // used by DrawBlock
    void FinishBuffer(CVifSCDmaPacket& packet, int numVertsToBreakStrip,
        int numVertsInBuffer, int vu1QuadsPerVert,
        int numStripsInBuffer, unsigned short* stripOffsets);

    // used by FinishBuffer
    void XferBufferHeader(CVifSCDmaPacket& packet, int numVertsToBreakStrip,
        int numVerts,
        int numStripsInBuffer, unsigned short* stripOffsets);

public:
    CLinearRenderer(void* packet, int packetSize, CRendererProps caps, CRendererProps reqs,
        int inQuadsPerVert, int outQuadsPerVert,
        int inGeomOffset, int inGeomBufSize,
        const char* name)
        : CBaseRenderer(packet, packetSize, caps, reqs,
              inQuadsPerVert, outQuadsPerVert, inGeomOffset, name)
        , InputGeomBufSize(inGeomBufSize)
    {
    }

    CLinearRenderer(void* packet, int packetSize,
        int inQuadsPerVert, int outQuadsPerVert,
        int inGeomOffset, int inGeomBufSize,
        const char* name)
        : CBaseRenderer(packet, packetSize, inQuadsPerVert, outQuadsPerVert, inGeomOffset, name)
        , InputGeomBufSize(inGeomBufSize)
    {
    }

    virtual void InitContext(GLenum primType, uint32_t rcChanges, bool userRcChanged);
    virtual void DrawLinearArrays(CGeometryBlock& block);
    virtual int GetPacketQwordSize(const CGeometryBlock& geometry);
    virtual CRendererProps GetRenderContextDeps();
    virtual bool GetCachePackets(const CGeometryBlock& geometry);
};

#endif // linear_renderer_h
