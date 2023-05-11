/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/math.h"
#include "ps2s/packet.h"

#include "ps2gl/dlgmanager.h"
#include "ps2gl/dlist.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/immgmanager.h"
#include "ps2gl/lighting.h"
#include "ps2gl/material.h"
#include "ps2gl/renderer.h"
#include "ps2gl/texture.h"

using namespace ArrayType;

/********************************************
 * methods
 */

CDListGeomManager::CDListGeomManager(CGLContext& context)
    : CGeomManager(context)
    , CurDList(NULL)
    , RendererMayHaveChanged(false)
{
}

void CDListGeomManager::BeginDListDef()
{
    Prim = GL_INVALID_VALUE;
    Geometry.Reset();

    CDList& dlist = GLContext.GetDListManager().GetOpenDList();
    CurDList      = &dlist;

    LastArrayAccessIsValid = false;
}

void CDListGeomManager::EndDListDef()
{
    Flush();

    CurNormalBuf   = NULL;
    CurVertexBuf   = NULL;
    CurTexCoordBuf = NULL;
    CurColorBuf    = NULL;

    CurDList = NULL;
}

void CDListGeomManager::Flush()
{
    if (Geometry.IsPending()) {
        DrawBlock(Geometry);
        Geometry.Reset();
    }
}

/********************************************
 * glBegin/glEnd
 */

void CDListGeomManager::BeginGeom(GLenum mode)
{
    if (Prim != mode)
        PrimChanged(mode);

    CDList& dlist  = GLContext.GetDListManager().GetOpenDList();
    CurNormalBuf   = &dlist.GetNormalBuf();
    CurVertexBuf   = &dlist.GetVertexBuf();
    CurTexCoordBuf = &dlist.GetTexCoordBuf();
    CurColorBuf    = &dlist.GetColorBuf();

    Geometry.SetPrimType(mode);
    Geometry.SetArrayType(kLinear);
    Geometry.SetNormals(CurNormalBuf->GetNextPtr());
    Geometry.SetVertices(CurVertexBuf->GetNextPtr());
    Geometry.SetTexCoords(CurTexCoordBuf->GetNextPtr());
    Geometry.SetColors(CurColorBuf->GetNextPtr());

    InsideBeginEnd = true;
}

void CDListGeomManager::Vertex(cpu_vec_xyzw newVert)
{
    *CurVertexBuf += newVert;

    Geometry.AddVertices();
}

