/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/packet.h"

#include "ps2gl/renderermanager.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/metrics.h"

#include "ps2gl/linear_renderer.h"
#include "ps2gl/indexed_renderer.h"

#include "vu1renderers.h"
#include "vu1_mem_linear.h"

using namespace RendererProps;

/********************************************
 * methods
 */

CRendererManager::CRendererManager( CGLContext &context)
   : GLContext(context),
     RendererReqsHaveChanged(false),
     CurUserPrimReqs(0), CurUserPrimReqMask(~0),
     NumDefaultRenderers(0), NumUserRenderers(0),
     CurrentRenderer(NULL), NewRenderer(NULL)
{
   RendererRequirements.PrimType	= 0;
   RendererRequirements.Lighting	= 0;
   RendererRequirements.NumDirLights	= 0;
   RendererRequirements.NumPtLights	= 0;
   RendererRequirements.Texture		= 0;
   RendererRequirements.Specular	= 0;
   RendererRequirements.PerVtxMaterial 	= kNoMaterial;
   RendererRequirements.Clipping	= kClipped;
   RendererRequirements.CullFace	= 0;
   RendererRequirements.TwoSidedLighting = 0;
   RendererRequirements.ArrayAccess	= 0;
   RendererRequirements.UserProps	= 0;

   CRendererProps no_reqs;
   no_reqs = (tU64)0;

   // indexed array renderer
   {
      CRendererProps capabilities =
      {
	 PrimType:	kPtsLinesStripsFans,
	 Lighting:	1,
	 NumDirLights:	k3DirLights | k8DirLights,
	 NumPtLights:	k1PtLight | k2PtLights | k8PtLights,
	 Texture:	1,
	 Specular:	1,
	 PerVtxMaterial: kNoMaterial,
	 Clipping:	kNonClipped | kClipped,
	 CullFace:	1,
	 TwoSidedLighting: 0,
	 ArrayAccess: kIndexed
      };

      RegisterDefaultRenderer( new CIndexedRenderer(mVsmAddr(Indexed), mVsmSize(Indexed), capabilities, no_reqs, 3, 3,
						    "indexed") );
   }

   // fast, no lights renderer
   {
      CRendererProps capabilities =
      {
	 PrimType:	kPtsLinesStripsFans,
	 Lighting:	0,
	 NumDirLights:	k3DirLights,
	 NumPtLights:	0,
	 Texture:	1,
	 Specular:	0,
	 PerVtxMaterial: kNoMaterial,
	 Clipping:	kNonClipped,
	 CullFace:	0,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(FastNoLights), mVsmSize(FastNoLights), capabilities, no_reqs, 3, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "fast, no lights") );
   }
   // fast renderer
   {
      CRendererProps capabilities =
      {
	 PrimType:	kPtsLinesStripsFans,
	 Lighting:	1,
	 NumDirLights:	k3DirLights,
	 NumPtLights:	0,
	 Texture:	1,
	 Specular:	0,
	 PerVtxMaterial: kNoMaterial,
	 Clipping:	kNonClipped,
	 CullFace:	0,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(Fast), mVsmSize(Fast), capabilities, no_reqs, 3, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "fast") );
   }

   // SCEI renderer
   {
      CRendererProps capabilities =
      {
	 PrimType:	kPtsLinesStripsFans,
	 Lighting:	1,
	 NumDirLights:	k3DirLights,
	 NumPtLights:	0,
	 Texture:	1,
	 Specular:	0,
	 PerVtxMaterial: kNoMaterial,
	 Clipping:	kClipped,
	 CullFace:	0,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(SCEI), mVsmSize(SCEI), capabilities, no_reqs, 3, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "scei") );
   }

   // linear, no specular renderers

   {
      CRendererProps capabilities =
      {
	 PrimType:	kPtsLinesStripsFans,
	 Lighting:	1,
	 NumDirLights:	k3DirLights | k8DirLights,
	 NumPtLights:	k1PtLight | k2PtLights | k8PtLights,
	 Texture:	1,
	 Specular:	0,
	 PerVtxMaterial: kNoMaterial,
	 Clipping:	kNonClipped | kClipped,
	 CullFace:	1,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(GeneralNoSpec), mVsmSize(GeneralNoSpec), capabilities, no_reqs, 3, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "linear, no specular") );
   }
   {
      CRendererProps capabilities =
      {
	 PrimType:	kTriangles,
	 Lighting:	1,
	 NumDirLights:	k3DirLights | k8DirLights,
	 NumPtLights:	k1PtLight | k2PtLights | k8PtLights,
	 Texture:	1,
	 Specular:	0,
	 PerVtxMaterial: kNoMaterial,
	 Clipping:	kNonClipped | kClipped,
	 CullFace:	1,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(GeneralNoSpecTri), mVsmSize(GeneralNoSpecTri), capabilities, no_reqs, 3, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "linear, tris, no specular") );
   }
   {
      CRendererProps capabilities =
      {
	 PrimType:	kQuads,
	 Lighting:	1,
	 NumDirLights:	k3DirLights | k8DirLights,
	 NumPtLights:	k1PtLight | k2PtLights | k8PtLights,
	 Texture:	1,
	 Specular:	0,
	 PerVtxMaterial: kNoMaterial,
	 Clipping:	kNonClipped | kClipped,
	 CullFace:	1,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(GeneralNoSpecQuad), mVsmSize(GeneralNoSpecQuad), capabilities, no_reqs, 3, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "linear, quads, no specular") );
   }

   // linear renderers

   {
      CRendererProps capabilities =
      {
	 PrimType:	kPtsLinesStripsFans,
	 Lighting:	1,
	 NumDirLights:	k3DirLights | k8DirLights,
	 NumPtLights:	k1PtLight | k2PtLights | k8PtLights,
	 Texture:	1,
	 Specular:	1,
	 PerVtxMaterial: kNoMaterial,
	 Clipping:	kNonClipped | kClipped,
	 CullFace:	1,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(General), mVsmSize(General), capabilities, no_reqs, 3, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "linear") );
   }
   {
      CRendererProps capabilities =
      {
	 PrimType:	kQuads,
	 Lighting:	1,
	 NumDirLights:	k3DirLights | k8DirLights,
	 NumPtLights:	k1PtLight | k2PtLights | k8PtLights,
	 Texture:	1,
	 Specular:	1,
	 PerVtxMaterial: kNoMaterial,
	 Clipping:	kNonClipped | kClipped,
	 CullFace:	1,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(GeneralQuad), mVsmSize(GeneralQuad), capabilities, no_reqs, 3, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "linear, quads") );
   }
   {
      CRendererProps capabilities =
      {
	 PrimType:	kTriangles,
	 Lighting:	1,
	 NumDirLights:	k3DirLights | k8DirLights,
	 NumPtLights:	k1PtLight | k2PtLights | k8PtLights,
	 Texture:	1,
	 Specular:	1,
	 PerVtxMaterial: kNoMaterial,
	 Clipping:	kNonClipped | kClipped,
	 CullFace:	1,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(GeneralTri), mVsmSize(GeneralTri), capabilities, no_reqs, 3, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "linear, tris") );
   }

