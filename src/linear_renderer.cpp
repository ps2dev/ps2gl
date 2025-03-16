/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/cpu_matrix.h"
#include "ps2s/math.h"
#include "ps2s/packet.h"

#include "ps2gl/drawcontext.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/immgmanager.h"
#include "ps2gl/lighting.h"
#include "ps2gl/linear_renderer.h"
#include "ps2gl/material.h"
#include "ps2gl/matrix.h"
#include "ps2gl/metrics.h"
#include "ps2gl/texture.h"

#include "vu1_mem_linear.h"

void CLinearRenderer::DrawLinearArrays(CGeometryBlock& block)
{
    int wordsPerVert   = block.GetWordsPerVertex();
    int wordsPerNormal = (block.GetNormalsAreValid()) ? block.GetWordsPerNormal() : 0;
    int wordsPerTex    = (block.GetTexCoordsAreValid()) ? block.GetWordsPerTexCoord() : 0;
    int wordsPerColor  = (block.GetColorsAreValid()) ? block.GetWordsPerColor() : 0;

    CVifSCDmaPacket& packet = pGLContext->GetVif1Packet();
    InitXferBlock(packet, wordsPerVert, wordsPerNormal, wordsPerTex, wordsPerColor);

    // get max number of vertices per vu1 buffer

    // let's assume separate unpacks for vertices, normals, tex uvs..
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

    // draw

    DrawBlock(packet, block, maxVertsPerBuffer);
}

void CLinearRenderer::InitContext(GLenum primType, uint32_t rcChanges, bool userRcChanged)
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
}

int CLinearRenderer::GetPacketQwordSize(const CGeometryBlock& geometry)
{
    // FIXME:  this is a pitiful hack for allocating enough memory
    return Math::Max(geometry.GetTotalVertices() / 70, 1) * 1000;
}

CRendererProps
CLinearRenderer::GetRenderContextDeps()
{
    CRendererProps deps;
    deps                = (uint64_t)0;
    deps.Lighting       = 1;
    deps.Texture        = 1;
    deps.PerVtxMaterial = 1;

    return deps;
}

bool CLinearRenderer::GetCachePackets(const CGeometryBlock& geometry)
{
    return !(pGLContext->GetImmLighting().GetLightingEnabled()
        && !geometry.GetNormalsAreValid());
}

