/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_renderermanager_h
#define ps2gl_renderermanager_h

// PLIN
// #include "eestruct.h"

#include "ps2s/vif.h"

#include "GL/gl.h"
#include "GL/ps2gl.h"

#include "ps2gl/base_renderer.h"
#include "ps2gl/renderer.h"

/********************************************
 * CRendererManager
 */

class CGLContext;
class CImmGeomManager;
class CVifSCDmaPacket;
class CGeometryBlock;
class CRenderer;

typedef enum { kDirectional,
    kPoint,
    kSpot } tLightType;

typedef struct {
    uint64_t capabilities;
    uint64_t requirements;
    CRenderer* renderer;
} tRenderer;

class CRendererManager {
    CGLContext& GLContext;

    CRendererProps RendererRequirements;
    bool RendererReqsHaveChanged;
    uint64_t CurUserPrimReqs, CurUserPrimReqMask;

    static const int kMaxDefaultRenderers = 64;
    static const int kMaxUserRenderers    = PGL_MAX_CUSTOM_RENDERERS;
    tRenderer DefaultRenderers[kMaxDefaultRenderers];
    tRenderer UserRenderers[kMaxUserRenderers];
    int NumDefaultRenderers, NumUserRenderers;
    const tRenderer *CurrentRenderer, *NewRenderer;

    void RegisterDefaultRenderer(CRenderer* renderer);

public:
    CRendererManager(CGLContext& context);

    void RegisterUserRenderer(CRenderer* renderer);

    bool UpdateNewRenderer();
    void MakeNewRendererCurrent();
    void LoadRenderer(CVifSCDmaPacket& packet);

    CRenderer& GetCurRenderer() { return *(CurrentRenderer->renderer); }
    CRendererProps GetRendererReqs() const { return RendererRequirements; }

    bool IsCurRendererCustom() const { return ((uint32_t)CurrentRenderer >= (uint32_t)UserRenderers); }

    // state updates

    void EnableCustom(uint64_t flag);
    void DisableCustom(uint64_t flag);

    void NumLightsChanged(tLightType type, int num);
    void PrimChanged(unsigned int prim);
    void TexEnabledChanged(bool enabled);
    void LightingEnabledChanged(bool enabled);
    void SpecularEnabledChanged(bool enabled);
    void PerVtxMaterialChanged(RendererProps::tPerVtxMaterial matType);
    void ClippingEnabledChanged(bool enabled);
    void CullFaceEnabledChanged(bool enabled);
    void ArrayAccessChanged(RendererProps::tArrayAccess accessType);
};

#endif // ps2gl_renderermanager_h
