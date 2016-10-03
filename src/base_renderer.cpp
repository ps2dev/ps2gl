/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/packet.h"
#include "ps2s/math.h"
#include "ps2s/cpu_matrix.h"

#include <stdlib.h>

#include "ps2gl/base_renderer.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/metrics.h"
#include "ps2gl/immgmanager.h"
#include "ps2gl/lighting.h"
#include "ps2gl/material.h"
#include "ps2gl/matrix.h"
#include "ps2gl/texture.h"
#include "ps2gl/drawcontext.h"

#include "vu1_context.h"

void
CBaseRenderer::GetUnpackAttribs( int numWords, unsigned int &mode, Vifs::tMask &mask )
{

   if ( numWords == 3 ) {
      Vifs::tMask vec3Mask = { 0, 0, 0, 1,
			       0, 0, 0, 1,
			       0, 0, 0, 1,
			       0, 0, 0, 1 };
      mode = Vifs::UnpackModes::v3_32;
      mask = vec3Mask;
   }
   else if ( numWords == 4 ) {
      Vifs::tMask vec4Mask = { 0, 0, 0, 0,
			       0, 0, 0, 0,
			       0, 0, 0, 0,
			       0, 0, 0, 0 };
      mode = Vifs::UnpackModes::v4_32;
      mask = vec4Mask;
   }
   else if ( numWords == 2 ) {
      Vifs::tMask vec2Mask = { 0, 0, 1, 1,
			       0, 0, 1, 1,
			       0, 0, 1, 1,
			       0, 0, 1, 1 };
      mode = Vifs::UnpackModes::v2_32;
      mask = vec2Mask;
   }
   else {
      mError("shouldn't get here (you're probably calling glDrawArrays"
	 "without setting one of the pointers)");
   }
}

/**
 * Caches some data frequently used by XferBlock(), sets up row register.
 * The parameters wordsPerNormal, wordsPerTex, and wordsPerColor should be
 * zero if the application has not given normals, texture coords, or colors.
 */
void
CBaseRenderer::InitXferBlock( CVifSCDmaPacket &packet,
			      int wordsPerVertex, int wordsPerNormal,
			      int wordsPerTex, int wordsPerColor )
{
   CImmGeomManager &gmanager = pGLContext->GetImmGeomManager();

   NormalBuf = & gmanager.GetNormalBuf();
   TexCoordBuf = & gmanager.GetTexCoordBuf();

   CurNormal = gmanager.GetCurNormal();
   const float* texCoord = gmanager.GetCurTexCoord();
   CurTexCoord[0] = texCoord[0];
   CurTexCoord[1] = texCoord[1];

   // get unpack modes/masks

   WordsPerVertex = wordsPerVertex;
   GetUnpackAttribs( WordsPerVertex, VertexUnpackMode, VertexUnpackMask );

   WordsPerNormal = (wordsPerNormal > 0) ? wordsPerNormal : 3;
   GetUnpackAttribs( WordsPerNormal, NormalUnpackMode, NormalUnpackMask );

   WordsPerTexCoord = (wordsPerTex > 0) ? wordsPerTex : 2;
   GetUnpackAttribs( WordsPerTexCoord, TexCoordUnpackMode, TexCoordUnpackMask );

   WordsPerColor = (wordsPerColor > 0) ? wordsPerColor : 3;
   GetUnpackAttribs( WordsPerColor, ColorUnpackMode, ColorUnpackMask );

   // set up the row register to expand vectors with fewer than 4 elements

   packet.Cnt();
   {
     // w is 256 to remind me that this is not used as the vertex w but
     // is necessary to clear any adc bits set for strips, otherwise they
     // accumulate..
      static const float row[4] = { 0.0f, 0.0f, 1.0f, 256.0f };
      packet.Strow( row );

      packet.Pad128();
   }
   packet.CloseTag();
}