void CLinearRenderer::DrawBlock(CVifSCDmaPacket& packet,
    CGeometryBlock& block, int maxVertsPerBuffer)
{
    mErrorIf(block.GetWordsPerVertex() == 2, "2 word vertices not supported");

    // interleave the vertices, normals, tex coords, and colors using
    // "skipping write" vif mode on each block of data
    // (clobbered in XferBufferHeader())
    packet.Cnt();
    {
        packet.Stcycl(1, InputQuadsPerVert);
        packet.Pad128();
    }
    packet.CloseTag();

    // return;

    //
    // loop over the strips in this geometry block
    //

    int numVertsToRestart  = block.GetNumVertsToRestartStrip();
    bool stripsCanBeMerged = block.GetStripsCanBeMerged();

    int numVertsXferred   = 0;
    int numStripsInBuffer = 0;
    unsigned short stripOffsets[16];
    bool haveContinued = false;
    const void *normals, *vertices, *texCoords, *colors;
    normals = vertices = texCoords = colors = NULL;
    int vu1BufferOffset = 0, stripIndex = 0, vertsInBlock = 0;
    // keep number of verts transferred in each full buffer in sync with
    // FindNumBuffers() below
    int adjMaxVertsPerBuffer = maxVertsPerBuffer - (Math::IsOdd(maxVertsPerBuffer - numVertsToRestart));
    for (int curStrip = 0; curStrip < block.GetNumStrips(); curStrip++) {

        // find the number of vu1 buffers this strip will take
        int numVertsFirstBuffer, numVertsLastBuffer, numBuffers;
        FindNumBuffers(block.GetStripLength(curStrip),
            numVertsToRestart, numVertsXferred, maxVertsPerBuffer,
            numVertsFirstBuffer, numVertsLastBuffer, numBuffers);

        //
        // loop over the buffers this strip will fill (usually one)
        //

        int numVertsThisBuffer;
        int indexIntoStrip  = 0;
        int vu1QuadsPerVert = InputQuadsPerVert;
        for (int curBuffer = 0;
             curBuffer < numBuffers;
             curBuffer++, indexIntoStrip += numVertsThisBuffer - numVertsToRestart) {

            // how many verts in this vu1 buffer?
            if (curBuffer == 0)
                numVertsThisBuffer = numVertsFirstBuffer;
            else if (curBuffer == numBuffers - 1)
                numVertsThisBuffer = numVertsLastBuffer;
            else
                numVertsThisBuffer = adjMaxVertsPerBuffer;

            // xfer the list of primitives

            if (!haveContinued) {
                vertices        = (block.GetVerticesAreValid()) ? block.GetVertices(curStrip) : NULL;
                normals         = (block.GetNormalsAreValid()) ? block.GetNormals(curStrip) : NULL;
                texCoords       = (block.GetTexCoordsAreValid()) ? block.GetTexCoords(curStrip) : NULL;
                colors          = (block.GetColorsAreValid()) ? block.GetColors(curStrip) : NULL;
                vu1BufferOffset = InputGeomOffset + numVertsXferred * vu1QuadsPerVert;
                stripIndex      = indexIntoStrip;
                vertsInBlock    = 0;
            }

            // we only want to transfer the geometry data if:
            //   - the next strip is not adjacent in memory to this one (indicated by
            //     CGeometryBlock::StripIsContinued) and so cannot be combined into
            //     one transfer
            //   - this strip spills over into the next buffer (so it must be split
            //     into more than one transfer)
            if (!block.StripIsContinued(curStrip)
                || curBuffer < numBuffers - 1) {
                XferBlock(packet,
                    vertices, normals, texCoords, colors,
                    vu1BufferOffset,
                    stripIndex, vertsInBlock + numVertsThisBuffer);
                haveContinued = false;
            } else {
                vertsInBlock += numVertsThisBuffer;
                haveContinued = true;
            }

            stripOffsets[numStripsInBuffer++] = numVertsXferred;
            mErrorIf(numStripsInBuffer > 16, "Too many strips in buffer.. this shouldn't happen");
            numVertsXferred += numVertsThisBuffer;

            // if there are more buffers in this strip, render this buffer
            if (curBuffer < numBuffers - 1) {
                FinishBuffer(packet, numVertsToRestart, numVertsXferred, vu1QuadsPerVert,
                    numStripsInBuffer, stripOffsets);
                numStripsInBuffer = 0;
                numVertsXferred   = 0;
            }

        } // end buffer loop

        // finish this buffer if:
        //   - strips of this prim type cannot be merged into a single buffer/giftag
        //   - there is too little free space left
        //   - we have filled the max number of strips per buffer
        //   - or this is the last strip

        // test against 'numVertsToRestart + 1' because FindNumBuffers() might
        // clip off one vert for backface culling
        // (maybe this should be 'adjMaxVertsPerBuffer'?)
        if (!stripsCanBeMerged
            || ((maxVertsPerBuffer - numVertsXferred) <= numVertsToRestart + 1)
            || numStripsInBuffer == 16
            || (curStrip == block.GetNumStrips() - 1)) {
            if (haveContinued) {
                XferBlock(packet,
                    vertices, normals, texCoords, colors,
                    vu1BufferOffset,
                    stripIndex, vertsInBlock);
                haveContinued = false;
            }

            FinishBuffer(packet, numVertsToRestart, numVertsXferred, vu1QuadsPerVert,
                numStripsInBuffer, stripOffsets);
            numStripsInBuffer = 0;
            numVertsXferred   = 0;
        }

    } // end strip loop
}