#if 0
/*
 * Compiler warnings:
 *   dvp-as -o vu1/general_pv_diff_quad.vo vu1/general_pv_diff_quad_vcl.vsm
 *   vu1/general_pv_diff_quad_vcl.vsm: Assembler messages:
 *   vu1/general_pv_diff_quad_vcl.vsm:1216: Warning: operand out of range (-1129 not between -1024 and 1023)
 *
 *   dvp-as -o vu1/general_pv_diff_tri.vo vu1/general_pv_diff_tri_vcl.vsm
 *   vu1/general_pv_diff_tri_vcl.vsm: Assembler messages:
 *   vu1/general_pv_diff_tri_vcl.vsm:1187: Warning: operand out of range (-1100 not between -1024 and 1023)
 *
 *   dvp-as -o vu1/general_pv_diff.vo vu1/general_pv_diff_vcl.vsm
 *   vu1/general_pv_diff_vcl.vsm: Assembler messages:
 *   vu1/general_pv_diff_vcl.vsm:1202: Warning: operand out of range (-1107 not between -1024 and 1023)
 */

   // linear, per-vertex color renderers

   {
      CRendererProps capabilities =
      {
	 PrimType:	kPtsLinesStripsFans,
	 Lighting:	1,
	 NumDirLights:	k3DirLights | k8DirLights,
	 NumPtLights:	k1PtLight | k2PtLights | k8PtLights,
	 Texture:	1,
	 Specular:	1,
	 PerVtxMaterial: kDiffuse,
	 Clipping:	kNonClipped | kClipped,
	 CullFace:	1,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(GeneralPVDiff), mVsmSize(GeneralPVDiff), capabilities, no_reqs, 4, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "linear, pvc") );
   }
   {
      CRendererProps capabilities =
      {
	 PrimType:	kTriangles,
	 Lighting:	1,
	 NumDirLights:	k3DirLights | k8DirLights,
	 NumPtLights:	k1PtLight | k2PtLights | k8PtLights,
	 Texture:	1,
	 Specular:	1,
	 PerVtxMaterial: kDiffuse,
	 Clipping:	kNonClipped | kClipped,
	 CullFace:	1,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(GeneralPVDiffTri), mVsmSize(GeneralPVDiffTri), capabilities, no_reqs, 4, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "linear, pvc, tris") );
   }
   {
      CRendererProps capabilities =
      {
	 PrimType:	kQuads,
	 Lighting:	1,
	 NumDirLights:	k3DirLights | k8DirLights,
	 NumPtLights:	k1PtLight | k2PtLights | k8PtLights,
	 Texture:	1,
	 Specular:	1,
	 PerVtxMaterial: kDiffuse,
	 Clipping:	kNonClipped | kClipped,
	 CullFace:	1,
	 TwoSidedLighting: 0,
	 ArrayAccess: kLinear
      };

      RegisterDefaultRenderer( new CLinearRenderer(mVsmAddr(GeneralPVDiffQuad), mVsmSize(GeneralPVDiffQuad), capabilities, no_reqs, 4, 3,
						   kInputStart, kInputBufSize - kInputStart,
						   "linear, pvc, quads") );
   }
