/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <string.h>

#include "dma.h"
#include "graph.h"
#include "kernel.h"

#include "GL/ps2gl.h"

#include "ps2s/displayenv.h"
#include "ps2s/drawenv.h"
#include "ps2s/gsmem.h"
#include "ps2s/math.h"
#include "ps2s/packet.h"
#include "ps2s/ps2stuff.h"
#include "ps2s/texture.h"
#include "ps2s/types.h"

#include "ps2gl/displaycontext.h"
#include "ps2gl/dlgmanager.h"
#include "ps2gl/dlist.h"
#include "ps2gl/drawcontext.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/gmanager.h"
#include "ps2gl/immgmanager.h"
#include "ps2gl/lighting.h"
#include "ps2gl/material.h"
#include "ps2gl/matrix.h"
#include "ps2gl/texture.h"

/********************************************
 * globals
 */

/********************************************
 * CGLContext
 */

// static members

CVifSCDmaPacket *CGLContext::CurPacket, *CGLContext::LastPacket,
    *CGLContext::Vif1Packet = NULL, *CGLContext::SavedVif1Packet = NULL,
    *CGLContext::ImmVif1Packet;

int CGLContext::RenderingFinishedSemaId          = -1;
int CGLContext::ImmediateRenderingFinishedSemaId = -1;
int CGLContext::VsyncSemaId                      = -1;

CGLContext::tRenderingFinishedCallback CGLContext::RenderingFinishedCallback = NULL;

CGLContext::CGLContext(int immBufferQwordSize, int immDrawBufferQwordSize)
    : StateChangesArePushed(false)
    , IsCurrentFieldEven(true)
    , CurrentFrameNumber(0)
    , CurBuffer(0)
{
    CurPacket = new CVifSCDmaPacket(kDmaPacketMaxQwordLength, DMAC::Channels::vif1,
        Packet::kXferTags, Core::MemMappings::UncachedAccl);
    LastPacket = new CVifSCDmaPacket(kDmaPacketMaxQwordLength, DMAC::Channels::vif1,
        Packet::kXferTags, Core::MemMappings::UncachedAccl);
    Vif1Packet = CurPacket;

    ImmVif1Packet = new CVifSCDmaPacket(immDrawBufferQwordSize, DMAC::Channels::vif1,
        Packet::kXferTags, Core::MemMappings::UncachedAccl);

    CurDrawEnvPtrs     = DrawEnvPtrs0;
    LastDrawEnvPtrs    = DrawEnvPtrs1;
    NumCurDrawEnvPtrs  = 0;
    NumLastDrawEnvPtrs = 0;

    ImmGManager   = new CImmGeomManager(*this, immBufferQwordSize);
    DListGManager = new CDListGeomManager(*this);
    CurGManager   = ImmGManager;

    ProjectionMatStack = new CImmMatrixStack(*this);
    ModelViewMatStack  = new CImmMatrixStack(*this);
    DListMatStack      = new CDListMatrixStack(*this);
    CurMatrixStack     = ModelViewMatStack;
    SavedCurMatStack   = NULL;

    ImmLighting   = new CImmLighting(*this);
    DListLighting = new CDListLighting(*this);
    CurLighting   = ImmLighting;
    // defaults
    CLight& light = ImmLighting->GetLight(0);
    light.SetDiffuse(cpu_vec_xyzw(1.0f, 1.0f, 1.0f, 1.0f));
    light.SetSpecular(cpu_vec_xyzw(1.0f, 1.0f, 1.0f, 1.0f));

    MaterialManager = new CMaterialManager(*this);
    DListManager    = new CDListManager;
    TexManager      = new CTexManager(*this);

    ImmDrawContext   = new CImmDrawContext(*this);
    DListDrawContext = new CDListDrawContext(*this);
    CurDrawContext   = ImmDrawContext;

    DisplayContext = new CDisplayContext(*this);

    SetRendererContextChanged(true);
    SetGsContextChanged(true);
    SetRendererPropsChanged(true);

    // util
    NumBuffersToBeFreed[0] = NumBuffersToBeFreed[1] = 0;

    GS::Init();

    // create a few semaphores

    struct t_ee_sema newSemaphore    = { 0, 1, 0 }; // but maxCount doesn't work?
    VsyncSemaId                      = CreateSema(&newSemaphore);
    RenderingFinishedSemaId          = CreateSema(&newSemaphore);
    ImmediateRenderingFinishedSemaId = CreateSema(&newSemaphore);
    mErrorIf(VsyncSemaId == -1
            || RenderingFinishedSemaId == -1
            || ImmediateRenderingFinishedSemaId == -1,
        "Failed to create ps2gl semaphores.");

    // add an interrupt handler for gs "signal" exceptions

    AddIntcHandler(INTC_GS, CGLContext::GsIntHandler, 0 /*first handler*/);
    EnableIntc(INTC_GS);
    // clear any signal/vsync exceptions and wait for the next
    *(volatile unsigned int*)GS::ControlRegs::csr = 9;
    // enable signal and vsync exceptions
    *(volatile unsigned int*)GS::ControlRegs::imr = 0x7600;
}

