/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <stdio.h>
#include <ps2gl/lighting.h>

#include "ps2s/cpu_matrix.h"
#include "ps2s/displayenv.h"
#include "ps2s/math.h"
#include "ps2s/packet.h"

#include "GL/ps2gl.h"
#include "ps2gl/clear.h"
#include "ps2gl/dlist.h"
#include "ps2gl/drawcontext.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/gmanager.h"
#include "ps2gl/material.h"
#include "ps2gl/matrix.h"
#include "ps2gl/renderer.h"
#include "ps2gl/texture.h"

using namespace ArrayType;

/********************************************
 * CImmGeomManager
 */

CImmGeomManager::CImmGeomManager(CGLContext& context, int immBufferQwordSize)
    : CGeomManager(context)
    , RendererManager(context)
    , VertexBuf0(immBufferQwordSize + immBufferQwordSize % 4,
          DMAC::Channels::vif1, Core::MemMappings::UncachedAccl)
    , NormalBuf0(immBufferQwordSize * 4 / 3 + 1 + (immBufferQwordSize * 4 / 3 + 1) % 4,
          DMAC::Channels::vif1, Core::MemMappings::UncachedAccl)
    , TexCoordBuf0(immBufferQwordSize / 2 + (immBufferQwordSize / 2) % 4,
          DMAC::Channels::vif1, Core::MemMappings::UncachedAccl)
    , ColorBuf0(immBufferQwordSize + immBufferQwordSize % 4,
          DMAC::Channels::vif1, Core::MemMappings::UncachedAccl)
    , VertexBuf1(immBufferQwordSize + immBufferQwordSize % 4,
          DMAC::Channels::vif1, Core::MemMappings::UncachedAccl)
    , NormalBuf1(immBufferQwordSize * 4 / 3 + 1 + (immBufferQwordSize * 4 / 3 + 1) % 4,
          DMAC::Channels::vif1, Core::MemMappings::UncachedAccl)
    , TexCoordBuf1(immBufferQwordSize / 2 + (immBufferQwordSize / 2) % 4,
          DMAC::Channels::vif1, Core::MemMappings::UncachedAccl)
    , ColorBuf1(immBufferQwordSize + immBufferQwordSize % 4,
          DMAC::Channels::vif1, Core::MemMappings::UncachedAccl)
{
    CurVertexBuf   = &VertexBuf0;
    CurNormalBuf   = &NormalBuf0;
    CurTexCoordBuf = &TexCoordBuf0;
    CurColorBuf    = &ColorBuf0;

    VertArray = new CVertArray;

    RendererManager.ArrayAccessChanged(RendererProps::kLinear);
}

CImmGeomManager::~CImmGeomManager()
{
    delete VertArray;
}

void CImmGeomManager::SwapBuffers()
{
    // flip the geometry buffers
    if (CurVertexBuf == &VertexBuf0)
        CurVertexBuf = &VertexBuf1;
    else
        CurVertexBuf = &VertexBuf0;
    CurVertexBuf->Reset();
    if (CurNormalBuf == &NormalBuf0)
        CurNormalBuf = &NormalBuf1;
    else
        CurNormalBuf = &NormalBuf0;
    CurNormalBuf->Reset();
    if (CurTexCoordBuf == &TexCoordBuf0)
        CurTexCoordBuf = &TexCoordBuf1;
    else
        CurTexCoordBuf = &TexCoordBuf0;
    CurTexCoordBuf->Reset();
    if (CurColorBuf == &ColorBuf0)
        CurColorBuf = &ColorBuf1;
    else
        CurColorBuf = &ColorBuf0;
    CurColorBuf->Reset();
}

/********************************************
 * glBegin/glEnd and related
 */

void CImmGeomManager::BeginGeom(GLenum mode)
{
    if (Prim != mode)
        PrimChanged(mode);

    Geometry.SetPrimType(mode);
    Geometry.SetArrayType(kLinear);
    ColorVariesInPrim = false;

    Geometry.SetNormals(CurNormalBuf->GetNextPtr());
    Geometry.SetVertices(CurVertexBuf->GetNextPtr());
    Geometry.SetTexCoords(CurTexCoordBuf->GetNextPtr());
    Geometry.SetColors(CurColorBuf->GetNextPtr());

    InsideBeginEnd = true;
}

