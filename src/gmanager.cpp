/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <stdio.h>

#include "GL/ps2gl.h"

#include "ps2s/packet.h"
#include "ps2s/cpu_matrix.h"
#include "ps2s/math.h"
#include "ps2s/displayenv.h"

#include "ps2gl/gmanager.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/dlist.h"
#include "ps2gl/clear.h"
#include "ps2gl/matrix.h"
#include "ps2gl/debug.h"


/********************************************
 * class CVertArray
 */

CVertArray::CVertArray()
{
   Vertices = Normals = TexCoords = Colors = NULL;
   VerticesAreValid = NormalsAreValid = TexCoordsAreValid = ColorsAreValid = false;
   WordsPerVertex = WordsPerTexCoord = WordsPerColor = 0;
   WordsPerNormal = 3; // not set by NormalPointer
}

/********************************************
 * class CGeomManager
 */

// static members

CVertArray 	*CGeomManager::VertArray;

tUserPrimEntry	CGeomManager::UserPrimTypes[kMaxUserPrimTypes];

bool 		CGeomManager::DoNormalize = false;

CGeomManager::CGeomManager( CGLContext &context )
   : GLContext(context),
     CurNormal(0.0f, 0.0f, 0.0f),
     Prim(GL_INVALID_VALUE),
     InsideBeginEnd(false),
     LastArrayAccessWasIndexed(false), LastArrayAccessIsValid(false),
     UserRenderContextChanged(false)
{
   for ( unsigned int i = 0; i < kMaxUserPrimTypes; i++ )
      UserPrimTypes[i].requirements = 0xffffffff;
}

/********************************************
 * gl api
 */

/**
 * @defgroup gl_api gl* API
 *
 * Differences between ps2gl gl* functions and the usual ones.
 * (If a gl* function is called with parameters that are
 * unsupported/broken, it should say so.)
 *
 * @{
 */



/**
 * @param size 2, 3, or 4
 * @param type must be GL_FLOAT
 * @param stride must be <b>zero</b>.  Non-zero strides are unsupported and likely
 * to remain so.
 */
void glVertexPointer( GLint size, GLenum type,
		      GLsizei stride, const GLvoid *ptr )
{
   if ( stride != 0 ) {
      mNotImplemented( "stride must be 0" );
      return;
   }
   if ( type != GL_FLOAT ) {
      mNotImplemented( "type must be float" );
      return;
   }

   CVertArray &vertArray = pGLContext->GetGeomManager().GetVertArray();
   vertArray.SetVertices( (void*)ptr );
   vertArray.SetWordsPerVertex( size );
}

/**
 * @param type must be GL_FLOAT
 * @param stride must be <b>zero</b>.  Non-zero strides are unsupported and likely
 * to remain so.
 */
void glNormalPointer( GLenum type, GLsizei stride,
		      const GLvoid *ptr )
{
   pglNormalPointer( 3, type, stride, ptr );
}

/**
 * @param size 2, 3, or 4
 * @param type must be GL_FLOAT
 * @param stride must be <b>zero</b>.  Non-zero strides are unsupported and likely
 * to remain so.
 */
void glTexCoordPointer( GLint size, GLenum type,
			GLsizei stride, const GLvoid *ptr )
{
   if ( stride != 0 ) {
      mNotImplemented( "stride must be 0" );
      return;
   }
   if ( type != GL_FLOAT ) {
      mNotImplemented( "type must be float" );
      return;
   }

   CVertArray &vertArray = pGLContext->GetGeomManager().GetVertArray();
   vertArray.SetTexCoords( (void*)ptr );
   vertArray.SetWordsPerTexCoord( size );
}

/**
 * @param size 3 or 4
 * @param type must be GL_FLOAT
 * @param stride must be <b>zero</b>.  Non-zero strides are unsupported and likely
 * to remain so.
 */
void glColorPointer( GLint size, GLenum type,
		     GLsizei stride, const GLvoid *ptr )
{
   if ( stride != 0 ) {
      mNotImplemented( "stride must be 0" );
      return;
   }
   if ( type != GL_FLOAT ) {
      mNotImplemented( "type must be float" );
      return;
   }

   CVertArray &vertArray = pGLContext->GetGeomManager().GetVertArray();
   vertArray.SetColors( (void*)ptr );
   vertArray.SetWordsPerColor( size );
}