CGLContext::~CGLContext()
{
    delete CurPacket;
    delete LastPacket;

    delete ImmGManager;
    delete DListGManager;

    delete ProjectionMatStack;
    delete ModelViewMatStack;
    delete DListMatStack;

    delete ImmLighting;
    delete DListLighting;

    delete MaterialManager;
    delete DListManager;
    delete TexManager;

    delete ImmDrawContext;
    delete DListDrawContext;

    delete DisplayContext;
}

/********************************************
 * display lists
 */

void CGLContext::BeginDListDef(unsigned int listID, GLenum mode)
{
    DListManager->NewList(listID, mode);

    PushStateChanges();

    // not so sure about these two, but let's be cautious
    SetRendererContextChanged(true);
    SetGsContextChanged(true);
    // definately need this to force an update - indexed/linear arrays
    SetRendererPropsChanged(true);

    MaterialManager->BeginDListDef();
    TexManager->BeginDListDef();
    DListGManager->BeginDListDef();

    CurLighting      = DListLighting;
    CurGManager      = DListGManager;
    SavedCurMatStack = CurMatrixStack;
    CurMatrixStack   = DListMatStack;
    CurDrawContext   = DListDrawContext;
}

void CGLContext::EndDListDef()
{
    DListGManager->EndDListDef();
    MaterialManager->EndDListDef();
    TexManager->EndDListDef();

    CurLighting    = ImmLighting;
    CurGManager    = ImmGManager;
    CurMatrixStack = SavedCurMatStack;
    CurDrawContext = ImmDrawContext;

    PopStateChanges();

    DListManager->EndList();
}

/********************************************
 * matrix mode
 */

class CSetMatrixModeCmd : public CDListCmd {
    GLenum Mode;

public:
    CSetMatrixModeCmd(GLenum mode)
        : Mode(mode)
    {
    }
    CDListCmd* Play()
    {
        glMatrixMode(Mode);
        return CDListCmd::GetNextCmd(this);
    }
};

void CGLContext::SetMatrixMode(GLenum mode)
{
    if (InDListDef()) {
        DListManager->GetOpenDList() += CSetMatrixModeCmd(mode);
    } else {
        switch (mode) {
        case GL_MODELVIEW:
            CurMatrixStack = ModelViewMatStack;
            break;
        case GL_PROJECTION:
            CurMatrixStack = ProjectionMatStack;
            break;
        default:
            mNotImplemented();
        }
    }
}

/********************************************
 * immediate geometry
 */

void CGLContext::BeginImmediateGeometry()
{
    //     mErrorIf( InDListDef == true,
    //  	     "pglBeginImmediateGeom can't be called in a display list definition." );

    // flush any pending geometry
    GetImmGeomManager().Flush();

    PushVif1Packet();
    SetVif1Packet(*ImmVif1Packet);

    ImmVif1Packet->Reset();
}

void CGLContext::EndImmediateGeometry()
{
    mAssert(Vif1Packet == ImmVif1Packet);

    EndVif1Packet(2);

    PopVif1Packet();
}

void CGLContext::RenderImmediateGeometry()
{
    ImmVif1Packet->End();
    ImmVif1Packet->Pad128();
    ImmVif1Packet->CloseTag();

    ImmVif1Packet->Send();
}

void CGLContext::FinishRenderingImmediateGeometry(bool forceImmediateStop)
{
    mWarnIf(forceImmediateStop, "Interrupting currently rendering dma chain not supported yet");
    mNotImplemented();
}

/********************************************
 * normal geometry
 */

void CGLContext::BeginGeometry()
{
    // reset packets that will be drawn to during this frame

    CurPacket->Reset();
}

void CGLContext::EndGeometry()
{
    EndVif1Packet(1);
}