/**
 * Transfers a block of geometry to vu0/vu1 using <i>packet</i>, where
 * "geometry" means vertices and zero or more normals,
 * texture coordinates, and colors.
 * <b>Note that you MUST set the vif1 write mode correctly before calling
 * XferBlock!!</b> (e.g., Stcycl(1, vu1QuadsPerVert))
 * normals, texCoords, and colors should be NULL if not provided.
 * @param vu1Offset offset into vu1 memory in quadwords
 * @param firstElement the starting "offset" into the vertex, normal, etc.
 *  arrays (for example: this would be "2" to start draw from the 3rd element)
 */
void
CBaseRenderer::XferBlock( CVifSCDmaPacket &packet,
			  const void *vertices, const void *normals,
			  const void *texCoords, const void *colors,
			  int vu1Offset, int firstElement, int numToAdd )
{
   //
   // vertices
   //

   if ( XferVertices ) {
      mErrorIf( vertices == NULL, "Tried to render an array with no vertices!" );
      XferVectors( packet, (unsigned int *)vertices,
		   firstElement, numToAdd,
		   WordsPerVertex, VertexUnpackMask, VertexUnpackMode,
		   vu1Offset );
   }

   //
   // normals
   //

   int firstNormal = firstElement;
   if ( XferNormals && normals == NULL ) {
      // no normals given, so use the current normal..
      // I hate to actually write every normal into the packet,
      // but I can't use the vif to expand the data because I
      // need it to interleave the vertices, normals, etc..
      CDmaPacket &normalBuf = *NormalBuf;
      normals = (void*)normalBuf.GetNextPtr();
      firstNormal = 0;

      for ( int i = 0; i < numToAdd; i++ )
	 normalBuf += CurNormal;
   }

   if ( XferNormals )
      XferVectors( packet, (unsigned int*)normals,
		   firstNormal, numToAdd,
		   WordsPerNormal, NormalUnpackMask, NormalUnpackMode,
		   vu1Offset + 1 );

   //
   // tex coords
   //

   int firstTexCoord = firstElement;
   if ( XferTexCoords && texCoords == NULL ) {
      // no tex coords given, so use the current value..
      // see note above for normals
      CDmaPacket &texCoordBuf = *TexCoordBuf;
      texCoords = (void*)texCoordBuf.GetNextPtr();
      firstTexCoord = 0;

      for ( int i = 0; i < numToAdd; i++ ) {
	 texCoordBuf += CurTexCoord[0];
	 texCoordBuf += CurTexCoord[1];
      }
   }
   if ( XferTexCoords )
      XferVectors( packet, (unsigned int*)texCoords,
		   firstTexCoord, numToAdd,
		   WordsPerTexCoord, TexCoordUnpackMask, TexCoordUnpackMode,
		   vu1Offset + 2 );

   //
   // colors
   //

   int firstColor = firstElement;
   if ( colors != NULL && XferColors ) {
      XferVectors( packet, (unsigned int*)colors,
		   firstColor, numToAdd,
		   WordsPerColor, ColorUnpackMask, ColorUnpackMode,
		   vu1Offset + 3 );
   }

}

#define kContextStart 0 // for the kLightBase stuff below

