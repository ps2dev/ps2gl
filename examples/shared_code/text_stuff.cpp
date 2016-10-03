/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "GL/gl.h"
#include "GL/ps2gl.h"
#include "GL/glut.h" // for pglutAllocDmaMem

#include "ps2s/gs.h"

#include "text_stuff.h"

float q_width = 127, q_height = 255.5f;
unsigned int font_texture = 0xffffffff;
unsigned int *font_clut;
float left_margin = 20, top_margin = 20;
float cursor_x = left_margin, cursor_y = top_margin;

void
tsShowFont( const char *text )
{
   glMatrixMode( GL_MODELVIEW );
   glPushMatrix();
   glLoadIdentity();

   glMatrixMode( GL_PROJECTION );
   glPushMatrix();
   glLoadIdentity();
   glOrtho( 0, 639, 447, 0, 0, 10 );
   // glOrtho( -2, 2, -2, 2, 0, 10 );

   glEnable( GL_TEXTURE_2D );
   glBindTexture( GL_TEXTURE_2D, font_texture );
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glColorTable( GL_COLOR_TABLE, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
		 font_clut );

   glDisable( GL_LIGHTING );
   glEnable( GL_BLEND );

   float tex_u = 0, tex_v = 0,
      cell_width = 1,
      cell_height = 1;

   float x_off = 40, y_off = 20;

   glBegin( GL_QUADS );
   {
      glTexCoord2f( tex_u, tex_v );
      glVertex3f( x_off, y_off, -1 );

      glTexCoord2f( tex_u, tex_v + cell_height );
      glVertex3f( x_off, y_off + q_height, -1 );

      glTexCoord2f( tex_u + cell_width, tex_v + cell_height );
      glVertex3f( x_off + q_width, y_off + q_height, -1 );

      glTexCoord2f( tex_u + cell_width, tex_v );
      glVertex3f( x_off + q_width, y_off, -1 );
   }
   glEnd();

   glPopMatrix();
   glMatrixMode( GL_MODELVIEW );
   glPopMatrix();

   glDisable( GL_BLEND );
   glEnable( GL_LIGHTING );
}

void
tsResetCursor()
{
   cursor_x = left_margin;
   cursor_y = top_margin;
}

void
tsDrawString( const char *text )
{
   glMatrixMode( GL_MODELVIEW );
   glPushMatrix();
   glLoadIdentity();

   glMatrixMode( GL_PROJECTION );
   glPushMatrix();
   glLoadIdentity();
   glOrtho( 0, 639, 447, 0, 0, 10 );

   glEnable( GL_TEXTURE_2D );
   glBindTexture( GL_TEXTURE_2D, font_texture );
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glColorTable( GL_COLOR_TABLE, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
		 font_clut );

   glDisable( GL_LIGHTING );
   glEnable( GL_BLEND );

   float tex_u = 0, tex_v = 0,
      cell_width = 8.0f/128.0f,
      cell_height = 16.0f/256.0f;

   glBegin( GL_QUADS );
   {
      for ( int i = 0; i < (int)strlen(text); i++ ) {

	 if ( text[i] == '\n' ) {
	    cursor_x = left_margin;
	    cursor_y += 16;
	    continue;
	 }

	 tex_u = ((unsigned int)text[i] % 16) * cell_width;
	 tex_v = ((unsigned int)text[i] / 16) * cell_height;

	 glTexCoord2f( tex_u, tex_v );
	 glVertex3f( cursor_x, cursor_y, -1 );

	 glTexCoord2f( tex_u, tex_v + cell_height );
	 glVertex3f( cursor_x, cursor_y + 16, -1 );

	 glTexCoord2f( tex_u + cell_width, tex_v + cell_height );
	 glVertex3f( cursor_x + 8, cursor_y + 16, -1 );

	 glTexCoord2f( tex_u + cell_width, tex_v );
	 glVertex3f( cursor_x + 8, cursor_y, -1 );

	 cursor_x += 8;
      }
   }
   glEnd();

   glPopMatrix();
   glMatrixMode( GL_MODELVIEW );
   glPopMatrix();

   glDisable( GL_BLEND );
   glEnable( GL_LIGHTING );
}

extern unsigned char glut_font_image[128*256];

void
tsLoadFont()
{
   int font_image_size = 128 * 256;
   void *font_image = pglutAllocDmaMem(font_image_size);

   memcpy( font_image, glut_font_image, font_image_size );

   glGenTextures( 1, & font_texture );
   glBindTexture( GL_TEXTURE_2D, font_texture );
   glTexImage2D( GL_TEXTURE_2D, 0, 3,
		 128, 256, 0,
		 GL_COLOR_INDEX, GL_UNSIGNED_BYTE, font_image );

   font_clut = (unsigned int*)pglutAllocDmaMem( sizeof(int) * 256 );
   memset( font_clut, 0, sizeof(int) * 256 );
   font_clut[1] = 0xffffffff;
}