void CGLContext::EndVif1Packet(unsigned short signalNum)
{
    //printf("%s(%d)\n", __FUNCTION__, signalNum);

    // flush any pending geometry
    GetImmGeomManager().Flush();

    // end current packet
    // write our id to the signal register and trigger an
    // exception on the core when this dma chain reaches the end

    tGifTag giftag;
    giftag.NLOOP = 1;
    giftag.EOP   = 1;
    giftag.PRE   = 0;
    giftag.FLG   = 0; // packed
    giftag.NREG  = 1;
    giftag.REGS0 = 0xe; // a+d

    Vif1Packet->End();
    Vif1Packet->Flush();
    Vif1Packet->OpenDirect();
    {
        *Vif1Packet += giftag;
        *Vif1Packet += Ps2glSignalId | signalNum;
        *Vif1Packet += (tU64)0x60; // signal
    }
    Vif1Packet->CloseDirect();
    Vif1Packet->CloseTag();
}

void CGLContext::RenderGeometry()
{
    //printf("%s\n", __FUNCTION__);

    // make sure the semaphore we'll signal on completion is zero now
    while (PollSema(RenderingFinishedSemaId) != -1)
        ;

    LastPacket->Send();
}

int CGLContext::GsIntHandler(int cause)
{
    int ret = 0;

    //printf("%s(%d)\n", __FUNCTION__, cause);

    tU32 csr = *(volatile tU32*)GS::ControlRegs::csr;
    // is this a signal interrupt?
    if (csr & 1) {
        // is it one of ours?
        tU64 sigLblId = *(volatile tU64*)GS::ControlRegs::siglblid;
        if ((tU16)(sigLblId >> 16) == GetPs2glSignalId()) {
            switch (sigLblId & 0xffff) {
            case 1:
                iSignalSema(RenderingFinishedSemaId);
                if (RenderingFinishedCallback != NULL)
                    RenderingFinishedCallback();
                break;
            case 2:
                iSignalSema(ImmediateRenderingFinishedSemaId);
                break;
            default:
                mError("Unknown signal");
            }

            // clear our signal id
            sigLblId &= ~0xffffffff;
            *(volatile tU64*)GS::ControlRegs::siglblid = sigLblId;
            // clear the exception and wait for the next
            *(volatile unsigned int*)GS::ControlRegs::csr = 1;

            ret = -1; // don't call other handlers
        }
    }
    // is this a vsync interrupt?
    else if (csr & 8) {
        iSignalSema(VsyncSemaId);
        // clear the exception and wait for the next
        *(volatile unsigned int*)GS::ControlRegs::csr = 8;
    }

    // I'm not entirely sure why this is necessary, but if I don't do
    // it then framing out can cause the display thread to lock (I've
    // only verified it frozen waiting on pglFinishRenderingGeometry().)
    // The GS manual says that a second "signal" event occuring before
    // the first is cleared causes the gs to stop drawing, and the second
    // interrupt will not be raised until that interrupt (signal) is masked
    // and then unmasked.  Could something similar be true for gs events/
    // interrupts in general?  This needs to be here, not in the vsync branch.
    if (ret == -1) {
        *(volatile unsigned int*)GS::ControlRegs::imr = 0x7f00;
        *(volatile unsigned int*)GS::ControlRegs::imr = 0x7600;
    }

    return ret;
}

void CGLContext::FinishRenderingGeometry(bool forceImmediateStop)
{
    //printf("%s(%d)\n", __FUNCTION__, forceImmediateStop);

    mWarnIf(forceImmediateStop, "Interrupting currently rendering dma chain not supported yet");
    WaitSema(RenderingFinishedSemaId);
}

void CGLContext::WaitForVSync()
{
    //printf("%s\n", __FUNCTION__);

    // wait for beginning of v-sync
    WaitSema(VsyncSemaId);
    // sometimes if we miss a frame the semaphore gets incremented
    // more than once (because maxCount is ignored?) which causes the next
    // call to WaitForVSync to fall through immediately, which is kinda bad,
    // so make sure the count is zero after waiting.
    while (PollSema(VsyncSemaId) != -1)
        ;
    // sceGsSyncV(0);
    tU32 csr           = *(volatile tU32*)GS::ControlRegs::csr;
    IsCurrentFieldEven = (bool)((csr >> 13) & 1);
}

void CGLContext::SwapBuffers()
{
    //printf("%s\n", __FUNCTION__);

    // switch packet ptrs

    CVifSCDmaPacket* tempPkt = CurPacket;
    CurPacket                = LastPacket;
    LastPacket               = tempPkt;
    Vif1Packet               = CurPacket;

    // switch drawenv ptrs

    void** tempDEPtrs  = CurDrawEnvPtrs;
    CurDrawEnvPtrs     = LastDrawEnvPtrs;
    LastDrawEnvPtrs    = tempDEPtrs;
    NumLastDrawEnvPtrs = NumCurDrawEnvPtrs;
    NumCurDrawEnvPtrs  = 0;

    // tell some modules that it's time to flip

    GetImmGeomManager().SwapBuffers();
    GetDListManager().SwapBuffers();
    GetDisplayContext().SwapBuffers();
    GetImmDrawContext().SwapBuffers(IsCurrentFieldEven);

    // free memory that was waiting til end of frame
    FreeWaitingBuffersAndSwap();

    CurrentFrameNumber++;
}