class CSetNormalCmd : public CDListCmd {
    cpu_vec_xyz Normal;

public:
    CSetNormalCmd(cpu_vec_xyz normal)
        : Normal(normal)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmGeomManager().Normal(Normal);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListGeomManager::Normal(cpu_vec_xyz normal)
{
    if (DoNormalize)
        normal.normalize();

    if (InsideBeginEnd) {
        CurNormal = normal;

        *CurNormalBuf += normal(0);
        *CurNormalBuf += normal(1);
        *CurNormalBuf += normal(2);

        Geometry.AddNormals();
    } else {
        // make sure we don't set this normal before any pending
        // geometry that depends on the current normal (the one before
        // setting this one)
        Flush();
        *CurDList += CSetNormalCmd(normal);
    }
}

class CColorCmd : public CDListCmd {
    cpu_vec_xyzw CurColor;

public:
    CColorCmd(cpu_vec_xyzw color)
        : CurColor(color)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetMaterialManager().Color(CurColor);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListGeomManager::Color(cpu_vec_xyzw color)
{
    if (InsideBeginEnd) {
        *CurColorBuf += color;

        Geometry.AddColors();
    } else {
        CDList& dlist = GLContext.GetDListManager().GetOpenDList();
        dlist += CColorCmd(color);
        GLContext.CurMaterialChanged();
    }
}

class CSetTexCoordCmd : public CDListCmd {
    float U, V;

public:
    CSetTexCoordCmd(float u, float v)
        : U(u)
        , V(v)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmGeomManager().TexCoord(U, V);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListGeomManager::TexCoord(float u, float v)
{
    if (InsideBeginEnd) {
        CurTexCoord[0] = u;
        CurTexCoord[1] = v;

        *CurTexCoordBuf += u;
        *CurTexCoordBuf += v;

        Geometry.AddTexCoords();
    } else {
        Flush(); // see note for Normal
        *CurDList += CSetTexCoordCmd(u, v);
    }
}

void CDListGeomManager::EndGeom()
{
    InsideBeginEnd = false;

    Geometry.SetWordsPerVertex(4);
    Geometry.SetWordsPerNormal(3);
    Geometry.SetWordsPerTexCoord(2);
    Geometry.SetWordsPerColor(4);

    Geometry.SetVerticesAreValid(true);

    // if no normals were added use whatever the current normal is at list execution,
    // if 1 normal was added, set that as the current normal,
    // if more than one normal was added, assume there was one normal per vertex, and
    // set the last to be the current normal
    Geometry.SetNormalsAreValid(false);
    if (Geometry.GetNumNewNormals() > 0) {
        // make last normal current
        *CurDList += CSetNormalCmd(CurNormal);

        if (Geometry.GetNumNewNormals() > 1) {
            mErrorIf(Geometry.GetNumNewVertices() != Geometry.GetNumNewNormals(),
                "Sorry, but in display lists you need to specify either one normal, "
                "or a normal for each vertex given.");
            Geometry.SetNormalsAreValid(true);
        }
    }

    // tex coord data
    Geometry.SetTexCoordsAreValid(false);
    if (Geometry.GetNumNewTexCoords() > 0) {
        // make last tex coord current
        *CurDList += CSetTexCoordCmd(CurTexCoord[0], CurTexCoord[1]);

        mErrorIf(Geometry.GetNumNewVertices() != Geometry.GetNumNewTexCoords(),
            "Sorry, but in display lists you need to specify either one "
            "texture coord for each vertex given, or zero.");
        Geometry.SetTexCoordsAreValid(true);
    }

    // colors
    if (Geometry.GetNumNewColors() > 0) {
        mErrorIf(Geometry.GetNumNewVertices() != Geometry.GetNumNewColors(),
            "Sorry, but in display lists inside glBegin/glEnd you need "
            "to specify either one color for each vertex given, or none.");
        Geometry.SetColorsAreValid(true);
    } else {
        Geometry.SetColorsAreValid(false);
        Geometry.SetColors(NULL);
    }

    CommitNewGeom();
}

/********************************************
 * DrawArrays
 */

void CDListGeomManager::DrawArrays(GLenum mode, int first, int count)
{
    if (Prim != mode)
        PrimChanged(mode);

    Geometry.SetPrimType(mode);
    Geometry.SetArrayType(kLinear);

    Geometry.SetVertices(VertArray->GetVertices());
    Geometry.SetNormals(VertArray->GetNormals());
    Geometry.SetTexCoords(VertArray->GetTexCoords());
    Geometry.SetColors(VertArray->GetColors());

    Geometry.SetVerticesAreValid(VertArray->GetVerticesAreValid());
    Geometry.SetNormalsAreValid(VertArray->GetNormalsAreValid());
    Geometry.SetTexCoordsAreValid(VertArray->GetTexCoordsAreValid());
    Geometry.SetColorsAreValid(VertArray->GetColorsAreValid());

    Geometry.SetWordsPerVertex(VertArray->GetWordsPerVertex());
    Geometry.SetWordsPerNormal(VertArray->GetWordsPerNormal());
    Geometry.SetWordsPerTexCoord(VertArray->GetWordsPerTexCoord());
    Geometry.SetWordsPerColor(VertArray->GetWordsPerColor());

    Geometry.AddVertices(count);
    Geometry.AddNormals(count);
    Geometry.AddTexCoords(count);
    Geometry.AddColors(count);

    Geometry.AdjustNewGeomPtrs(first);

    CommitNewGeom();
}

void CDListGeomManager::DrawIndexedArrays(GLenum primType,
    int numIndices, const unsigned char* indices,
    int numVertices)
{
    if (Prim != primType)
        PrimChanged(primType);

    Geometry.SetPrimType(primType);
    Geometry.SetArrayType(kIndexed);

    Geometry.SetVertices(VertArray->GetVertices());
    Geometry.SetNormals(VertArray->GetNormals());
    Geometry.SetTexCoords(VertArray->GetTexCoords());
    Geometry.SetColors(VertArray->GetColors());

    Geometry.SetVerticesAreValid(VertArray->GetVerticesAreValid());
    Geometry.SetNormalsAreValid(VertArray->GetNormalsAreValid());
    Geometry.SetTexCoordsAreValid(VertArray->GetTexCoordsAreValid());
    Geometry.SetColorsAreValid(VertArray->GetColorsAreValid());

    Geometry.SetWordsPerVertex(VertArray->GetWordsPerVertex());
    Geometry.SetWordsPerNormal(VertArray->GetWordsPerNormal());
    Geometry.SetWordsPerTexCoord(VertArray->GetWordsPerTexCoord());
    Geometry.SetWordsPerColor(VertArray->GetWordsPerColor());

    Geometry.AddVertices(numVertices);
    Geometry.AddNormals(numVertices);
    Geometry.AddTexCoords(numVertices);
    Geometry.AddColors(numVertices);

    Geometry.SetNumIndices(numIndices);
    Geometry.SetIndices(indices);
    Geometry.SetIStripLengths(NULL);

    CommitNewGeom();
}

/********************************************
 * methods / dlist commands that update state
 */

class CUpdateRendererContextCmd : public CDListCmd {
    GLenum PrimType;

public:
    CUpdateRendererContextCmd(GLenum type)
        : PrimType(type)
    {
    }
    CDListCmd* Play()
    {
        CImmGeomManager& gmanager = pGLContext->GetImmGeomManager();
        gmanager.SyncRendererContext(PrimType);
        return CDListCmd::GetNextCmd(this);
    }
};

class CUpdatePrimCmd : public CDListCmd {
    unsigned char Prim;

public:
    CUpdatePrimCmd(unsigned char prim)
        : Prim(prim)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmGeomManager().GetRendererManager().PrimChanged(Prim);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListGeomManager::PrimChanged(unsigned char prim)
{
    GLContext.PrimChanged();
    GLContext.GetDListManager().GetOpenDList() += CUpdatePrimCmd(prim);
}

class CUpdateRendererCmd : public CDListCmd {
    bool PerVtxColors;
    tArrayType ArrayType;

public:
    CUpdateRendererCmd(bool pvColors, tArrayType type)
        : PerVtxColors(pvColors)
        , ArrayType(type)
    {
    }
    CDListCmd* Play()
    {
        CImmGeomManager& gmanager = pGLContext->GetImmGeomManager();
        gmanager.SyncColorMaterial(PerVtxColors);
        gmanager.SyncArrayType(ArrayType);
        gmanager.SyncRenderer();
        return CDListCmd::GetNextCmd(this);
    }
};

class CUpdateGsContextCmd : public CDListCmd {
public:
    CDListCmd* Play()
    {
        pGLContext->GetImmGeomManager().SyncGsContext();
        return CDListCmd::GetNextCmd(this);
    }
};

/********************************************
 * render-related methods/commands
 */

void CDListGeomManager::DrawingLinearArray()
{
    if (!LastArrayAccessIsValid || LastArrayAccessWasIndexed) {
        GLContext.ArrayAccessChanged();
        LastArrayAccessIsValid = true;
    }
    LastArrayAccessWasIndexed = false;
}

void CDListGeomManager::DrawingIndexedArray()
{
    if (!LastArrayAccessIsValid || !LastArrayAccessWasIndexed) {
        GLContext.ArrayAccessChanged();
        LastArrayAccessIsValid = true;
    }
    LastArrayAccessWasIndexed = true;
}

void CDListGeomManager::CommitNewGeom()
{
    // do this before updating the renderer
    if (Geometry.GetNewArrayType() == kLinear)
        DrawingLinearArray();
    else
        DrawingIndexedArray();

    bool doReset = true;

    if (Geometry.IsPending()) {
        // if the context hasn't changed, try to merge the new geometry
        // into the current block
        if (GLContext.GetRendererContextChanged() == 0
            && GLContext.GetGsContextChanged() == 0
            && !UserRenderContextChanged
            && GLContext.GetRendererPropsChanged() == 0
            && Geometry.MergeNew())
            doReset = false;
        // couldn't merge; draw the old geometry so we can reset and start a new block
        else {
            DrawBlock(Geometry);
        }
    }

    if (doReset) {
        Geometry.MakeNewValuesCurrent();
        Geometry.ResetNew(); // if we don't do this counts will keep accumulating

        if (GLContext.GetRendererPropsChanged()) {
            *CurDList += CUpdateRendererCmd(Geometry.GetColorsAreValid(), Geometry.GetArrayType());
            GLContext.SetRendererPropsChanged(false);
            GLContext.SetRendererContextChanged(true);
        }
        if (GLContext.GetRendererContextChanged()) {
            *CurDList += CUpdateRendererContextCmd(Geometry.GetPrimType());
            GLContext.SetRendererContextChanged(false);
            Prim = Geometry.GetPrimType();
        }
        if (GLContext.GetGsContextChanged()) {
            *CurDList += CUpdateGsContextCmd();
            GLContext.SetGsContextChanged(false);
        }
        if (UserRenderContextChanged) {
            // not much point in adding this into the dl since there's no way now of
            // adding the user context change into the dl..
            // might be useful to break things up, though..
            UserRenderContextChanged = false;
        }
    }
}

class CDrawArraysCmd : public CDListCmd {
    CGeometryBlock Geometry;
    bool IsCached;
    CDList& DList;
    CVifSCDmaPacket* RenderPacket;
    tU64 RenderContextDependencies, RenderContextDepMask;

public:
    CDrawArraysCmd(CGeometryBlock& block, CDList& dlist)
        : Geometry(block)
        , IsCached(false)
        , DList(dlist)
        , RenderPacket(NULL)
        , RenderContextDependencies(0)
        , RenderContextDepMask(0)
    {
    }