void CImmGeomManager::Vertex(cpu_vec_xyzw newVert)
{
    cpu_vec_xyz normal = GetCurNormal();
    *CurNormalBuf += normal;

    const float* texCoord = GetCurTexCoord();
    *CurTexCoordBuf += texCoord[0];
    *CurTexCoordBuf += texCoord[1];

    if (ColorVariesInPrim) {
        const cpu_vec_xyzw color = GetCurGeomColor();
        *CurColorBuf += color;
        Geometry.AddColors();
    }
    *CurVertexBuf += newVert;

    Geometry.AddVertices();
    Geometry.AddNormals();
    Geometry.AddTexCoords();
}

void CImmGeomManager::Normal(cpu_vec_xyz normal)
{
    if (DoNormalize)
        normal.normalize();
    CurNormal = normal;
}

void CImmGeomManager::Color(cpu_vec_xyzw color)
{
    if (InsideBeginEnd) {
        if (!ColorVariesInPrim) {
            int backFillVertexCount = Geometry.GetNumNewVertices() - Geometry.GetNumNewColors();
            const cpu_vec_xyzw currentColor = GetCurGeomColor();
            for (int i = 0; i < backFillVertexCount; ++i) {
                *CurColorBuf += currentColor;
                Geometry.AddColors();
            }
            ColorVariesInPrim = true;
        }
        SetCurGeomColor(color);
    } else {
        SetCurGeomColor(color);
        GLContext.GetMaterialManager().Color(color);
    }
}

void CImmGeomManager::TexCoord(float u, float v)
{
    CurTexCoord[0] = u;
    CurTexCoord[1] = v;
}

void CImmGeomManager::EndGeom()
{
    InsideBeginEnd = false;

    Geometry.SetVerticesAreValid(true);
    Geometry.SetNormalsAreValid(true);
    Geometry.SetTexCoordsAreValid(true);

    const bool useColorLane =
        (!GLContext.GetImmLighting().GetLightingEnabled() && ColorVariesInPrim) ||
        ( GLContext.GetImmLighting().GetLightingEnabled() &&
          GLContext.GetMaterialManager().GetColorMaterialEnabled() &&
          ColorVariesInPrim);

    LaneConfig lanes;
    lanes.vertices  = QW_XYZW;
    lanes.normals   = GLContext.GetImmLighting().GetLightingEnabled() ? QW_XYZ : QW_NONE;
    lanes.texcoords = GLContext.GetTexManager().GetTexEnabled() ? QW_XY : QW_NONE;
    lanes.colors    = useColorLane ? QW_XYZW : QW_NONE;

    ValidateLaneConfig(&lanes, "EndGeom");

    Geometry.SetColorsAreValid(LanePresent(lanes.colors));
    SyncColorMaterial(LanePresent(lanes.colors));
    RendererManager.PerVtxMaterialChanged(useColorLane ? RendererProps::kDiffuse : RendererProps::kNoMaterial);

    Geometry.SetWordsPerVertex(QWToWords(lanes.vertices));
    Geometry.SetWordsPerNormal(QWToWords(lanes.normals));
    Geometry.SetWordsPerTexCoord(QWToWords(lanes.texcoords));
    Geometry.SetWordsPerColor(QWToWords(lanes.colors));

    CommitNewGeom();
}

/********************************************
 * LinearArraysGeomStage
 */