void CGLContext::FreeWaitingBuffersAndSwap()
{
    //printf("%s\n", __FUNCTION__);

    CurBuffer = 1 - CurBuffer;

    for (int i = 0; i < NumBuffersToBeFreed[CurBuffer]; i++) {
        free(BuffersToBeFreed[CurBuffer][i]);
    }

    NumBuffersToBeFreed[CurBuffer] = 0;
}

/********************************************
 * ps2gl C interface
 */

/// global pointer to the GLContext
CGLContext* pGLContext = NULL;

/**
 * @addtogroup pgl_api pgl* API
 *
 * The pgl* functions affect the behavior of ps2gl and
 * provide access to ps2 features not well-suited to the gl api.
 *
 * The recommended way to use the ps2gl library is for the app
 * to use the pgl* functions to configure the library.  Alternatively,
 * for a quick start try the [very incomplete] glut implementation, which
 * will set things up using default values.
 *
 * @{
 */

/**
 * Initialize the ps2gl library.  You must call this before any other pgl* or gl* functions!
 * (When using glut, it will be called in glutInit() if the ps2gl library was not
 * initialized by the app previously.)
 * Also, the application is now responsible for resetting the machine, including the
 * display mode (putting the gs into the right output state, i.e., resolution and interlaced),
 * usually this just means calling sceGsResetGraph.
 * @param immBufferVertexSize ps2gl uses fixed-size internal buffers to store geometry;
 * this argument tells the library how much space to allocate.
 */
int pglInit(int immBufferVertexSize, int immDrawBufferQwordSize)
{
    ps2sInit();
    pGLContext = new CGLContext(immBufferVertexSize, immDrawBufferQwordSize);

    return true;
}

/**
 * Has pglInit() been called?
 * @return 1 if pglInit has been called, 0 otherwise
 */
int pglHasLibraryBeenInitted(void)
{
    return (pGLContext != NULL);
}

/**
 * Do any necessary clean up when finished using ps2gl.
 */
void pglFinish(void)
{
    if (pGLContext)
        delete pGLContext;
    ps2sFinish();
}

/**
 * Wait for dma transfers to vif1 to end.  Polls cop0, so it should not slow down
 * the transfer, unlike sceGsSyncPath().
 *
 * This call is for convenience only -- there is no need to call it if the app
 * can manage on its own.
 */
void pglWaitForVU1(void)
{
    dma_channel_fast_waits(DMAC::Channels::vif1);
}

/**
 * Wait for the vertical retrace.  Note that this call is <b>required</b>
 * for the interlacing to work properly.  (Called by glut.)
 */
void pglWaitForVSync(void)
{
    pGLContext->WaitForVSync();
}

/**
 * Signals the end of the current rendering loop and swaps anything
 * double-buffered (display, draw buffers).
 *
 * Note that this call is <b>required</b>.  (Called by glut.)
 */
void pglSwapBuffers(void)
{
    mErrorIf(pGLContext == NULL, "You need to call pglInit()");

    pGLContext->SwapBuffers();
}

/**
 * Set a function to be called back when rendering finishes.  <b>This
 * function will be called from the interrupt handler; be careful!</b>
 * @param a pointer to the callback function or NULL to clear
 */
void pglSetRenderingFinishedCallback(void (*cb)(void))
{
    pGLContext->SetRenderingFinishedCallback(cb);
}

/********************************************
 * immediate geometry
 */

void pglBeginImmediateGeometry(void)
{
    pGLContext->BeginImmediateGeometry();
}
void pglEndImmediateGeometry(void)
{
    pGLContext->EndImmediateGeometry();
}
void pglRenderImmediateGeometry(void)
{
    pGLContext->RenderImmediateGeometry();
}
void pglFinishRenderingImmediateGeometry(int forceImmediateStop)
{
    pGLContext->FinishRenderingImmediateGeometry((bool)forceImmediateStop);
}

/********************************************
 * normal geometry
 */

void pglBeginGeometry(void)
{
    pGLContext->BeginGeometry();
}
void pglEndGeometry(void)
{
    pGLContext->EndGeometry();
}
void pglRenderGeometry(void)
{
    pGLContext->RenderGeometry();
}
void pglFinishRenderingGeometry(int forceImmediateStop)
{
    pGLContext->FinishRenderingGeometry((bool)forceImmediateStop);
}

