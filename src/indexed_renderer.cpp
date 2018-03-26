/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <stdio.h>

#include "ps2s/cpu_matrix.h"
#include "ps2s/math.h"

#include "ps2gl/glcontext.h"
#include "ps2gl/indexed_renderer.h"
#include "ps2gl/lighting.h"
#include "ps2gl/material.h"
#include "ps2gl/texture.h"

#include "vu1_mem_indexed.h"

CIndexedRenderer::CIndexedRenderer(void* packet, int packetSize, CRendererProps caps, CRendererProps reqs,
    int inQuadsPerVert, int outQuadsPerVert,
    const char* name)
    : CBaseRenderer(packet, packetSize, caps, reqs,
          inQuadsPerVert, outQuadsPerVert, kInputGeomStart, name)
{
    printf("Max number of vertices in indexed array is %d\n", (kInputBufSize - 4) / 3 - 3);
    printf("giftag = 0x%04x, context length = 0x%04x (%d)\n",
        kGifTag, kContextLength, kContextLength);
}

void CIndexedRenderer::InitContext(GLenum primType, tU32 rcChanges, bool userRcChanged)
{
    CGLContext& glContext   = *pGLContext;
    CVifSCDmaPacket& packet = glContext.GetVif1Packet();

    packet.Cnt();
    {
        AddVu1RendererContext(packet, primType, kContextStart);

        packet.Mscal(0);
        packet.Flushe();

        packet.Base(kDoubleBufBase);
        packet.Offset(kDoubleBufOffset);
    }
    packet.CloseTag();

    CacheRendererState();

    // more caching:  calculate and save the current constant vertex color

    CImmLighting& lighting = glContext.GetImmLighting();
    bool doLighting        = lighting.GetLightingEnabled();

    float maxColorValue = GetMaxColorValue(XferTexCoords);
    cpu_vec_4 globalAmb;
    if (doLighting)
        globalAmb = lighting.GetGlobalAmbient() * maxColorValue;
    else
        globalAmb.set(0, 0, 0, 0);

    CImmMaterial& material = glContext.GetMaterialManager().GetImmMaterial();
    cpu_vec_4 materialAmb  = material.GetAmbient();

    cpu_vec_4 materialEmm;
    if (doLighting)
        materialEmm = material.GetEmission() * maxColorValue;
    else
        materialEmm = glContext.GetMaterialManager().GetCurColor() * maxColorValue;

    ConstantVertColor = materialAmb * globalAmb + materialEmm;
}

int CIndexedRenderer::GetPacketQwordSize(const CGeometryBlock& geometry)
{
    // FIXME:  this is a pitiful hack for allocating enough memory
    return Math::Max(geometry.GetTotalVertices() / 70, 1) * 1000;
}

CRendererProps
CIndexedRenderer::GetRenderContextDeps()
{
    CRendererProps deps;
    deps                = (tU64)0;
    deps.Lighting       = 1;
    deps.Texture        = 1;
    deps.PerVtxMaterial = 1;

    return deps;
}

bool CIndexedRenderer::GetCachePackets(const CGeometryBlock& geometry)
{
    return !(pGLContext->GetImmLighting().GetLightingEnabled()
        && !geometry.GetNormalsAreValid());
}

void CIndexedRenderer::DrawIndexedArrays(CGeometryBlock& block)
{
    // TODO:
    //  - transfer strip lengths into temp area w's and set strip adc's
    //  - test with real data..

    // transfer the arrays

    CVifSCDmaPacket& packet = pGLContext->GetVif1Packet();

    int wordsPerVert   = block.GetWordsPerVertex();
    int wordsPerNormal = (block.GetNormalsAreValid()) ? block.GetWordsPerNormal() : 0;
    int wordsPerTex    = (block.GetTexCoordsAreValid()) ? block.GetWordsPerTexCoord() : 0;
    int wordsPerColor  = (block.GetColorsAreValid()) ? block.GetWordsPerColor() : 0;

    InitXferBlock(packet, wordsPerVert, wordsPerNormal, wordsPerTex, wordsPerColor);

    for (int curArray = 0; curArray < block.GetNumArrays(); curArray++) {

        const void *normals, *vertices, *texCoords, *colors;
        vertices  = block.GetVerticesAreValid() ? block.GetVertices(curArray) : NULL;
        normals   = block.GetNormalsAreValid() ? block.GetNormals(curArray) : NULL;
        texCoords = block.GetTexCoordsAreValid() ? block.GetTexCoords(curArray) : NULL;
        colors    = block.GetColorsAreValid() ? block.GetColors(curArray) : NULL;

        packet.Cnt();
        packet.Stcycl(1, 3).Nop();
        packet.CloseTag();

        int numIndices      = block.GetNumIndices(curArray);
        const void* indices = block.GetIndices(curArray);
        int numVertices     = block.GetArrayLength(curArray);

        XferBlock(packet,
            vertices, normals, texCoords, colors,
            kInputGeomStart,
            0, numVertices);

        // transfer the indices

        packet.Cnt();
        {
            packet.Stmod(Vifs::AddModes::kOffset);
            // don't write over the xyz's
            Vifs::tMask mask = { 3, 3, 3, 0,
                3, 3, 3, 0,
                3, 3, 3, 0,
                3, 3, 3, 0 };
            packet.Stmask(mask);
            static const float row[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            packet.Strow(row);
            packet.Pad128();
        }
        packet.CloseTag();

        int numIndexQwords = numIndices / 16 + (numIndices % 16 > 0);
        packet.Ref(Core::MakePtrNormal((const unsigned int*)indices), numIndexQwords);
        {
            packet.Stcycl(1, 1);
            packet.OpenUnpack(Vifs::UnpackModes::s_16, kInputGeomStart,
                Packet::kDoubleBuff, Packet::kMasked);
            // packet.CloseUnpack( numIndices/2 );
            packet.CloseUnpack(numIndexQwords * 8);
        }

        // transfer a buffer header & start renderer

        packet.Cnt();
        {
            // buffer header

            packet.Stmod(Vifs::AddModes::kNone);
            packet.OpenUnpack(Vifs::UnpackModes::v4_32, 0, Packet::kDoubleBuff);
            {
                packet += numVertices;
                packet += numIndices / 2 + (numIndices & 1);
                packet += numIndices;
                packet += 0;
            }
            packet.CloseUnpack();

            // constant color of each vertex

            packet.Strow(&ConstantVertColor);
            packet.Stcycl(numVertices, 0);
            Vifs::tMask mask = { 1, 1, 1, 3,
                1, 1, 1, 3,
                1, 1, 1, 3,
                1, 1, 1, 3 };
            packet.Stmask(mask);
            packet.OpenUnpack(Vifs::UnpackModes::v4_32, kTempAreaStart,
                Packet::kDoubleBuff, Packet::kMasked);
            packet.CloseUnpack(numVertices);

            // start renderer

            packet.Mscnt();
            packet.Pad128();
        }
        packet.CloseTag();
    }
}