    CDListCmd* Play()
    {

        // ** do not ** update the renderer context here; it will have
        // been taken care of by other dlist commands

        // first, check to see if the packet needs to be rebuilt.  This decision
        // is based on what the renderer tells us that it cares about.  The
        // default renderers, for example, will generate packets that embed
        // whether texture mapping is enabled and so will need to be rebuilt if
        // the packet is called in a context different from that in which it was
        // first created.

        tU64 curRenderContext     = (tU64)pGLContext->GetImmGeomManager().GetRendererManager().GetRendererReqs();
        tU64 curRenderContextDeps = curRenderContext & RenderContextDepMask;
        bool rebuildPacket        = (curRenderContextDeps != RenderContextDependencies);

        // [re]build cached dma packet

        if (!IsCached || rebuildPacket) {

            // we need to build the packet to render this array

            // first, can it be built once and reused?
            CImmGeomManager& gmanager = pGLContext->GetImmGeomManager();
            CRenderer& renderer       = gmanager.GetRendererManager().GetCurRenderer();
            bool cachePacket          = renderer.GetCachePackets(Geometry);

            // next, under what conditions does the packet we're about to build remain valid?
            RenderContextDepMask      = renderer.GetRenderContextDeps();
            RenderContextDependencies = RenderContextDepMask & curRenderContext;

            if (cachePacket) {
                // allocate memory for the new packet

                // ask the renderer how much memory to allocate for the packet
                int qwords = renderer.GetPacketQwordSize(Geometry);

                if (!RenderPacket) {
                    RenderPacket = new CVifSCDmaPacket(qwords, DMAC::Channels::vif1,
                        Packet::kXferTags,
                        Core::MemMappings::UncachedAccl);
                    DList.RegisterNewPacket(RenderPacket);
                }

                // if this is not the first time creating the packet, we need to
                // delete the old packet, but not immediately
                // because it may take up to two frames to be dma'ed
                if (IsCached) {
                    void* newBuf = CDmaPacket::AllocBuffer(qwords,
                        Core::MemMappings::UncachedAccl);
                    void* oldBuf = RenderPacket->SwapOutBuffer(newBuf);
                    pGLContext->AddBufferToBeFreed(Core::MakePtrNormal(oldBuf));
                }

                RenderPacket->Reset();

                IsCached = true;
            } else {
                // don't cache the packet..  use the main dma packet
                RenderPacket = &pGLContext->GetVif1Packet();
            }

            pGLContext->PushVif1Packet();
            {
                pGLContext->SetVif1Packet(*RenderPacket);
                renderer.DrawLinearArrays(Geometry);
            }
            pGLContext->PopVif1Packet();

            // terminate cached packets with a Ret() so they can be Call()ed
            if (cachePacket) {
                RenderPacket->Ret();
                RenderPacket->Pad128();
                RenderPacket->CloseTag();
            }
        }

        // if the packet was cached, call() it from the vif1 dma chain
        if (IsCached) {
            pGLContext->GetVif1Packet().Call(*RenderPacket);
            pGLContext->GetVif1Packet().Pad128();
            pGLContext->GetVif1Packet().CloseTag();
        } else {
            // not really necessary
            RenderPacket = NULL;
        }

        return CDListCmd::GetNextCmd(this);
    }
};

class CDrawIndexedArraysCmd : public CDListCmd {
    CGeometryBlock Geometry;
    bool IsCached;
    CDList& DList;
    CVifSCDmaPacket* RenderPacket;
    bool IsTexEnabled, IsLightingEnabled;

public:
    CDrawIndexedArraysCmd(CGeometryBlock& block, CDList& dlist)
        : Geometry(block)
        , IsCached(false)
        , DList(dlist)
        , RenderPacket(NULL)
        , IsTexEnabled(false)
        , IsLightingEnabled(false)
    {
    }