/**
 * The important thing to remember with DrawArrays() is that <b>array data
 * is not copied (mostly)</b>.  Since the only rendering mode supported now is
 * delayed one frame, this means that the app must <b>double-buffer geometry</b>
 * when it changes.  The "mostly" above is because little bits of the array
 * will become part of the dma chain, so modifying the data referenced by a
 * display list won't work as expected.  (This would be really useful and should
 * be made possible.)
 *
 * There is no limit on strip lengths (make them as long as possible!).
 */
void glDrawArrays( GLenum mode, GLint first, GLsizei count )
{
   CGeomManager& gmanager = pGLContext->GetGeomManager();
   gmanager.DrawArrays( mode, first, count );
}

/**
 * This is not implemented yet
 */
void glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
   mError("glDrawElements is a placeholder ATM and should not be called");
}

/**
 * This is not implemented yet
 */
void glInterleavedArrays( GLenum format, GLsizei stride, const GLvoid *pointer )
{
   mError("glInterleavedArrays is a placeholder ATM and should not be called");
}

/**
 * This is not implemented yet
 */
void glArrayElement( GLint i )
{
   mError("glArrayElement is a placeholder ATM and should not be called");
}

/**
 * Flushes the internal geometry buffers.
 */
void glFlush( void )
{
   CGeomManager& gmanager = pGLContext->GetGeomManager();
   gmanager.Flush();
}

/** @} */ // gl_api

void glEnableClientState( GLenum cap )
{
   CGeomManager& gmanager = pGLContext->GetGeomManager();
   CVertArray &vertArray = gmanager.GetVertArray();

   switch (cap) {
      case GL_NORMAL_ARRAY: vertArray.SetNormalsValid(true); break;
      case GL_VERTEX_ARRAY: vertArray.SetVerticesValid(true); break;
      case GL_COLOR_ARRAY: vertArray.SetColorsValid(true); break;
      case GL_TEXTURE_COORD_ARRAY: vertArray.SetTexCoordsValid(true); break;

      case GL_INDEX_ARRAY:
      case GL_EDGE_FLAG_ARRAY:
	 mNotImplemented( "capability = %d", cap );
	 break;
   }
}

void glDisableClientState( GLenum cap )
{
   CGeomManager &gmanager = pGLContext->GetGeomManager();
   CVertArray &vertArray = gmanager.GetVertArray();

   switch (cap) {
      case GL_NORMAL_ARRAY: vertArray.SetNormalsValid(false); break;
      case GL_VERTEX_ARRAY: vertArray.SetVerticesValid(false); break;
      case GL_COLOR_ARRAY: vertArray.SetColorsValid(false); break;
      case GL_TEXTURE_COORD_ARRAY: vertArray.SetTexCoordsValid(false); break;

      case GL_INDEX_ARRAY:
      case GL_EDGE_FLAG_ARRAY:
	 mNotImplemented( "capability = %d", cap );
	 break;
   }
}



void glBegin( GLenum mode )
{
   CGeomManager& gmanager = pGLContext->GetGeomManager();
   gmanager.BeginGeom( mode );
}

void glNormal3f( GLfloat x, GLfloat y, GLfloat z )
{
   CGeomManager& gmanager = pGLContext->GetGeomManager();
   gmanager.Normal( cpu_vec_xyz(x, y, z) );
}

void glNormal3fv( const GLfloat *v )
{
   glNormal3f( v[0], v[1], v[2] );
}

void glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   CGeomManager &gmanager = pGLContext->GetGeomManager();
   gmanager.Vertex( cpu_vec_xyzw(x, y, z, w) );
}

void glVertex4fv( const GLfloat *vertex )
{
   glVertex4f( vertex[0], vertex[1], vertex[2], vertex[3] );
}

void glVertex3f( GLfloat x, GLfloat y, GLfloat z )
{
   glVertex4f(x, y, z, 1.0f);
}

void glVertex3fv( const GLfloat *vertex )
{
   glVertex4f( vertex[0], vertex[1], vertex[2], 1.0f );
}

void glTexCoord2f( GLfloat u, GLfloat v )
{
   CGeomManager &gmanager = pGLContext->GetGeomManager();
   gmanager.TexCoord( u, v );
}

void glTexCoord2fv( const GLfloat *texCoord )
{
   glTexCoord2f( texCoord[0], texCoord[1] );
}

void glColor3f( GLfloat red, GLfloat green, GLfloat blue )
{
   CGeomManager &gmanager = pGLContext->GetGeomManager();
   gmanager.Color( cpu_vec_xyzw(red, green, blue, 1.0f) );
}

void glColor3fv( const GLfloat *color )
{
   glColor3f( color[0], color[1], color[2] );
}

void glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
   CGeomManager &gmanager = pGLContext->GetGeomManager();
   gmanager.Color( cpu_vec_xyzw(red, green, blue, alpha) );
}

void glColor4fv( const GLfloat *color )
{
   glColor4f( color[0], color[1], color[2], color[3] );
}

void glEnd( void )
{
   CGeomManager &gmanager = pGLContext->GetGeomManager();
   gmanager.EndGeom();
}

/********************************************
 * pgl api
 */

/**
 * @addtogroup pgl_api
 * @{
 */

/**
 * Specify a normal pointer with either 3 or 4 elements.
 * If 4-element normals are specified, the last element (w)
 * will be ignored.
 */
void pglNormalPointer( GLint size, GLenum type,
		       GLsizei stride, const GLvoid *ptr )
{
   if ( stride != 0 ) {
      mNotImplemented( "stride must be 0" );
      return;
   }
   if ( type != GL_FLOAT ) {
      mNotImplemented( "type must be float" );
      return;
   }

   CVertArray &vertArray = pGLContext->GetGeomManager().GetVertArray();
   vertArray.SetNormals( (void*)ptr );
   vertArray.SetWordsPerNormal(size);
}

void pglDrawIndexedArrays( GLenum primType,
			   int numIndices, const unsigned char* indices,
			   int numVertices )
{
   pGLContext->GetGeomManager().DrawIndexedArrays(primType, numIndices, indices, numVertices);
}

/**
 * @addtogroup custom_renderers_prims_state
 * @{
 */

/**
 * Register a new primitive.  After registering a primitive with this call it can
 * be used anywhere a normal primitive can be used (glBegin, glDrawArrays, etc.).
 * Defining a new primitive usually implies writing a renderer to go along with it.
 *
 * @param primType	    bit 31 must be set (this indicates a user prim to ps2gl).  The
 * 			    lower 31 bits should be a number from 0 to
 * 			    PGL_MAX_CUSTOM_PRIM_TYPES.
 * @param requirements	    gives the bit flags to be set in the renderer
 * 			    requirements bitfield (see the documentation for custom
 * 			    renderers).  Usually this will be one bit indicating
 * 			    the prim type used to select a renderer.
 * @param rendererReqMask   a mask to be applied to the renderer requirements
 * 			    bitfield before testing against renderer capabilities.
 * 			    This could be used, for example, to mask off the
 * 			    default lower 32 bits if they are irrelevent to this
 * 			    custom primitive type.
 * @param mergeContiguous   rather calling a renderer with every block of geometry
 * 			    that is specified (with glBegin/End, DrawArrays, etc.),
 * 			    ps2gl tries to combine multiple blocks into a single
 * 			    call to the renderer.  This flag tells ps2gl whether it
 * 			    can treat blocks of geometry that were specified
 * 			    independently but are contiguous in memory as a single
 * 			    block.  (State changes will of course force them to be
 * 			    treated separately.)  This is done, for example, with
 * 			    points, lines, triangles, and quads, but not with
 * 			    strips, since merging would lose the strip boundaries.
 */
void
pglRegisterCustomPrimType( GLenum primType,
			   pglU64_t requirements, pglU64_t rendererReqMask, int mergeContiguous )
{
   mErrorIf( ! CGeomManager::IsUserPrimType(primType), "custom prim types must have bit 31 set" );
   CGeomManager::RegisterUserPrimType( primType, requirements, rendererReqMask, mergeContiguous );
}

/**
 * Enable a custom attribute/state change.  Call this to enable the corresponding
 * bit(s) in the renderer requirements bitfield (see above).  The lower 32 bits should
 * be zero.
 * @param flag the bit(s) to enable (lower 32 should be zero)
 */
void
pglEnableCustom( pglU64_t flag )
{
   flag &= ~(tU64)0xffffffff;
   pGLContext->GetGeomManager().EnableCustom( flag );
}

/**
 * Disable a custom attribute/state change.  Call this to disable the corresponding
 * bit(s) in the renderer requirements bitfield.  The lower 32 bits should be zero.
 * @param flag the bit(s) to disable (lower 32 should be zero).  This is the same
 *             constant passed to pglEnableCustom.
 */
void
pglDisableCustom( pglU64_t flag )
{
   flag &= ~(tU64)0xffffffff;
   pGLContext->GetGeomManager().DisableCustom( flag );
}

void
pglUserRenderContextChanged()
{
   pGLContext->GetGeomManager().SetUserRenderContextChanged();
}

/** @} */

/** @} */