void CLinearRenderer::FinishBuffer(CVifSCDmaPacket& packet, int numVertsToBreakStrip,
    int numVertsInBuffer, int vu1QuadsPerVert,
    int numStripsInBuffer, unsigned short* stripOffsets)
{
    packet.Cnt();
    {
        // going to start a new buffer, so finish this one
        // header (giftag, etc..)
        XferBufferHeader(packet, numVertsToBreakStrip,
            numVertsInBuffer,
            numStripsInBuffer, stripOffsets);

        packet.Mscnt();
        packet.Pad128();
    }
    packet.CloseTag();
}

void CLinearRenderer::FindNumBuffers(int numToAdd, int numVertsToRestart,
    int numVertsAlreadyInFirstBuffer, int maxVertsPerBuffer,
    int& numVertsFirstBuffer, int& numVertsLastBuffer,
    int& numBuffers)
{
    // find number of buffers (chunks that will fit into vu1 mem buffers/unpack)

    // deal with the first buffer
    int numLeftToAdd;
    int freeVertsFirstBuffer = maxVertsPerBuffer - numVertsAlreadyInFirstBuffer;
    if (numToAdd <= freeVertsFirstBuffer) {
        numVertsFirstBuffer = numToAdd;
        numLeftToAdd        = 0;
    } else {
        // we only want even numbers of vertices in tri strips that are spilled
        // across buffer boundaries so that the sense of the backfacing calculation
        // remains correct in the next buffer
        numVertsFirstBuffer = freeVertsFirstBuffer - (int)Math::IsOdd(freeVertsFirstBuffer);
        numLeftToAdd        = numToAdd - (numVertsFirstBuffer - numVertsToRestart);
    }
    // if this doesn't make sense, try drawing it..
    int adjVertsPerBuffer = maxVertsPerBuffer - numVertsToRestart;
    adjVertsPerBuffer -= (int)Math::IsOdd(adjVertsPerBuffer);
    numBuffers = 1 + numLeftToAdd / adjVertsPerBuffer; // 1 is first buffer
    if (numLeftToAdd % adjVertsPerBuffer > numVertsToRestart)
        numBuffers++;

    numVertsLastBuffer = (numBuffers > 1)
        ? numToAdd - ((numVertsFirstBuffer - numVertsToRestart)
                         + (numBuffers - 2) * adjVertsPerBuffer)
        : numToAdd;
}

void CLinearRenderer::XferBufferHeader(CVifSCDmaPacket& packet,
    int numVertsToBreakStrip,
    int numVerts,
    int numStripsInBuffer, unsigned short* stripOffsets)
{
    int vu1OutQuadsPerVert = OutputQuadsPerVert;

    // xfer header info for strip
    packet.Stcycl(4, 4);
    packet.OpenUnpack(Vifs::UnpackModes::v4_32, 0, Packet::kDoubleBuff);
    {
        // num vertices
        packet += numVerts;
        packet += 0;
        packet += (uint64_t)0;

        // adc bits for the beginning of strips.. 16 24-bit values of following format:
        //   bits 0-9	: offset into output geometry (not including giftag)
        //   bit 10	: stop bit
        //   bit 11	: ADC bit of second vertex starting at offset
        // these are converted to floats to do a right shift with a fp add on vu1
        unsigned int adcBits = 0;
        if (numVertsToBreakStrip == 0)
            numStripsInBuffer = 0;
        else if (numVertsToBreakStrip == 2)
            adcBits = 0x800;

        float adc;
        unsigned int stopBit = 0x400;
        for (int i = 0; i < 16; i++) {
            if (i < numStripsInBuffer)
                adc = (float)(adcBits | (unsigned int)stripOffsets[i] * vu1OutQuadsPerVert);
            else if (i == numStripsInBuffer)
                adc = (float)stopBit;
            else
                adc = (float)0;
            packet += adc;
        }
    }
    packet.CloseUnpack();

    // restore the write cycle
    packet.Stcycl(1, InputQuadsPerVert);
}