#endif

   // if we don't do this pglGetCurRendererName() will crash if called before rendering
   // any geometry
   CurrentRenderer = &DefaultRenderers[0];
}

void
CRendererManager::RegisterDefaultRenderer( CRenderer *renderer )
{
   mErrorIf( NumDefaultRenderers == kMaxDefaultRenderers,
	     "Trying to register too many renderers; adjust the limit" );

   tRenderer newEntry = { renderer->GetCapabilities(), renderer->GetRequirements(), renderer };
   DefaultRenderers[NumDefaultRenderers++] = newEntry;
}

void
CRendererManager::RegisterUserRenderer( CRenderer *renderer )
{
   mErrorIf( renderer == NULL,
	     "Trying to register a null renderer is not playing fair..." );

   mErrorIf( NumUserRenderers == kMaxUserRenderers,
	     "Trying to register too many renderers; adjust the limit" );

   tRenderer newEntry = { renderer->GetCapabilities(), renderer->GetRequirements(), renderer };
   UserRenderers[NumUserRenderers++] = newEntry;
}

// state updates

void
CRendererManager::EnableCustom( tU64 flag )
{
   tU64 newState = RendererRequirements;
   newState |= flag;

   if ( newState != (tU64)RendererRequirements )
      RendererReqsHaveChanged = true;

   RendererRequirements = newState;
}

void
CRendererManager::DisableCustom( tU64 flag )
{
   tU64 newState = RendererRequirements;
   newState &= ~flag;

   if ( newState != (tU64)RendererRequirements )
      RendererReqsHaveChanged = true;

   RendererRequirements = newState;
}

