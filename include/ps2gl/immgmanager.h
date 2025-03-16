/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_immgmanager_h
#define ps2gl_immgmanager_h

#include "ps2gl/gmanager.h"

/********************************************
 * CImmGeomManager - the immediate renderer
 */

class CImmGeomManager : public CGeomManager {

    CRendererManager RendererManager;

    // a double set of buffers for storing immediate-mode vertices, normals, etc.
    // these don't really need to be dma packets since they will be reffed by
    // the microcode
    CDmaPacket VertexBuf0, NormalBuf0, TexCoordBuf0, ColorBuf0;
    CDmaPacket VertexBuf1, NormalBuf1, TexCoordBuf1, ColorBuf1;
    CDmaPacket *CurVertexBuf, *CurNormalBuf, *CurTexCoordBuf, *CurColorBuf;

    CGeometryBlock Geometry;

    void CommitNewGeom();

public:
    CImmGeomManager(CGLContext& context, int immBufferQwordSize);
    virtual ~CImmGeomManager();

    CRendererManager& GetRendererManager() { return RendererManager; }

    void SwapBuffers();

    // state changes / updates

    void PrimChanged(GLenum primType);
    void SyncRendererContext(GLenum primType);
    void SyncRenderer();
    void SyncGsContext();
    void SyncColorMaterial(bool pvColorsArePresent);

    void DrawingLinearArray();
    void DrawingIndexedArray();

    void SyncArrayType(ArrayType::tArrayType type)
    {
        if (type == ArrayType::kLinear)
            DrawingLinearArray();
        else
            DrawingIndexedArray();
    }

    // for microcode

    // microcode needs to be able to request these for storage when no
    // normal, tex coord or vertex color is supplied for each vertex
    inline CDmaPacket& GetNormalBuf() { return *CurNormalBuf; }
    inline CDmaPacket& GetTexCoordBuf() { return *CurTexCoordBuf; }

    // user state

    void EnableCustom(uint64_t flag) { RendererManager.EnableCustom(flag); }
    void DisableCustom(uint64_t flag) { RendererManager.DisableCustom(flag); }

    // geometry specification

    void BeginGeom(GLenum mode);
    void Vertex(cpu_vec_xyzw newVert);
    void Normal(cpu_vec_xyz normal);
    void TexCoord(float u, float v);
    void Color(cpu_vec_xyzw color);
    void EndGeom();
    void DrawArrays(GLenum mode, int first, int count);
    void DrawIndexedArrays(GLenum primType,
        int numIndices, const unsigned char* indices,
        int numVertices);
    void Flush();
};

#endif // ps2gl_immgmanager_h