void
CBaseRenderer::AddVu1RendererContext( CVifSCDmaPacket &packet, GLenum primType, int vu1Offset )
{
   CGLContext &glContext = *pGLContext;

   packet.Stcycl(1,1);
   packet.Flush();
   packet.Pad96();
   packet.OpenUnpack( Vifs::UnpackModes::v4_32, vu1Offset, Packet::kSingleBuff );
   {
      // find light pointers
      CImmLighting &lighting = glContext.GetImmLighting();
      tLightPtrs lightPtrs[8];
      tLightPtrs *nextDir, *nextPt, *nextSpot;
      nextDir = nextPt = nextSpot = &lightPtrs[0];
      int numDirs, numPts, numSpots;
      numDirs = numPts = numSpots = 0;
      for ( int i = 0; i < 8; i++ ) {
	 CImmLight& light = lighting.GetImmLight(i);
	 if ( light.IsEnabled() ) {
	    int lightBase = kLight0Base + vu1Offset;
	    if ( light.IsDirectional() ) {
	       nextDir->dir = lightBase + i * kLightStructSize;
	       nextDir++;
	       numDirs++;
	    }
	    else if ( light.IsPoint() ) {
	       nextPt->point = lightBase + i * kLightStructSize;
	       nextPt++;
	       numPts++;
	    }
	    else if ( light.IsSpot() ) {
	       nextSpot->spot = lightBase + i * kLightStructSize;
	       nextSpot++;
	       numSpots++;
	    }
	 }
      }	 

      bool doLighting = glContext.GetImmLighting().GetLightingEnabled();

      // transpose of object to world space xfrm (for light directions)
      cpu_mat_44 objToWorldXfrmTrans = glContext.GetModelViewStack().GetTop();
      // clear any translations.. should be doing a 3x3 transpose..
      objToWorldXfrmTrans.set_col3( cpu_vec_xyzw(0, 0, 0, 1) );
      objToWorldXfrmTrans = objToWorldXfrmTrans.transpose();
      // do we need to rescale normals?
      cpu_mat_44 normalRescale;
      normalRescale.set_identity();
      float normalScale = 1.0f;
      CImmDrawContext &drawContext = glContext.GetImmDrawContext();
      if ( drawContext.GetRescaleNormals() ) {
	 cpu_vec_xyzw fake_normal( 1, 0, 0, 0 );
	 fake_normal = objToWorldXfrmTrans * fake_normal;
	 normalScale = 1.0f / fake_normal.length();
	 normalRescale.set_scale( cpu_vec_xyz(normalScale, normalScale, normalScale) );
      }
      objToWorldXfrmTrans = normalRescale * objToWorldXfrmTrans;

      // num lights
      if ( doLighting ) {
	 packet += numDirs;
	 packet += numPts;
	 packet += numSpots;
      }
      else {
	 packet += (tU64)0;
	 packet += 0;
      }

      // backface culling multiplier -- this is 1.0f or -1.0f, the 6th bit
      // also turns on/off culling
      float bfc_mult = (float)drawContext.GetCullFaceDir();
      unsigned int bfc_word;
      asm( " ## nop ## " : "=r" (bfc_word) : "0" (bfc_mult) );
      bool do_culling = drawContext.GetDoCullFace() && (primType > GL_LINE_STRIP);
      packet += bfc_word | (unsigned int)do_culling << 5;

      // light pointers
      packet.Add( &lightPtrs[0], 8 );
	 
      float maxColorValue = GetMaxColorValue( glContext.GetTexManager().GetTexEnabled() );

      // add light info
      for ( int i = 0; i < 8; i++ ) {
	 CImmLight& light = lighting.GetImmLight(i);
	 packet += light.GetAmbient() * maxColorValue;
	 packet += light.GetDiffuse() * maxColorValue;
	 packet += light.GetSpecular() * maxColorValue;

	 if ( light.IsDirectional() )
	    packet += light.GetPosition();
	 else {
	    packet += light.GetPosition();
	 }

	 packet += light.GetSpotDir();

	 // attenuation coeffs for positional light sources
	 // because we're doing lighting calculations in object space,
	 // we need to adjust the attenuation of positional light sources
	 // and all lighting directions to take into account scaling
	 packet += light.GetConstantAtten();
	 packet += light.GetLinearAtten() * 1.0f/normalScale;
	 packet += light.GetQuadAtten() * 1.0f/normalScale;
	 packet += 0; // padding

      }

      // global ambient
      cpu_vec_4 globalAmb;
      if ( doLighting )
	 globalAmb = lighting.GetGlobalAmbient() * maxColorValue;
      else
	 globalAmb = cpu_vec_4(0, 0, 0, 0);
      packet.Add( (tU32*)&globalAmb, 3 );

      // stick in the offset to convert clip space depth value to GS
      float depthClipToGs = (float)((1 << drawContext.GetDepthBits()) - 1)/2.0f;
      packet += depthClipToGs;

      // cur material

      CImmMaterial& material = glContext.GetMaterialManager().GetImmMaterial();

      // add emissive component
      cpu_vec_4 emission;
      if ( doLighting )
	 emission = material.GetEmission() * maxColorValue;
      else
	 emission = glContext.GetMaterialManager().GetCurColor() * maxColorValue;
      packet += emission;

      // ambient
      packet += material.GetAmbient();

      // diffuse
      cpu_vec_4 matDiffuse = material.GetDiffuse();
      // the alpha value is set to the alpha of the diffuse in the renderers;
      // this should be the current color alpha if lighting is disabled
      if ( ! doLighting )
	 matDiffuse[3] = glContext.GetMaterialManager().GetCurColor()[3];
      packet += matDiffuse;

      // specular
      packet += material.GetSpecular();

      // vertex xform
      packet += drawContext.GetVertexXform();

      // fixed vertToEye vector for non-local specular
      cpu_vec_xyzw vertToEye( 0.0f, 0.0f, 1.0f, 0.0f );
      packet += objToWorldXfrmTrans * vertToEye;

      // transpose of object to world space transform
      packet += objToWorldXfrmTrans;

      // world to object space xfrm (for light positions)
      cpu_mat_44 worldToObjXfrm = glContext.GetModelViewStack().GetInvTop();
      packet += worldToObjXfrm;

      // giftag - this is down at the bottom to make sure that when switching
      // primitives the last buffer will have a chance to copy the giftag before
      // it is overwritten with the new one
      GLenum newPrimType = drawContext.GetPolygonMode();
      if (newPrimType == GL_FILL) newPrimType = primType;
      newPrimType &= 0xff;
      tGifTag giftag = BuildGiftag( newPrimType );
      packet += giftag;

      // add info used by clipping code
      // first the dimensions of the framebuffer
      float xClip = (float)2048.0f/(drawContext.GetFBWidth() * 0.5f * 2.0f);
      packet += Math::Max( xClip, 1.0f );
      float yClip = (float)2048.0f/(drawContext.GetFBHeight() * 0.5f * 2.0f);
      packet += Math::Max( yClip, 1.0f );
      float depthClip = 2048.0f / depthClipToGs;
      // FIXME: maybe these 2048's should be 2047.5s...
      depthClip *= 1.003f; // round up a bit for fp error (????)
      packet += depthClip;
      // enable/disable clipping
      packet += (drawContext.GetDoClipping()) ? 1 : 0;
   }
   packet.CloseUnpack();
}