void CImmGeomManager::LinearArraysGeomStage(GLenum mode, int first, int count)
{
    if (Prim != mode)
        PrimChanged(mode);

    Geometry.SetPrimType(mode);
    Geometry.SetArrayType(kLinear);

    Geometry.SetVertices(VertArray->GetVertices());
    Geometry.SetNormals(VertArray->GetNormals());
    Geometry.SetTexCoords(VertArray->GetTexCoords());

    void* colorsPtr = VertArray->GetColors();
    const bool colorArrayEnabled = VertArray->GetColorsAreValid() && VertArray->GetWordsPerColor() == 4;

    if (colorArrayEnabled && VertArray->GetColorSrcType() == kColor_UByte) {
        float* bufStart = (float*)CurColorBuf->GetNextPtr();
        const unsigned char* srcColorBuf_U8 = (const unsigned char*)colorsPtr;
        const int totalCount = first + count;
        for (int i = 0; i < totalCount; ++i) {
            const unsigned char* colorChannels = srcColorBuf_U8 + 4*i;
            *CurColorBuf += colorChannels[0] / 255.0f;
            *CurColorBuf += colorChannels[1] / 255.0f;
            *CurColorBuf += colorChannels[2] / 255.0f;
            *CurColorBuf += colorChannels[3] / 255.0f;
        }
        colorsPtr = bufStart;
    }

    Geometry.SetColors(colorsPtr);

    Geometry.SetVerticesAreValid(VertArray->GetVerticesAreValid());
    Geometry.SetNormalsAreValid(VertArray->GetNormalsAreValid());
    Geometry.SetTexCoordsAreValid(VertArray->GetTexCoordsAreValid());

    const bool arrayHasColors = VertArray->GetColorsAreValid() && (VertArray->GetWordsPerColor() > 0);

    LaneConfig lanes;
    lanes.vertices  = (VertArray->GetWordsPerVertex()   == 4) ? QW_XYZW :
                      (VertArray->GetWordsPerVertex()   == 3) ? QW_XYZ  : QW_NONE;
    lanes.normals   = (VertArray->GetNormalsAreValid()   &&
                       VertArray->GetWordsPerNormal()    == 3 &&
                       GLContext.GetImmLighting().GetLightingEnabled()) ? QW_XYZ : QW_NONE;
    lanes.texcoords = (VertArray->GetTexCoordsAreValid() &&
                       VertArray->GetWordsPerTexCoord()  == 2) ? QW_XY : QW_NONE;
    lanes.colors    =
        ((!GLContext.GetImmLighting().GetLightingEnabled() && arrayHasColors) ||
         ( GLContext.GetImmLighting().GetLightingEnabled() &&
           GLContext.GetMaterialManager().GetColorMaterialEnabled() &&
           arrayHasColors)) ? QW_XYZW : QW_NONE;

    ValidateLaneConfig(&lanes, "LinearArraysGeomStage");

    Geometry.SetWordsPerVertex(QWToWords(lanes.vertices));
    Geometry.SetWordsPerNormal(QWToWords(lanes.normals));
    Geometry.SetWordsPerTexCoord(QWToWords(lanes.texcoords));
    Geometry.SetWordsPerColor(QWToWords(lanes.colors));

    Geometry.SetColorsAreValid(LanePresent(lanes.colors));
    SyncColorMaterial(LanePresent(lanes.colors));
    RendererManager.PerVtxMaterialChanged(LanePresent(lanes.colors) ? RendererProps::kDiffuse : RendererProps::kNoMaterial);

    Geometry.AddVertices(count);
    Geometry.AddNormals(count);
    Geometry.AddTexCoords(count);
    if (LanePresent(lanes.colors)) Geometry.AddColors(count);
    Geometry.AdjustNewGeomPtrs(first);

    CommitNewGeom();
}

