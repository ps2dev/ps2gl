/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_base_renderer_h
#define ps2gl_base_renderer_h

#include "ps2gl/renderer.h"

#include "ps2s/cpu_vector.h"
#include "ps2s/gs.h"

/********************************************
 * code common to ps2gl's built-in renderers
 */

class CBaseRenderer : public CRenderer {
   protected:
      // cache some state for use in DrawArrays
      bool		XferVertices, XferColors, XferNormals, XferTexCoords;

      bool		VifDoubleBuffered;

      // cached in DrawArrays from geometry manager for XferBlock
      float		CurTexCoord[2];
      cpu_vec_xyz	CurNormal;
      CDmaPacket	*TexCoordBuf, *NormalBuf;

      int		WordsPerVertex, WordsPerNormal, WordsPerTexCoord, WordsPerColor;
      unsigned int	VertexUnpackMode, NormalUnpackMode;
      unsigned int	TexCoordUnpackMode, ColorUnpackMode;
      Vifs::tMask	VertexUnpackMask, NormalUnpackMask;
      Vifs::tMask	TexCoordUnpackMask, ColorUnpackMask;

      int		InputQuadsPerVert, OutputQuadsPerVert;
      int		InputGeomOffset;

      void		*MicrocodePacket;

      const char	*Name;

      CBaseRenderer( void *packet,
		     int inQuadsPerVert, int outQuadsPerVert,
		     int inGeomOffset, const char *name)
	 : XferVertices(true), VifDoubleBuffered(true),
	   WordsPerVertex(0), WordsPerNormal(0), WordsPerTexCoord(0), WordsPerColor(0),
	   InputQuadsPerVert(inQuadsPerVert), OutputQuadsPerVert(outQuadsPerVert),
	   InputGeomOffset(inGeomOffset),
	   MicrocodePacket(packet),
	   Name(name)
      {}

      CBaseRenderer( void *packet, CRendererProps caps, CRendererProps reqs,
		     int inQuadsPerVert, int outQuadsPerVert,
		     int inGeomOffset, const char *name)
	 : CRenderer(caps, reqs),
	   XferVertices(true), VifDoubleBuffered(true),
	   WordsPerVertex(0), WordsPerNormal(0), WordsPerTexCoord(0), WordsPerColor(0),
	   InputQuadsPerVert(inQuadsPerVert), OutputQuadsPerVert(outQuadsPerVert),
	   InputGeomOffset(inGeomOffset),
	   MicrocodePacket(packet),
	   Name(name)
      {}

      void SetVifDoubleBuffered( bool db ) { VifDoubleBuffered = db; }

      // used by InitXferBlock
      void GetUnpackAttribs( int numWords, unsigned int &mode, Vifs::tMask &mask );

      // called by DrawArrays
      void InitXferBlock( CVifSCDmaPacket &packet,
			  int wordsPerVertex, int wordsPerNormal,
			  int wordsPerTex, int wordsPerColor );

      // used by DrawBlock
      void XferBlock( CVifSCDmaPacket &packet,
		      const void *vertices, const void *normals,
		      const void *texCoords, const void *colors,
		      int vu1Offset, int firstElement, int numToAdd );

      // used by XferBlock
      void XferVectors( CVifSCDmaPacket &packet, unsigned int *dataStart,
			int startOffset, int numVectors, int wordsPerVec,
			Vifs::tMask unpackMask, tU32 unpackMode,
			int vu1MemOffset );

      // used by InitContext
      void AddVu1RendererContext( CVifSCDmaPacket &packet, GLenum primType, int vu1Offset );
      tGifTag BuildGiftag( GLenum primType );
      void CacheRendererState();

      float GetMaxColorValue( bool texEnabled ) {
	 // when texturing in modulate mode, 100% of a channel occurs at
	 // 0.5 on the gs, so we want a color component value of 1.0 to map to
	 // 128, else 255
	 return (texEnabled) ? 128.0f : 255.0f;
      }
   public:
      virtual void Load();
      virtual const char* GetName() { return Name; }
};

#endif // ps2gl_base_renderer_h
