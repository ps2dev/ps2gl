/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_context_h
#define ps2gl_context_h

/********************************************
 * includes
 */

#include "ps2s/gsmem.h"
#include "ps2s/packet.h"

#include "GL/gl.h"

/********************************************
 * state change flags
 */

namespace RendererCtxtFlags {
static const int NumLights        = 1 << 0;
static const int LightPropChanged = 1 << 1;
static const int GlobalAmb        = 1 << 9;
static const int CurMaterial      = 1 << 10;
static const int Xform            = 1 << 11;
static const int Prim             = 1 << 12;
static const int Shading          = 1 << 13;
static const int TexEnabled       = 1 << 14;
static const int LightingEnabled  = 1 << 15;
static const int AlphaBlending    = 1 << 16;
static const int CullFaceDir      = 1 << 17;
static const int ClippingEnabled  = 1 << 18;
}

namespace GsCtxtFlags {
static const int Texture = 1;
static const int DrawEnv = Texture * 2;
}

/**
 * Flags to indicate that state has changed.  Note that these flags do
 * <b>not</b> indicate the value of a property, e.g., setting the TexEnabled
 * flag does not mean that texturing is enabled, only that the value has changed.
 */
namespace RendererPropFlags {
static const int NumLights       = 1;
static const int TexEnabled      = NumLights * 2;
static const int LightingEnabled = TexEnabled * 2;
static const int SpecularEnabled = LightingEnabled * 2;
static const int PerVtxMaterial  = SpecularEnabled * 2;
static const int CullFaceEnabled = PerVtxMaterial * 2;
static const int Prim            = CullFaceEnabled * 2;
static const int ArrayAccessType = Prim * 2;
static const int ClippingEnabled = ArrayAccessType * 2;
}

/********************************************
 * CGLContext
 */

class CImmMatrixStack;
class CDListMatrixStack;
class CMatrixStack;

class CImmLighting;
class CDListLighting;
class CLighting;

class CImmGeomManager;
class CDListGeomManager;
class CGeomManager;

class CMaterialManager;
class CDListManager;
class CTexManager;

class CImmDrawContext;
class CDListDrawContext;
class CDrawContext;

class CDisplayContext;

class CGLContext {
    CImmMatrixStack *ProjectionMatStack, *ModelViewMatStack;
    CDListMatrixStack* DListMatStack;
    CMatrixStack *CurMatrixStack, *SavedCurMatStack;

    CImmLighting* ImmLighting;
    CDListLighting* DListLighting;
    CLighting* CurLighting;

    CImmGeomManager* ImmGManager;
    CDListGeomManager* DListGManager;
    CGeomManager* CurGManager;

    CMaterialManager* MaterialManager;
    CDListManager* DListManager;
    CTexManager* TexManager;

    CImmDrawContext* ImmDrawContext;
    CDListDrawContext* DListDrawContext;
    CDrawContext* CurDrawContext;

    CDisplayContext* DisplayContext;

    // state changes

    tU32 RendererContextChanged, SavedRendererContextChanges;
    tU32 GsContextChanged, SavedGsContextChanges;
    tU32 RendererPropsChanged, SavedRendererPropsChanges;
    bool StateChangesArePushed;

    inline void PushStateChanges()
    {
        mErrorIf(StateChangesArePushed, "Trying to push state changes when already pushed.");
        SavedRendererContextChanges = RendererContextChanged;
        SavedGsContextChanges       = GsContextChanged;
        SavedRendererPropsChanges   = RendererPropsChanged;
        StateChangesArePushed       = true;
    }
    inline void PopStateChanges()
    {
        mErrorIf(!StateChangesArePushed,
            "Trying to pop state changes that haven't been pushed.");
        RendererContextChanged = SavedRendererContextChanges;
        GsContextChanged       = SavedGsContextChanges;
        RendererPropsChanged   = SavedRendererPropsChanges;
        StateChangesArePushed  = false;
    }

    // rendering loop management

    bool IsCurrentFieldEven;
    unsigned int CurrentFrameNumber;

    // double-buffered dma packets for rendering use
    static const int kDmaPacketMaxQwordLength = 65000;
    static CVifSCDmaPacket *CurPacket, *LastPacket,
        *Vif1Packet, *SavedVif1Packet,
        *ImmVif1Packet;