tGifTag
CBaseRenderer::BuildGiftag( GLenum primType )
{
   CGLContext &glContext = *pGLContext;

   primType &= 0x7;  // convert from GL #define to gs prim number
   CImmDrawContext &drawContext = glContext.GetImmDrawContext();
   bool smoothShading = drawContext.GetDoSmoothShading();
   bool useTexture = glContext.GetTexManager().GetTexEnabled();
   bool alpha = drawContext.GetBlendEnabled();
   unsigned int nreg = OutputQuadsPerVert;

   GS::tPrim prim = { PRIM: primType, IIP: smoothShading, TME: useTexture,
		      FGE: 0, ABE: alpha, AA1: 0, FST: 0, CTXT: 0, FIX: 0 };
   tGifTag giftag = { NLOOP: 0, EOP: 1, pad0: 0, id: 0, PRE: 1,
			PRIM: *(tU64*)&prim, FLG: 0, NREG: nreg, REGS0: 2, REGS1: 1,
			REGS2: 4 };
   return giftag;
}

void
CBaseRenderer::CacheRendererState()
{
   XferNormals = pGLContext->GetImmLighting().GetLightingEnabled();
   XferTexCoords = pGLContext->GetTexManager().GetTexEnabled();
   XferColors = pGLContext->GetMaterialManager().GetColorMaterialEnabled();
}

