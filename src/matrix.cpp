/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/math.h"

#include "ps2gl/matrix.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/dlist.h"
#include "ps2gl/immgmanager.h"
#include "ps2gl/dlgmanager.h"

/********************************************
 * CDListMatrixStack
 */


class CMatrixPopCmd : public CDListCmd {
   public:
      CMatrixPopCmd() {}
      CDListCmd* Play() { glPopMatrix(); return CDListCmd::GetNextCmd(this); }
};

void
CDListMatrixStack::Pop()
{
   GLContext.GetDListGeomManager().Flush();
   GLContext.GetDListManager().GetOpenDList() += CMatrixPopCmd();
   GLContext.XformChanged();
}

class CMatrixPushCmd : public CDListCmd {
   public:
      CMatrixPushCmd() {}
      CDListCmd* Play() { glPushMatrix(); return CDListCmd::GetNextCmd(this); }
};

void
CDListMatrixStack::Push()
{
   GLContext.GetDListManager().GetOpenDList() += CMatrixPushCmd();
}

class CMatrixConcatCmd : public CDListCmd {
      cpu_mat_44	Matrix, Inverse;
   public:
      CMatrixConcatCmd( const cpu_mat_44 &mat, const cpu_mat_44 &inv )
	 : Matrix(mat), Inverse(inv) {}
      CDListCmd* Play() {
	 pGLContext->GetCurMatrixStack().Concat( Matrix, Inverse );
	 return CDListCmd::GetNextCmd(this);
      }
};

void
CDListMatrixStack::Concat( const cpu_mat_44& xform, const cpu_mat_44& inverse )
{
   GLContext.GetDListGeomManager().Flush();
   GLContext.GetDListManager().GetOpenDList() += CMatrixConcatCmd( xform, inverse );
   GLContext.XformChanged();
}

class CMatrixSetTopCmd : public CDListCmd {
      cpu_mat_44	Matrix, Inverse;
   public:
      CMatrixSetTopCmd( const cpu_mat_44 &mat, const cpu_mat_44 &inv )
	 : Matrix(mat), Inverse(inv) {}
      CDListCmd* Play() {
	 pGLContext->GetCurMatrixStack().SetTop( Matrix, Inverse );
	 return CDListCmd::GetNextCmd(this);
      }
};

void
CDListMatrixStack::SetTop( const cpu_mat_44 &newMat, const cpu_mat_44 &newInv )
{
   GLContext.GetDListGeomManager().Flush();
   GLContext.GetDListManager().GetOpenDList() += CMatrixSetTopCmd( newMat, newInv );
   GLContext.XformChanged();
}


/********************************************
 * gl api
 */

void glMatrixMode( GLenum mode )
{
   pGLContext->SetMatrixMode( mode );
}

void glLoadIdentity( void )
{
   CMatrixStack& matStack = pGLContext->GetCurMatrixStack();

   cpu_mat_44 ident;
   ident.set_identity();
   matStack.SetTop( ident, ident );
}

void glPushMatrix( void )
{
   CMatrixStack& matStack = pGLContext->GetCurMatrixStack();
   matStack.Push();
}

void glPopMatrix( void )
{
   CMatrixStack& matStack = pGLContext->GetCurMatrixStack();
   matStack.Pop();
}

// courtesy the lovely folks at Intel
extern void Invert2( float *mat, float *dst );

void glLoadMatrixf( const GLfloat *m )
{
   CMatrixStack& matStack = pGLContext->GetCurMatrixStack();
   cpu_mat_44 newMat;

   // unfortunately we can't assume the matrix is qword-aligned
   float *dest = reinterpret_cast<float*>(&newMat);
   for ( int i = 0; i < 16; i++ )
      *dest++ = *m++;

   cpu_mat_44 invMatrix;
   Invert2( (float*)&newMat, (float*)&invMatrix );

   matStack.SetTop( newMat, invMatrix );
}

void glFrustum( GLdouble left, GLdouble right,
		GLdouble bottom, GLdouble top,
		GLdouble zNear, GLdouble zFar )
{
   cpu_mat_44 xform( cpu_vec_xyzw( (2.0f * zNear)
				   / (right - left),
				   0.0f,
				   0.0f,
				   0.0f ),
		     cpu_vec_xyzw( 0.0f,
				   (2.0f * zNear)
				   / (top - bottom),
				   0.0f,
				   0.0f ),
		     cpu_vec_xyzw( (right + left)
				   / (right - left),
				   (top + bottom)
				   / (top - bottom),
				   -(zFar + zNear)
				   / (zFar - zNear),
				   -1.0f ),
		     cpu_vec_xyzw( 0.0f,
				   0.0f,
				   (-2.0f * zFar * zNear)
				   / (zFar - zNear),
				   0.0f )
      );

   cpu_mat_44 inv( cpu_vec_xyzw( (right - left)
				 / (2 * zNear),
				 0,
				 0,
				 0 ),
		   cpu_vec_xyzw( 0,
				 (top - bottom)
				 / (2 * zNear),
				 0,
				 0 ),
		   cpu_vec_xyzw( 0,
				 0,
				 0,
				 -(zFar - zNear)
				 / (2 * zFar * zNear) ),
		   cpu_vec_xyzw( (right + left)
				 / (2 * zNear),
				 (top + bottom)
				 / (2 * zNear),
				 -1,
				 (zFar + zNear)
				 / (2 * zFar * zNear) )
      );

   CMatrixStack& matStack = pGLContext->GetCurMatrixStack();
   matStack.Concat( xform, inv );
}