void
CRendererManager::NumLightsChanged( tLightType type, int num )
{
   CRendererProps newState = RendererRequirements;

   switch (type) {
      case kDirectional:
	 if ( num == 0 )
	    newState.NumDirLights = 0;
	 else if (0 < num && num <= 3)
	    newState.NumDirLights = k3DirLights; // in namespace??
	 else
	    newState.NumDirLights = k8DirLights;

	 if ( RendererRequirements != newState ) {
	    RendererRequirements = newState;
	    RendererReqsHaveChanged = true;
	 }
	 break;

      case kPoint:
	 if (num == 0)
	    newState.NumPtLights = 0;
	 else if (num == 1)
	    newState.NumPtLights = k1PtLight;
	 else if (num == 2)
	    newState.NumPtLights = k2PtLights;
	 else
	    newState.NumPtLights = k8PtLights;

	 if ( RendererRequirements != newState ) {
	    RendererRequirements = newState;
	    RendererReqsHaveChanged = true;
	 }
	 break;

      case kSpot:
	 break;
   }
}

void
CRendererManager::PrimChanged( unsigned int prim )
{
   // is this a user-defined prim type?
   if ( CGeomManager::IsUserPrimType(prim) ) {
      // user-defined

      tU64 newState = RendererRequirements;

      // clear the current user prim flags (if any)
      newState &= ~CurUserPrimReqs;

      // clear the user-prim-type flag (bit 31)
      prim &= 0x7fffffff;

      CurUserPrimReqs = CGeomManager::GetUserPrimRequirements(prim);
      newState |= CurUserPrimReqs;

      CurUserPrimReqMask = CGeomManager::GetUserPrimReqMask(prim);

      if ( (tU64)RendererRequirements != newState ) {
	 RendererRequirements = newState;
	 RendererReqsHaveChanged = true;
      }
   }
   else {
      // normal prim type

      CRendererProps newState = RendererRequirements;

      if ( CurUserPrimReqs ) {
	 RendererReqsHaveChanged = true;
	 // clear any requirements set by a user prim type
	 newState = (tU64)newState & ~CurUserPrimReqs;
	 CurUserPrimReqs = 0;
      }

      CurUserPrimReqMask = ~(tU32)0;

      if (prim <= GL_LINE_STRIP)
	 newState.PrimType = kPtsLinesStripsFans;
      else if (prim == GL_TRIANGLE_STRIP
	       || prim == GL_TRIANGLE_FAN
	       || prim == GL_POLYGON)
	 newState.PrimType = kPtsLinesStripsFans;
      else if (prim == GL_QUAD_STRIP)
	 newState.PrimType = kPtsLinesStripsFans;
      else if (prim == GL_TRIANGLES)
	 newState.PrimType = kTriangles;
      else if (prim == GL_QUADS)
	 newState.PrimType = kQuads;
      else {
	 mError("shouldn't get here");
      }

      if ( RendererRequirements != newState ) {
	 RendererRequirements = newState;
	 RendererReqsHaveChanged = true;
      }
   }
}

void
CRendererManager::TexEnabledChanged( bool enabled )
{
   CRendererProps newState = RendererRequirements;

   if ( enabled ) newState.Texture = 1;
   else newState.Texture = 0;

   if ( RendererRequirements != newState ) {
      RendererRequirements = newState;
      RendererReqsHaveChanged = true;
   }
}

void
CRendererManager::LightingEnabledChanged( bool enabled )
{
   CRendererProps newState = RendererRequirements;

   if (enabled) newState.Lighting = 1;
   else newState.Lighting = 0;

   if (RendererRequirements != newState) {
      RendererRequirements = newState;
      RendererReqsHaveChanged = true;
   }
}

void
CRendererManager::SpecularEnabledChanged( bool enabled )
{
   CRendererProps newState = RendererRequirements;

   if (enabled) newState.Specular = 1;
   else newState.Specular = 0;

   if (RendererRequirements != newState) {
      RendererRequirements = newState;
      RendererReqsHaveChanged = true;
   }
}