void CImmGeomManager::IndexedArraysGeomStage(GLenum primType,
    int numIndices, const unsigned char* indices,
    int numVertices)
{
    if (Prim != primType) PrimChanged(primType);

    Geometry.SetPrimType(primType);
    Geometry.SetArrayType(kIndexed);

    Geometry.SetVertices(VertArray->GetVertices());
    Geometry.SetNormals(VertArray->GetNormals());
    Geometry.SetTexCoords(VertArray->GetTexCoords());

    void* colorsPtr = VertArray->GetColors();
    const bool colorArrayEnabled = VertArray->GetColorsAreValid() && VertArray->GetWordsPerColor() == 4;
    if (colorArrayEnabled && VertArray->GetColorSrcType() == kColor_UByte) {
        float* bufStart = (float*)CurColorBuf->GetNextPtr();
        const unsigned char* srcColorBuf_U8 = (const unsigned char*)colorsPtr;
        for (int i = 0; i < numVertices; ++i) {
            const unsigned char* colorChannels = srcColorBuf_U8 + 4*i;
            *CurColorBuf += (float)colorChannels[0] / 255.0f;
            *CurColorBuf += (float)colorChannels[1] / 255.0f;
            *CurColorBuf += (float)colorChannels[2] / 255.0f;
            *CurColorBuf += (float)colorChannels[3] / 255.0f;
        }
        colorsPtr = bufStart;
    }
    Geometry.SetColors(colorsPtr);

    Geometry.SetVerticesAreValid(VertArray->GetVerticesAreValid());
    Geometry.SetNormalsAreValid(VertArray->GetNormalsAreValid());
    Geometry.SetTexCoordsAreValid(VertArray->GetTexCoordsAreValid());

    const bool arrayHasColors = VertArray->GetColorsAreValid() && (VertArray->GetWordsPerColor() > 0);

    LaneConfig lanes;
    lanes.vertices  = (VertArray->GetWordsPerVertex()   == 4) ? QW_XYZW :
                      (VertArray->GetWordsPerVertex()   == 3) ? QW_XYZ  : QW_NONE;
    lanes.normals   = (VertArray->GetNormalsAreValid()   &&
                       VertArray->GetWordsPerNormal()    == 3 &&
                       GLContext.GetImmLighting().GetLightingEnabled()) ? QW_XYZ : QW_NONE;
    lanes.texcoords = (VertArray->GetTexCoordsAreValid() &&
                       VertArray->GetWordsPerTexCoord()  == 2) ? QW_XY : QW_NONE;
    lanes.colors    =
        ((!GLContext.GetImmLighting().GetLightingEnabled() && arrayHasColors) ||
         ( GLContext.GetImmLighting().GetLightingEnabled() &&
           GLContext.GetMaterialManager().GetColorMaterialEnabled() &&
           arrayHasColors)) ? QW_XYZW : QW_NONE;

    ValidateLaneConfig(&lanes, "IndexedArraysGeomStage");

    Geometry.SetWordsPerVertex(QWToWords(lanes.vertices));
    Geometry.SetWordsPerNormal(QWToWords(lanes.normals));
    Geometry.SetWordsPerTexCoord(QWToWords(lanes.texcoords));
    Geometry.SetWordsPerColor(QWToWords(lanes.colors));

    Geometry.SetColorsAreValid(LanePresent(lanes.colors));
    SyncColorMaterial(LanePresent(lanes.colors));
    RendererManager.PerVtxMaterialChanged(LanePresent(lanes.colors) ? RendererProps::kDiffuse
                                                                    : RendererProps::kNoMaterial);

    Geometry.AddVertices(numVertices);
    Geometry.AddNormals(numVertices);
    Geometry.AddTexCoords(numVertices);
    if (LanePresent(lanes.colors)) Geometry.AddColors(numVertices);

    Geometry.SetNumIndices(numIndices);
    Geometry.SetIndices(indices);
    Geometry.SetIStripLengths(NULL);

    CommitNewGeom();
}

/********************************************
 * common and synchronization code
 */

void CImmGeomManager::DrawingIndexedArray()
{
    if (!LastArrayAccessIsValid || !LastArrayAccessWasIndexed) {
        GLContext.ArrayAccessChanged();
        RendererManager.ArrayAccessChanged(RendererProps::kIndexed);
        LastArrayAccessIsValid = true;
    }
    LastArrayAccessWasIndexed = true;
}

void CImmGeomManager::DrawingLinearArray()
{
    if (!LastArrayAccessIsValid || LastArrayAccessWasIndexed) {
        GLContext.ArrayAccessChanged();
        RendererManager.ArrayAccessChanged(RendererProps::kLinear);
        LastArrayAccessIsValid = true;
    }
    LastArrayAccessWasIndexed = false;
}

void CImmGeomManager::CommitNewGeom()
{
    // do this before updating the renderer
    if (Geometry.GetNewArrayType() == kLinear)
        DrawingLinearArray();
    else
        DrawingIndexedArray();

    bool doReset         = true;
    bool rendererChanged = RendererManager.UpdateNewRenderer();

    if (Geometry.IsPending()) {

        // FIXME: need to ask the renderer what context changes it cares about/updates

        // if the context hasn't changed, try to merge the new geometry
        // into the current block
        if (GLContext.GetRendererContextChanged() == 0
            && GLContext.GetGsContextChanged() == 0
            && !UserRenderContextChanged
            && !rendererChanged
            && Geometry.MergeNew()) {
            doReset = false;
        } else {
            // couldn't merge; draw the old geometry so we can reset and start a new block
            if (Geometry.GetArrayType() == kLinear)
                RendererManager.GetCurRenderer().DrawLinearArrays(Geometry);
            else
                RendererManager.GetCurRenderer().DrawIndexedArrays(Geometry);
        }
    }

    if (doReset) {
        Geometry.MakeNewValuesCurrent();
        Geometry.ResetNew();

        if (rendererChanged) {
            RendererManager.MakeNewRendererCurrent();
            RendererManager.LoadRenderer(GLContext.GetVif1Packet());
        }
        SyncRendererContext(Geometry.GetPrimType());
        SyncGsContext();
    }
}