    CDListCmd* Play()
    {

        // ** do not ** update the renderer context here; it will have
        // been taken care by other dlist commands

        bool texEnabled = pGLContext->GetTexManager().GetTexEnabled();
        bool lEnabled   = pGLContext->GetImmLighting().GetLightingEnabled();

        // don't cache the packet if it depends on the current normal
        bool dontCache = (lEnabled && !Geometry.GetNormalsAreValid());

        // [re]build cached dma packet
        if (!IsCached
            || texEnabled != IsTexEnabled
            || lEnabled != IsLightingEnabled) {

            if (!dontCache) {

                // allocate packet
                if (!RenderPacket) {
                    // FIXME:  this is a pitiful hack for allocating enough memory
                    // change below, too
                    int qwords   = Math::Max(Geometry.GetNumArrays(), 1) * 100;
                    RenderPacket = new CVifSCDmaPacket(qwords, DMAC::Channels::vif1,
                        Packet::kXferTags,
                        Core::MemMappings::UncachedAccl);
                    DList.RegisterNewPacket(RenderPacket);
                }

                // if this is not the first time creating the packet, we need to
                // delete the old packet, but not immediately
                // because it may take up to two frames to be dma'ed
                if (IsCached) {
                    int qwords   = Math::Max(Geometry.GetNumArrays(), 1) * 100;
                    void* newBuf = CDmaPacket::AllocBuffer(qwords,
                        Core::MemMappings::UncachedAccl);
                    void* oldBuf = RenderPacket->SwapOutBuffer(newBuf);
                    pGLContext->AddBufferToBeFreed(oldBuf);
                }

                RenderPacket->Reset();

                IsCached = true;
            } else {
                // don't cache the packet..  use the main dma packet
                RenderPacket = &pGLContext->GetVif1Packet();
            }

            IsTexEnabled      = texEnabled;
            IsLightingEnabled = lEnabled;

            pGLContext->PushVif1Packet();
            {
                pGLContext->SetVif1Packet(*RenderPacket);
                CImmGeomManager& gmanager = pGLContext->GetImmGeomManager();
                CRenderer& renderer       = gmanager.GetRendererManager().GetCurRenderer();
                renderer.DrawIndexedArrays(Geometry);
            }
            pGLContext->PopVif1Packet();

            if (!dontCache) {
                RenderPacket->Ret();
                RenderPacket->Pad128();
                RenderPacket->CloseTag();
            }
        }

        if (!dontCache) {
            pGLContext->GetVif1Packet().Call(*RenderPacket);
            pGLContext->GetVif1Packet().Pad128();
            pGLContext->GetVif1Packet().CloseTag();
        } else {
            RenderPacket = NULL;
        }

        return CDListCmd::GetNextCmd(this);
    }
};

void CDListGeomManager::DrawBlock(CGeometryBlock& block)
{
    if (block.GetArrayType() == kLinear)
        *CurDList += CDrawArraysCmd(block, *CurDList);
    else
        *CurDList += CDrawIndexedArraysCmd(block, *CurDList);
}

class CEnableCustomCmd : public CDListCmd {
    tU64 Flag;

public:
    CEnableCustomCmd(tU64 flag)
        : Flag(flag)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmGeomManager().GetRendererManager().EnableCustom(Flag);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListGeomManager::EnableCustom(tU64 flag)
{
    *CurDList += CEnableCustomCmd(flag);
}

class CDisableCustomCmd : public CDListCmd {
    tU64 Flag;

public:
    CDisableCustomCmd(tU64 flag)
        : Flag(flag)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmGeomManager().GetRendererManager().DisableCustom(Flag);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListGeomManager::DisableCustom(tU64 flag)
{
    *CurDList += CDisableCustomCmd(flag);
}