void
CRendererManager::PerVtxMaterialChanged( RendererProps::tPerVtxMaterial matType )
{
   if ( RendererRequirements.PerVtxMaterial != (unsigned int)matType ) {
      RendererRequirements.PerVtxMaterial = matType;
      RendererReqsHaveChanged = true;
   }
}

void
CRendererManager::ClippingEnabledChanged( bool enabled )
{
   tClipping clipping = (enabled) ? kClipped : kNonClipped;
   if ( RendererRequirements.Clipping != (unsigned int)clipping ) {
      RendererRequirements.Clipping = clipping;
      RendererReqsHaveChanged = true;
   }
}

void
CRendererManager::CullFaceEnabledChanged( bool enabled )
{
   if ( RendererRequirements.CullFace != enabled ) {
      RendererRequirements.CullFace = enabled;
      RendererReqsHaveChanged = true;
   }
}

void
CRendererManager::ArrayAccessChanged( RendererProps::tArrayAccess accessType )
{
   if ( RendererRequirements.ArrayAccess != (unsigned int)accessType ) {
      RendererRequirements.ArrayAccess = accessType;
      RendererReqsHaveChanged = true;
   }
}

/**
 * Finds a new renderer if the renderer requirements have changed, but does
 * <b>not</b> replace the current renderer.  To begin using the new renderer
 * call MakeNewRendererCurrent().
 * @return true if the renderer changed, false otherwise
 */
bool
CRendererManager::UpdateNewRenderer()
{
   bool rendererChanged = false;

   if ( RendererReqsHaveChanged ) {
      // do a little fixin' up..
      CRendererProps rreqs = RendererRequirements;
      if ( ! rreqs.Lighting ) {
	 // don't care about these if there's no light
	 rreqs.Specular = 0;
	 rreqs.TwoSidedLighting = 0;
      }

      rreqs = (tU64)rreqs & CurUserPrimReqMask;

      // first check the user renderers

      int i;
      bool userRendererFound = false;

      for ( i = 0; i < NumUserRenderers; i++ )
	 if ( rreqs == (rreqs & UserRenderers[i].capabilities)
	      && UserRenderers[i].requirements == (rreqs & UserRenderers[i].requirements) )
	    break;

      // did we find a user renderer?
      if ( i < NumUserRenderers ) {
	 userRendererFound = true;

	 NewRenderer = &UserRenderers[i];

	 if (CurrentRenderer != &UserRenderers[i]) {
	    rendererChanged = true;
	 }
      }

      // now the default renderers

      if ( ! userRendererFound ) {
	 for ( i = 0; i < NumDefaultRenderers; i++ )
	    if ( rreqs == (rreqs & DefaultRenderers[i].capabilities)
	       && DefaultRenderers[i].requirements == (rreqs & DefaultRenderers[i].requirements) )
	       break;

	 mErrorIf( i == NumDefaultRenderers,
		   "Couldn't find a suitable renderer..\n"
		   "state reqs = 0x%08x 0x%08x, mask = %08x %08x\n",
		   (tU32)((tU64)rreqs >> 32),
		   (tU32)((tU64)rreqs),
		   (tU32)((tU64)CurUserPrimReqMask >> 32),
		   (tU32)((tU64)CurUserPrimReqMask)
	    );

	 NewRenderer = &DefaultRenderers[i];

	 if (CurrentRenderer != &DefaultRenderers[i]) {
	    // printf("vu1 renderer requirements are 0x%08x, renderer = 0x%08x i = %d\n",
	    // (unsigned int)RendererRequirements, (unsigned int)CurrentRenderer->capabilities, i);
	    rendererChanged = true;
	 }
      }
   }

   RendererReqsHaveChanged = false;

   return rendererChanged;
}

void
CRendererManager::MakeNewRendererCurrent()
{
   mAssert( NewRenderer != NULL );
   CurrentRenderer = NewRenderer;
   NewRenderer = NULL;
}

