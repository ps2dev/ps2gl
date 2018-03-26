/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef PS2GLMESH_H
#define PS2GLMESH_H

typedef struct ps2glMeshHeader {
    int nStrips;
} ps2GlMeshHeader;

typedef struct ps2glStripHeader {
    int nVertices;
    int nNormals;
    int nUVs;
    int nRGBAs;
} ps2GlStripHeader;

typedef struct ps2glMeshVertex {
    float x;
    float y;
    float z;
} ps2glMeshVertex;

typedef struct ps2glMeshNormal {
    float nx;
    float ny;
    float nz;
} ps2glMeshNormal;

typedef struct ps2glMeshRGBA {
    float r;
    float g;
    float b;
    float a;
} ps2glMeshRGBA;

typedef struct ps2glMeshUV {
    float u;
    float v;
} ps2glMeshUV;

#endif