/********************************************
 * enable / disable
 */

void pglEnable(GLenum cap)
{
    switch (cap) {
    case PGL_CLIPPING:
        pGLContext->GetDrawContext().SetDoClipping(true);
        break;
    default:
        mError("Unknown option passed to pglEnable()");
    }
}

void pglDisable(GLenum cap)
{
    switch (cap) {
    case PGL_CLIPPING:
        pGLContext->GetDrawContext().SetDoClipping(false);
        break;
    default:
        mError("Unknown option passed to pglDisable()");
    }
}

/**
 * @}
 */

/********************************************
 * gl interface
 */

void glEnable(GLenum cap)
{
    //printf("%s(0x%x)\n", __FUNCTION__, cap);

    CLighting& lighting = pGLContext->GetLighting();

    switch (cap) {
    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
        lighting.GetLight(0x7 & cap).SetEnabled(true);
        break;
    case GL_LIGHTING:
        lighting.SetLightingEnabled(true);
        break;

    case GL_BLEND:
        pGLContext->GetDrawContext().SetBlendEnabled(true);
        break;

    case GL_COLOR_MATERIAL:
        pGLContext->GetMaterialManager().SetUseColorMaterial(true);
        break;
    case GL_RESCALE_NORMAL:
        pGLContext->GetDrawContext().SetRescaleNormals(true);
        break;

    case GL_TEXTURE_2D:
        pGLContext->GetTexManager().SetTexEnabled(true);
        break;

    case GL_NORMALIZE:
        pGLContext->GetGeomManager().SetDoNormalize(true);
        break;

    case GL_CULL_FACE:
        pGLContext->GetDrawContext().SetDoCullFace(true);
        break;

    case GL_ALPHA_TEST:
        pGLContext->GetDrawContext().SetAlphaTestEnabled(true);
        break;

    case GL_DEPTH_TEST:
    default:
        mNotImplemented();
        break;
    }
}

void glDisable(GLenum cap)
{
    //printf("%s(0x%x)\n", __FUNCTION__, cap);

    switch (cap) {
    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
        pGLContext->GetLighting().GetLight(0x7 & cap).SetEnabled(false);
        break;
    case GL_LIGHTING:
        pGLContext->GetLighting().SetLightingEnabled(false);
        break;

    case GL_BLEND:
        pGLContext->GetDrawContext().SetBlendEnabled(false);
        break;

    case GL_COLOR_MATERIAL:
        pGLContext->GetMaterialManager().SetUseColorMaterial(false);
        break;
    case GL_RESCALE_NORMAL:
        pGLContext->GetDrawContext().SetRescaleNormals(false);
        break;

    case GL_TEXTURE_2D:
        pGLContext->GetTexManager().SetTexEnabled(false);
        break;

    case GL_NORMALIZE:
        pGLContext->GetGeomManager().SetDoNormalize(false);
        break;

    case GL_CULL_FACE:
        pGLContext->GetDrawContext().SetDoCullFace(false);
        break;

    case GL_ALPHA_TEST:
        pGLContext->GetDrawContext().SetAlphaTestEnabled(false);
        break;

    case GL_DEPTH_TEST:
    default:
        mNotImplemented();
    }
}

void glHint(GLenum target, GLenum mode)
{
    //printf("%s(0x%x,0x%x)\n", __FUNCTION__, target, mode);

    mNotImplemented();
}

void glGetFloatv(GLenum pname, GLfloat* params)
{
    //printf("%s(0x%x,...)\n", __FUNCTION__, pname);

    switch (pname) {
    case GL_MODELVIEW_MATRIX:
        memcpy(params, &(pGLContext->GetModelViewStack().GetTop()), 16 * 4);
        break;
    case GL_PROJECTION_MATRIX:
        memcpy(params, &(pGLContext->GetProjectionStack().GetTop()), 16 * 4);
        break;
    default:
        mNotImplemented("pname %d", pname);
        break;
    }
}

void glGetIntegerv(GLenum pname, int* params)
{
    //printf("%s(0x%x,...)\n", __FUNCTION__, pname);

    mNotImplemented();
}

GLenum glGetError(void)
{
    //printf("%s()\n", __FUNCTION__);

    mWarn("glGetError does nothing");

    return 0;
}

const GLubyte* glGetString(GLenum name)
{
    //printf("%s(0x%x)\n", __FUNCTION__, name);

    mNotImplemented();
    return (GLubyte*)"not implemented";
}