void
CBaseRenderer::Load()
{
   mErrorIf( *(unsigned short*)MicrocodePacket > 1024,
	     "This vu1 renderer won't fit into vu1 code memory: 0x%08x",
	     (unsigned int)Capabilities );
   CVifSCDmaPacket &packet = pGLContext->GetVif1Packet();

   packet.Call( MicrocodePacket );
   packet.Pad128();
   packet.CloseTag();

   pglAddToMetric(kMetricsRendererUpload);
}

void
CBaseRenderer::XferVectors( CVifSCDmaPacket &packet, unsigned int *dataStart,
			 int startOffset, int numVectors, int wordsPerVec,
			 Vifs::tMask unpackMask, tU32 unpackMode,
			 int vu1MemOffset )
{
   // find number of words to prepend with a cnt

   unsigned int *vecDataStart = dataStart + startOffset * wordsPerVec;
   unsigned int *vecDataEnd = vecDataStart + numVectors * wordsPerVec;

   mAssert( numVectors > 0 );
   mErrorIf( (unsigned int)vecDataStart & 4-1,
	     "XferVectors only works with word-aligned data" );

   int numWordsToPrepend = 0;
   unsigned int *refXferStart = vecDataStart;
   while ( (unsigned int)refXferStart & (16-1) ) {
      numWordsToPrepend++;
      refXferStart++;
      if ( refXferStart == vecDataEnd ) break;
   }
   int numWordsToAppend = 0;
   unsigned int *refXferEnd = vecDataEnd;
   while ( ((unsigned int)refXferEnd & (16-1)) && refXferEnd > refXferStart ) {
      numWordsToAppend++;
      refXferEnd--;
   }
   int numQuadsInRefXfer = ((unsigned int)refXferEnd - (unsigned int)refXferStart) / 16;

   packet.Cnt();
   {
      // set mask to expand vectors appropriately
      packet.Stmask( unpackMask );

      // prepend
      if ( numWordsToPrepend > 1 ) {
	 // either 2 or 3 words to prepend
	 packet.Nop().Nop();
	 if ( numWordsToPrepend == 2 )
	    packet.Nop();
	 
	 packet.OpenUnpack( unpackMode,
			    vu1MemOffset,
			    VifDoubleBuffered,
			    Packet::kMasked );
	 packet.CloseUnpack( numVectors );

	 if ( numWordsToPrepend == 3 )
	    packet += *vecDataStart;
      }

      packet.Pad128();
   }
   packet.CloseTag();

   // xfer qword block of vectors
   packet.Ref( Core::MakePtrNormal(refXferStart), numQuadsInRefXfer );
   {
      // either 0 words to prepend or 1 word left to prepend
      if ( numWordsToPrepend == 0 )
	 packet.Nop();
      if ( numWordsToPrepend <= 1 ) {
	 packet.OpenUnpack( unpackMode,
			    vu1MemOffset,
			    VifDoubleBuffered,
			    Packet::kMasked );
	 packet.CloseUnpack( numVectors );
      }
      if ( numWordsToPrepend == 1 )
	 packet += *vecDataStart;
      else if ( numWordsToPrepend == 2 )
	 packet.Add( vecDataStart, 2 );
      else if ( numWordsToPrepend == 3 )
	 packet.Add( &vecDataStart[1], 2 );
   }

   // xfer any remaining vectors
   if ( numWordsToAppend > 0 ) {
      packet.Cnt();
      {
	 packet.Add( refXferEnd, numWordsToAppend );
	 packet.Pad128();
      }
      packet.CloseTag();
   }
}

