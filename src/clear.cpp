/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/math.h"
#include "ps2s/packet.h"

#include "ps2gl/clear.h"
#include "ps2gl/drawcontext.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/immgmanager.h"

CClearEnv::CClearEnv()
{
    pDrawEnv = new GS::CDrawEnv(GS::kContext2);
    pDrawEnv->SetDepthTestPassMode(GS::ZTest::kAlways);

    pSprite = new CSprite(GS::kContext2, 0, 0, 0, 0);
    pSprite->SetUseTexture(false);
    unsigned int clearColor[4] = { 0, 0, 0, 0 };
    pSprite->SetColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    pSprite->SetDepth(0);
}

CClearEnv::~CClearEnv()
{
    delete pDrawEnv;
    delete pSprite;
}

void CClearEnv::SetDimensions(int width, int height)
{
    pDrawEnv->SetFrameBufferDim(width, height);
    pSprite->SetVertices(0, 0, width, height);
}

void CClearEnv::SetFrameBufPsm(GS::tPSM psm)
{
    pDrawEnv->SetFrameBufferPSM(psm);
}

void CClearEnv::SetDepthBufPsm(GS::tPSM psm)
{
    pDrawEnv->SetDepthBufferPSM(psm);
}

void CClearEnv::ClearBuffers(unsigned int bitMask)
{
    if (bitMask & GL_DEPTH_BUFFER_BIT)
        pDrawEnv->EnableDepthTest();
    else
        pDrawEnv->DisableDepthTest();

    if (bitMask & GL_COLOR_BUFFER_BIT)
        pDrawEnv->SetFrameBufferDrawMask(0);
    else
        pDrawEnv->SetFrameBufferDrawMask(0xffffffff);

    CVifSCDmaPacket& packet = pGLContext->GetVif1Packet();
    pGLContext->AddingDrawEnvToPacket((uint128_t*)pGLContext->GetVif1Packet().GetNextPtr() + 1);
    pDrawEnv->SendSettings(packet);
    pSprite->Draw(packet);
}

/********************************************
 * C gl api
 */

void glClearColor(GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha)
{
    GL_FUNC_DEBUG("%s(%f,%f,%f,%f)\n", __FUNCTION__, red, green, blue, alpha);

    CClearEnv& clearEnv = pGLContext->GetImmDrawContext().GetClearEnv();

    using namespace Math;
    clearEnv.SetClearColor(Clamp(red, 0.0f, 1.0f),
        Clamp(green, 0.0f, 1.0f),
        Clamp(blue, 0.0f, 1.0f),
        Clamp(alpha, 0.0f, 1.0f));
}

void glClearDepth(GLclampd depth)
{
    GL_FUNC_DEBUG("%s(%f)\n", __FUNCTION__, depth);

    CClearEnv& clearEnv = pGLContext->GetImmDrawContext().GetClearEnv();

    clearEnv.SetClearDepth((float)depth);
}

void glClear(GLbitfield mask)
{
    GL_FUNC_DEBUG("%s(0x%x)\n", __FUNCTION__, mask);

    pGLContext->GetImmDrawContext().GetClearEnv().ClearBuffers(mask);
}