void
CRendererManager::LoadRenderer( CVifSCDmaPacket &packet )
{
   mAssert( CurrentRenderer != NULL );

     printf("Loading renderer: %s\n", CurrentRenderer->renderer->GetName() );

   CurrentRenderer->renderer->Load();

   pglAddToMetric(kMetricsRendererUpload);
}

/********************************************
 * ps2gl C api
 */

/**
 * @addtogroup pgl_api
 * @{
 */

/**
 * @addtogroup custom_renderers_prims_state defining custom renderers, primitive types, and state
 *
 * API to define custom primitive types, state chagnes, renderers, or override
 * the default renderers.
 *
 * The ps2gl code that runs on the ee core is just a large, ugly state machine --
 * all of the "real" work is done in the renderers.  There are currently a dozen or
 * so internal renderers which handle different types of geometry, lighting,
 * texturing, etc.  Most of the internal renderers implement some case that none of
 * the others can do, but a few are specializations of more general renderers that
 * implement a commonly encountered case more efficiently that the general renderer
 * can.  Using the functions described below, an application can define custom
 * renderers or override existing ones.
 *
 * But first, an explanation of how ps2gl chooses renderers is necessary.  ps2gl
 * keeps a bitfield of capabilities that a renderer must provide to be chosen to
 * render.  This bitfield is called the "renderer requirements" and describes
 * things like lighting, texturing, prim type, shading type, etc.  Each renderer
 * also keeps a corresponding bitfield describing the capabilities that it
 * provides, called its "capabilities."  Each bit means that the corresponding
 * capability is required (or provided, in the case of a renderer).  Consider the
 * "lighting" bit.  This bit is set when glEnable( GL_LIGHTING ) is called and
 * changes the renderer requirements, causing ps2gl to look for a new renderer the
 * next time geometry is rendered.  Each renderer's capabilities will be compared
 * to the current renderer requirements, and the first renderer that meets all the
 * requirements is chosen.  In this example, only a renderer that can do lit
 * geometry will be selected.
 *
 * Renderers are searched in the order they were registered, application-defined
 * renderers first.  This means that specialized renderers should be registered
 * before general ones.  It also means that to override a default renderer, an
 * application need only register a renderer that provides the same capabilites (or
 * a subset).
 *
 * Implementing a custom renderer whose purpose is not to override a built-in
 * renderer implies defining custom prim types and/or properties, for which
 * the upper 32 bits of the renderer capabilities/requirements are reserved.
 * See the function documentation below for more details.
 *
 * For more information on implementing a renderer, see the file "renderer.h" which
 * defines the CRenderer interface.  (Sorry, but the renderer needs to be a cpp
 * class.)  The best examples are the default renderers, so take a look at
 * linear_renderer.* and base_renderer.*.
 *
 * @{
 */

/**
 * Call this before registering any renderers.
 */
void
pglBeginRendererDefs()
{
   // does nothing now, but a begin/end pair could come in handy later
   // if I add caches, hashes, etc..
}

/**
 * Register a custom renderer.  The maximum number of custom renderers is
 * indicated by PGL_MAX_CUSTOM_RENDERERS.
 * @param renderer should point to a CRenderer object (passed as a void* for
 * 	  	   compatibility with C code)
 */
void
pglRegisterRenderer( void *renderer )
{
   CRenderer *newRenderer = reinterpret_cast<CRenderer*>(renderer);
   pGLContext->GetImmGeomManager().GetRendererManager().RegisterUserRenderer( newRenderer );
}

/**
 * Call this after registering custom renderers.
 */
void
pglEndRendererDefs()
{
   // see comment in begin
}

/**
 * Returns the name (string) of the current renderer.  You'll need to call glFlush()
 * to be sure that the renderer is in in sync.  Probably doesn't make much sense
 * to call this inside of a glBegin/glEnd pair.
 * @return pointer to the renderer's name
 */
const char*
pglGetCurRendererName()
{
   return pGLContext->GetImmGeomManager().GetRendererManager().GetCurRenderer().GetName();
}

/** @} */

/** @} */