    // double-buffered list of draw environment ptrs so that
    // the dma chains can be reused in different draw buffers,
    // depth buffers, pixel formats, etc.
    // If only they didn't pack so many logically distinct
    // properties into the same registers I wouldn't have to
    // do this!!  Damn!
    static const int kMaxDrawEnvChanges = 100;
    void* DrawEnvPtrs0[kMaxDrawEnvChanges];
    void* DrawEnvPtrs1[kMaxDrawEnvChanges];
    void **CurDrawEnvPtrs, **LastDrawEnvPtrs;
    int NumCurDrawEnvPtrs, NumLastDrawEnvPtrs;

    // list of memory to free after this frame is finished
    static const int kMaxBuffersToBeFreed = 1024;
    int CurBuffer;
    void* BuffersToBeFreed[2][kMaxBuffersToBeFreed];
    int NumBuffersToBeFreed[2];

    /// this value will be written to the signal register as the last
    /// item in the dma chain.  The method to query its value is below.
    static const tU64 Ps2glSignalId = 0xffffffff00000000 | (tU32)'G' << 24 | (tU32)'L' << 16;

    /// Semaphores signaled by the gs int handler
    static int RenderingFinishedSemaId, ImmediateRenderingFinishedSemaId, VsyncSemaId;

    static int GsIntHandler(int cause);

    void FreeWaitingBuffersAndSwap();

    void EndVif1Packet(unsigned short signalNum);

    typedef void (*tRenderingFinishedCallback)(void);
    static tRenderingFinishedCallback RenderingFinishedCallback;

public:
    CGLContext(int immBufferQwordSize, int immDrawBufferQwordSize);
    ~CGLContext();

    void SetMatrixMode(GLenum mode);
    inline CMatrixStack& GetCurMatrixStack() { return *CurMatrixStack; }
    inline CImmMatrixStack& GetModelViewStack() { return *ModelViewMatStack; }
    inline CImmMatrixStack& GetProjectionStack() { return *ProjectionMatStack; }

    inline CLighting& GetLighting() { return *CurLighting; }
    inline CImmLighting& GetImmLighting() { return *ImmLighting; }
    inline CDListLighting& GetDListLighting() { return *DListLighting; }

    inline CGeomManager& GetGeomManager() { return *CurGManager; }
    inline CImmGeomManager& GetImmGeomManager() { return *ImmGManager; }
    inline CDListGeomManager& GetDListGeomManager() { return *DListGManager; }

    inline CMaterialManager& GetMaterialManager() { return *MaterialManager; }

    inline CDListManager& GetDListManager() { return *DListManager; }

    inline CTexManager& GetTexManager() { return *TexManager; }

    inline CDrawContext& GetDrawContext() { return *CurDrawContext; }
    inline CImmDrawContext& GetImmDrawContext() { return *ImmDrawContext; }
    inline CDListDrawContext& GetDListDrawContext() { return *DListDrawContext; }

    inline CDisplayContext& GetDisplayContext() { return *DisplayContext; }

    inline bool InDListDef() const { return CurGManager != (CGeomManager*)ImmGManager; }
    void BeginDListDef(unsigned int listID, GLenum mode);
    void EndDListDef();

    void BeginImmediateGeometry();
    void EndImmediateGeometry();
    void RenderImmediateGeometry();
    void FinishRenderingImmediateGeometry(bool forceImmediateStop);

    void BeginGeometry();
    void EndGeometry();
    void RenderGeometry();
    void FinishRenderingGeometry(bool forceImmediateStop);

    void AddingDrawEnvToPacket(void* de)
    {
        mErrorIf(NumCurDrawEnvPtrs == kMaxDrawEnvChanges,
            "Too many draw environment changes.  Need to increase kMaxDrawEnvChanges");
        CurDrawEnvPtrs[NumCurDrawEnvPtrs++] = de;
    }
    void** GetDrawEnvPtrs() { return LastDrawEnvPtrs; }
    int GetNumDrawEnvPtrs() const { return NumLastDrawEnvPtrs; }

    bool GetCurrentFieldIsEven() const { return IsCurrentFieldEven; }

    /**
       * This is the upper 16 bits of the 32-bit values written to the signal
       * register by ps2gl.  (The lower 16 bits are used to differentiate signals.)
       */
    static tU16 GetPs2glSignalId() { return (tU16)(Ps2glSignalId >> 16); }

    /**
       * util - add a block of memory to a list to be freed at the end of the
       * frame
       */
    inline void AddBufferToBeFreed(void* buf)
    {
        mAssert(NumBuffersToBeFreed[CurBuffer] < kMaxBuffersToBeFreed);
        BuffersToBeFreed[CurBuffer][NumBuffersToBeFreed[CurBuffer]++] = buf;
    }

    inline static void SetRenderingFinishedCallback(tRenderingFinishedCallback cb)
    {
        RenderingFinishedCallback = cb;
    }