void CImmGeomManager::PrimChanged(GLenum primType)
{
    GLContext.PrimChanged();
    RendererManager.PrimChanged(primType);
}

void CImmGeomManager::SyncRenderer()
{
    if (RendererManager.UpdateNewRenderer()) {
        RendererManager.MakeNewRendererCurrent();
        RendererManager.LoadRenderer(GLContext.GetVif1Packet());
    }
}

void CImmGeomManager::SyncRendererContext(GLenum primType)
{
    // resend the rendering context if necessary
    if (GLContext.GetRendererContextChanged() || (RendererManager.IsCurRendererCustom() && UserRenderContextChanged)) {
        RendererManager.GetCurRenderer().InitContext(primType, GLContext.GetRendererContextChanged(), UserRenderContextChanged);
        GLContext.SetRendererContextChanged(false);
        UserRenderContextChanged = false;
        Prim                     = primType;
    }
}

void CImmGeomManager::SyncGsContext()
{
    if (uint32_t changed = GLContext.GetGsContextChanged()) {
        // has the texture changed?
        bool texEnabled         = GLContext.GetTexManager().GetTexEnabled();
        CVifSCDmaPacket& packet = GLContext.GetVif1Packet();
        if (texEnabled
            && changed & GsCtxtFlags::Texture) {
            // we have to wait for all previous buffers to finish, or
            // the new texture settings might beat the geometry to the gs..
            packet.Cnt();
            packet.Flush().Nop();
            packet.CloseTag();
            GLContext.GetTexManager().UseCurTexture(GLContext.GetVif1Packet());
        }

        // has the draw environment changed?
        if (changed & GsCtxtFlags::DrawEnv) {
            // as with textures..
            packet.Cnt();
            packet.Flush().Nop();
            packet.CloseTag();
            // FIXME
            GLContext.AddingDrawEnvToPacket((uint128_t*)GLContext.GetVif1Packet().GetNextPtr() + 1);
            GLContext.GetImmDrawContext().GetDrawEnv().SendSettings(GLContext.GetVif1Packet());
        }

        GLContext.SetGsContextChanged(false);
    }
}

void CImmGeomManager::SyncColorMaterial(bool pvColorsArePresent)
{
    CMaterialManager& mm = GLContext.GetMaterialManager();
    //if (pvColorsArePresent && mm.GetColorMaterialEnabled()) {
    if (GLContext.GetImmLighting().GetLightingEnabled() && pvColorsArePresent && mm.GetColorMaterialEnabled()) {
        switch (mm.GetColorMaterialMode()) {
        case GL_EMISSION:
            mNotImplemented("Only GL_DIFFUSE can change per-vertex");
            break;
        case GL_AMBIENT:
            mNotImplemented("Only GL_DIFFUSE can change per-vertex");
            break;
        case GL_DIFFUSE:
            // fix later..
            GLContext.PerVtxMaterialChanged();
            RendererManager.PerVtxMaterialChanged(RendererProps::kDiffuse);
            break;
        case GL_AMBIENT_AND_DIFFUSE:
            mNotImplemented("Only GL_DIFFUSE can change per-vertex");
            break;
        case GL_SPECULAR:
            mNotImplemented("Only GL_DIFFUSE can change per-vertex");
            // PerVtxMaterialChanged( PerVtxMaterial::kSpecular );
            break;
        }
    } else {
        RendererManager.PerVtxMaterialChanged(RendererProps::kNoMaterial);
    }
}

void CImmGeomManager::Flush()
{
    if (Geometry.IsPending()) {
        if (Geometry.GetArrayType() == kLinear)
            RendererManager.GetCurRenderer().DrawLinearArrays(Geometry);
        else
            RendererManager.GetCurRenderer().DrawIndexedArrays(Geometry);
        Geometry.Reset();
    }
}
