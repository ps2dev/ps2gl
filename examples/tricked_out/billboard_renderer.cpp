/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

/********************************************
 * includes
 */

#include "ps2s/math.h"

#include "ps2gl/drawcontext.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/material.h"
#include "ps2gl/texture.h"

#include "GL/glut.h"

#include "billboard_renderer.h"
#include "billboard_renderer_mem.h"

/********************************************
 * code
 */

void* CBillboardRenderer::Microcode = NULL;

extern "C" void vsmBillboards();
extern "C" void vsmBillboardsEnd();

CBillboardRenderer::CBillboardRenderer()
    : CLinearRenderer(Microcode,
          1, // 1 input quad per vertex
          0, // the number of output quads per vert doesn't actually matter
          // (it's only used for strips)
          kInputStart,
          kInputBufSize - kInputStart,
          "billboard renderer")
{
    Requirements = 0;                      // no requirements
    Capabilities = kBillboardPrimTypeFlag; // provides the "kBillboardPrimType" capability
}

CBillboardRenderer*
CBillboardRenderer::Register()
{
#ifndef PS2_LINUX
    Microcode = (void*)vsmBillboards;
#else
    // under linux, since the renderer is just linked in with the rest of the code,
    // it is put in normal virtual mem by the loader.  However, we need to dma it
    // to vu1, so allocate some dma'able memory here and copy the renderer over.
    unsigned int microcodeSize = (unsigned int)vsmBillboardsEnd - (unsigned int)vsmBillboards;
    Microcode                  = pglutAllocDmaMem(microcodeSize);
    memcpy(Microcode, (void*)vsmBillboards, microcodeSize);
#endif

    // create a renderer and register it

    CBillboardRenderer* renderer = new CBillboardRenderer;
    pglRegisterRenderer(renderer);

    // register the prim type

    pglRegisterCustomPrimType(kBillboardPrimType, // the prim type we will pass to ps2gl (glBegin...)
        kBillboardPrimTypeFlag,                   // the corresponding renderer requirement
        ~(tU64)0xffffffff,                        // we only care about the custom stuff (upper 32 bits)
        true);                                    // ok to merge multiple calls when possible

    return renderer;
}

void CBillboardRenderer::DrawLinearArrays(CGeometryBlock& block)
{
    int wordsPerVert   = 4;
    int wordsPerNormal = 0, wordsPerTex = 0, wordsPerColor = 0;

    CVifSCDmaPacket& packet = pGLContext->GetVif1Packet();
    InitXferBlock(packet, wordsPerVert, wordsPerNormal, wordsPerTex, wordsPerColor);

    // get max number of vertices per vu1 buffer

    int maxUnpackVerts = 256;
    // max number of vertex data in vu1 memory
    int vu1QuadsPerVert      = InputQuadsPerVert;
    int inputBufSize         = InputGeomBufSize;
    int maxVu1VertsPerBuffer = inputBufSize / vu1QuadsPerVert;
    // max vertices per buffer
    int maxVertsPerBuffer = Math::Min(maxUnpackVerts, maxVu1VertsPerBuffer);
    maxVertsPerBuffer -= 3;
    // make sure we don't end in the middle of a polygon
    maxVertsPerBuffer -= maxVertsPerBuffer % block.GetNumVertsPerPrim();

    // since ps2gl doesn't know anything about this new prim type,
    // we have to set a few things before continuing.. (eventually I'd
    // like to move this to the registration step)

    block.SetNumVertsPerPrim(1);
    block.SetNumVertsToRestartStrip(0);

    // draw

    DrawBlock(packet, block, maxVertsPerBuffer);
}

void CBillboardRenderer::InitContext(GLenum primType, tU32 rcChanges, bool userRcChanged)
{
    CGLContext& glContext        = *pGLContext;
    CVifSCDmaPacket& packet      = glContext.GetVif1Packet();
    CImmDrawContext& drawContext = glContext.GetImmDrawContext();

    packet.Cnt();
    {
        AddVu1RendererContext(packet, primType, kContextStart);

        // overwrite the giftag built by CBaseRenderer::AddVu1RendererContext()...
        // ..it knows not what it does.

        bool alpha      = drawContext.GetBlendEnabled();
        bool useTexture = glContext.GetTexManager().GetTexEnabled();
        GS::tPrim prim  = { PRIM : 6, IIP : 0, TME : useTexture, FGE : 0, ABE : alpha, AA1 : 0, FST : 0, CTXT : 0, FIX : 0 };
        tGifTag giftag  = { NLOOP : 0, EOP : 1, pad0 : 0, id : 0, PRE : 1, PRIM : *(tU64*)&prim, FLG : 0, NREG : 4, REGS0 : 2, REGS1 : 4, REGS2 : 2, REGS3 : 4 };

        packet.Pad96();
        packet.OpenUnpack(Vifs::UnpackModes::v4_32, kGifTag, Packet::kSingleBuff);
        packet += giftag;
        packet.CloseUnpack(1);

        // since we'll be using "packed" mode gif packets, the Q register for (s,t,q)
        // texture mapping is stored internally when given as an (s,t,q) triplet, then
        // set when the gif gets the next (r,g,b,a).  However, since the color will
        // be the same for all packets we only want to set it once.  Also, we don't
        // need perspective correction of the texture since billboards have constant
        // depth.  SO, let's set up the Q register to 1.0 here along with the color
        // from glColor*().

        packet.Pad96();
        packet.OpenDirect();
        {
            giftag.NLOOP = 1;
            giftag.NREG  = 2;
            giftag.REGS0 = 2; // stQ
            giftag.REGS1 = 1; // rgba (outputs rgba + Q)
            packet += giftag;

            // the s and t don't matter, just the Q
            packet += cpu_vec_4(0, 0, 1, 0);

            // set the color, max is 128 because this is unity when texture mapping is enabled
            cpu_vec_4 color = glContext.GetMaterialManager().GetCurColor() * 128.0f;
            packet += (unsigned int)color[0];
            packet += (unsigned int)color[1];
            packet += (unsigned int)color[2];
            packet += (unsigned int)color[3];
        }
        packet.CloseDirect();

        packet.Mscal(0);
        packet.Flushe();

        packet.Base(kDoubleBufBase);
        packet.Offset(kDoubleBufOffset);

        packet.Pad128();
    }
    packet.CloseTag();

    // we only want to transfer vertices
    XferColors    = false;
    XferNormals   = false;
    XferTexCoords = false;
}