    // state updates

    inline void NumLightsChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::NumLights;
        RendererPropsChanged |= RendererPropFlags::NumLights;
    }
    inline void LightPropChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::LightPropChanged;
    }
    inline void GlobalAmbChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::GlobalAmb;
    }
    inline void CurMaterialChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::CurMaterial;
    }
    inline void XformChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::Xform;
    }
    inline void PrimChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::Prim;
        RendererPropsChanged |= RendererPropFlags::Prim;
    }
    inline void ShadingChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::Shading;
    }
    inline void TexEnabledChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::TexEnabled;
        RendererPropsChanged |= RendererPropFlags::TexEnabled;
        GsContextChanged |= GsCtxtFlags::Texture;
    }
    inline void LightingEnabledChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::LightingEnabled;
        RendererPropsChanged |= RendererPropFlags::LightingEnabled;
    }
    inline void BlendEnabledChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::AlphaBlending;
    }
    inline void DrawEnvChanged()
    {
        GsContextChanged |= GsCtxtFlags::DrawEnv;
    }
    inline void AlphaTestEnabledChanged()
    {
        GsContextChanged |= GsCtxtFlags::DrawEnv;
    }
    inline void DepthTestEnabledChanged()
    {
        GsContextChanged |= GsCtxtFlags::DrawEnv;
    }
    inline void DrawInterlacedChanged()
    {
        GsContextChanged |= GsCtxtFlags::DrawEnv;
    }
    inline void AlphaTestFuncChanged()
    {
        GsContextChanged |= GsCtxtFlags::DrawEnv;
    }
    inline void DepthWriteEnabledChanged()
    {
        GsContextChanged |= GsCtxtFlags::DrawEnv;
    }
    inline void FrameBufferDrawMaskChanged()
    {
        GsContextChanged |= GsCtxtFlags::DrawEnv;
    }
    inline void SpecularEnabledChanged()
    {
        RendererPropsChanged |= RendererPropFlags::SpecularEnabled;
    }
    inline void TextureChanged()
    {
        GsContextChanged |= GsCtxtFlags::Texture;
    }
    inline void BlendModeChanged()
    {
        GsContextChanged |= GsCtxtFlags::DrawEnv;
    }
    inline void DrawBufferChanged()
    {
        GsContextChanged |= GsCtxtFlags::DrawEnv;
    }
    inline void PerVtxMaterialChanged()
    {
        RendererPropsChanged |= RendererPropFlags::PerVtxMaterial;
    }
    inline void ClippingEnabledChanged()
    {
        RendererPropsChanged |= RendererPropFlags::ClippingEnabled;
        RendererContextChanged |= RendererCtxtFlags::ClippingEnabled;
    }
    inline void CullFaceEnabledChanged()
    {
        RendererPropsChanged |= RendererPropFlags::CullFaceEnabled;
    }
    inline void CullFaceDirChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::CullFaceDir;
    }
    inline void ArrayAccessChanged()
    {
        RendererPropsChanged |= RendererPropFlags::ArrayAccessType;
    }
    inline void PolyModeChanged()
    {
        RendererContextChanged |= RendererCtxtFlags::Prim;
    }

    // ps2 rendering

    inline tU32 GetRendererContextChanged() const { return RendererContextChanged; }
    inline void SetRendererContextChanged(bool changed)
    {
        RendererContextChanged = (changed) ? 0xff : 0;
    }

    inline tU32 GetGsContextChanged() const { return GsContextChanged; }
    inline void SetGsContextChanged(bool changed)
    {
        GsContextChanged = (changed) ? 0xff : 0;
    }

    inline tU32 GetRendererPropsChanged() const { return RendererPropsChanged; }
    inline void SetRendererPropsChanged(bool changed)
    {
        RendererPropsChanged = (changed) ? 0xff : 0;
    }

    inline void PushVif1Packet()
    {
        mAssert(SavedVif1Packet == NULL);
        SavedVif1Packet = Vif1Packet;
    }
    inline void PopVif1Packet()
    {
        mAssert(SavedVif1Packet != NULL);
        Vif1Packet      = SavedVif1Packet;
        SavedVif1Packet = NULL;
    }
    inline void SetVif1Packet(CVifSCDmaPacket& packet) { Vif1Packet = &packet; }
    inline CVifSCDmaPacket& GetVif1Packet() { return *Vif1Packet; }

    void WaitForVSync();
    void SwapBuffers();
};

// global pointer to the GLContext
extern CGLContext* pGLContext;

#endif // ps2gl_context_h