void glOrtho( GLdouble left, GLdouble right,
	      GLdouble bottom, GLdouble top,
	      GLdouble zNear, GLdouble zFar )
{
   cpu_mat_44 xform( cpu_vec_xyzw( (2.0f)
				   / (right - left),
				   0.0f,
				   0.0f,
				   0.0f ),
		     cpu_vec_xyzw( 0.0f,
				   (2.0f)
				   / (top - bottom),
				   0.0f,
				   0.0f ),
		     cpu_vec_xyzw( 0.0f,
				   0.0f,
				   -2
				   / (zFar - zNear),
				   0.0f ),
		     cpu_vec_xyzw( - (right + left)
				   / (right - left),
				   - (top + bottom)
				   / (top - bottom),
				   - (zFar + zNear)
				   / (zFar - zNear),
				   1.0f )
      );

   cpu_mat_44 inv( cpu_vec_xyzw( (right - left)
				 / 2,
				 0,
				 0,
				 0 ),
		   cpu_vec_xyzw( 0,
				 (top - bottom)
				 / 2,
				 0,
				 0 ),
		   cpu_vec_xyzw( 0,
				 0,
				 (zFar - zNear)
				 / -2,
				 0 ),
		   cpu_vec_xyzw( (right + left)
				 / 2,
				 (top + bottom)
				 / 2,
				 (zFar + zNear)
				 / 2,
				 1 )
      );

   CMatrixStack& matStack = pGLContext->GetCurMatrixStack();
   matStack.Concat( xform, inv );
}

void glMultMatrixf( const GLfloat *m )
{
   // unfortunately we can't assume the matrix is qword-aligned
   // the casts to float below fix an apparent parse error... something's up here..
   cpu_mat_44 newMatrix( cpu_vec_xyzw( (float)m[0], (float)m[1], (float)m[2], (float)m[3] ),
			 cpu_vec_xyzw( (float)m[4], (float)m[5], (float)m[6], (float)m[7] ),
			 cpu_vec_xyzw( (float)m[8], (float)m[9], (float)m[10], (float)m[11] ),
			 cpu_vec_xyzw( (float)m[12], (float)m[13], (float)m[14], (float)m[15] ) );

   // close your eyes.. this is a temporary hack

   /*
   // assume that newMatrix consists of rotations, uniform scales, and translations
   cpu_vec_xyzw scaledVec( 1, 0, 0, 0 );
   scaledVec = newMatrix * scaledVec;
   float scale = scaledVec.length();

   cpu_mat_44 invMatrix = newMatrix;
   invMatrix.set_col3( cpu_vec_xyzw(0,0,0,0) );
   invMatrix.transpose_in_place();
   invMatrix.set_col3( -newMatrix.get_col3() );
   cpu_mat_44 scaleMat;
   scaleMat.set_scale( cpu_vec_xyz(scale, scale, scale) );
   invMatrix = scaleMat * invMatrix;
   */

   cpu_mat_44 invMatrix;
//     invMatrix.set_identity();
   Invert2( (float*)&newMatrix, (float*)&invMatrix );
   CMatrixStack& matStack = pGLContext->GetCurMatrixStack();
   matStack.Concat( newMatrix, invMatrix );

//     mWarn( "glMultMatrix is not correct" );

}

void glRotatef( GLfloat angle,
		GLfloat x, GLfloat y, GLfloat z )
{
   cpu_mat_44 xform, inverse;
   cpu_vec_xyz axis(x,y,z);
   axis.normalize();
   xform.set_rotate( Math::DegToRad(angle), axis );
   inverse.set_rotate( Math::DegToRad(-angle), axis );

   CMatrixStack& matStack = pGLContext->GetCurMatrixStack();
   matStack.Concat( xform, inverse );
}

void glScalef( GLfloat x, GLfloat y, GLfloat z )
{
   cpu_mat_44 xform, inverse;
   xform.set_scale( cpu_vec_xyz(x,y,z) );
   inverse.set_scale( cpu_vec_xyz(1.0f/x, 1.0f/y, 1.0f/z) );

   CMatrixStack& matStack = pGLContext->GetCurMatrixStack();
   matStack.Concat( xform, inverse );
}

void glTranslatef( GLfloat x, GLfloat y, GLfloat z )
{
   cpu_mat_44 xform, inverse;
   cpu_vec_xyz direction(x, y, z);
   xform.set_translate( direction );
   inverse.set_translate( -direction );

   CMatrixStack& matStack = pGLContext->GetCurMatrixStack();
   matStack.Concat( xform, inverse );
}
