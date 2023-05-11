/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2gl/gblock.h"
#include "ps2gl/gmanager.h"

using namespace ArrayType;

/********************************************
 * CGeometryBlock
 */

bool CGeometryBlock::SameDataFormat()
{
    bool same = true;

    if (AreVerticesValid != AreNewVerticesValid
        || AreNormalsValid != AreNewNormalsValid
        || AreTexCoordsValid != AreNewTexCoordsValid
        || AreColorsValid != AreNewColorsValid)
        same = false;
    else if (AreNewVerticesValid && WordsPerVertex != WordsPerNewVertex)
        same = false;
    else if (AreNewNormalsValid && WordsPerNormal != WordsPerNewNormal)
        same = false;
    else if (AreNewTexCoordsValid && WordsPerTexCoord != WordsPerNewTexCoord)
        same = false;
    else if (AreNewColorsValid && WordsPerColor != WordsPerNewColor)
        same = false;

    return same;
}

bool CGeometryBlock::MergeNew()
{
    mAssert(IsPending());
    mErrorIf(NumNewVertices == 0, "Trying to merge geometry with no vertices!");

    bool success = false;

    if (NewArrayType == kLinear)
        success = MergeNewLinear();
    else
        success = MergeNewIndexed();

    return success;
}

bool CGeometryBlock::MergeNewIndexed()
{
    bool success = true;

    if (NewPrimType != PrimType
        || NewArrayType != ArrayType
        || NumStrips >= kMaxNumStrips)
        success = false;

    if (success) {
        Vertices[NumStrips]  = NewVertices;
        Normals[NumStrips]   = NewNormals;
        TexCoords[NumStrips] = NewTexCoords;
        Colors[NumStrips]    = NewColors;

        StripLengths[NumStrips]  = (unsigned int)NumNewVertices;
        IStripLengths[NumStrips] = NewIStripLengths;
        Indices[NumStrips]       = NewIndices;
        NumIndices[NumStrips]    = NumNewIndices;

        NumStrips++;

        ResetNew();
    }

    return success;
}

bool CGeometryBlock::MergeNewLinear()
{
    bool success = true;

    if (NewPrimType != PrimType
        || NewArrayType != ArrayType
        || !SameDataFormat()
        || NumStrips == kMaxNumStrips)
        success = false;
    else {
        bool merged = false;

        TotalVertices += NumNewVertices;

        // if the new and old vertices, normals, tex, .. whatever
        // are all contiguous in memory they can be combined into the same list of
        // primitives

        int stripLength = GetStripLength(NumStrips - 1);
        if ((float*)Vertices[NumStrips - 1] + WordsPerVertex * stripLength
                == (float*)NewVertices
            && (!AreNormalsValid
                   || (float*)Normals[NumStrips - 1] + WordsPerNormal * stripLength
                       == (float*)NewNormals)
            && (!AreTexCoordsValid
                   || (float*)TexCoords[NumStrips - 1] + WordsPerTexCoord * stripLength
                       == (float*)NewTexCoords)
            && (!AreColorsValid
                   || (float*)Colors[NumStrips - 1] + WordsPerColor * stripLength
                       == (float*)NewColors)) {
            // is this a user-defined prim type?
            bool mergeUser = true;
            if (CGeomManager::IsUserPrimType(PrimType))
                mergeUser = CGeomManager::GetUserPrimMerge(PrimType);

            // strips can't really be merged because they have to be broken
            // at their boundaries in vu1 by setting adc bits
            if (PrimType != GL_LINE_STRIP
                && PrimType != GL_TRIANGLE_STRIP
                && PrimType != GL_TRIANGLE_FAN
                && PrimType != GL_QUAD_STRIP
                && mergeUser) {
                merged = true;
                StripLengths[NumStrips - 1] += (unsigned int)NumNewVertices;
            } else if (PrimType != GL_TRIANGLE_FAN) {
                StripLengths[NumStrips - 1] |= kContinueFlag;
            }
        }

        if (!merged) {
            // can't be combined with previous list
            Vertices[NumStrips]  = NewVertices;
            Normals[NumStrips]   = NewNormals;
            TexCoords[NumStrips] = NewTexCoords;
            Colors[NumStrips]    = NewColors;

            StripLengths[NumStrips] = (unsigned int)NumNewVertices;
        }

        if (!merged)
            NumStrips++;

        ResetNew();
    }

    return success;
}

void CGeometryBlock::MakeNewValuesCurrent()
{
    CommitPrimType();
    ArrayType = NewArrayType;

    NumStrips = 0;

    Vertices[NumStrips]      = NewVertices;
    Normals[NumStrips]       = NewNormals;
    TexCoords[NumStrips]     = NewTexCoords;
    Colors[NumStrips]        = NewColors;
    IStripLengths[NumStrips] = NewIStripLengths;
    Indices[NumStrips]       = NewIndices;
    NumIndices[NumStrips]    = NumNewIndices;

    TotalVertices           = NumNewVertices;
    StripLengths[NumStrips] = (unsigned int)NumNewVertices;

    WordsPerVertex   = WordsPerNewVertex;
    WordsPerNormal   = WordsPerNewNormal;
    WordsPerTexCoord = WordsPerNewTexCoord;
    WordsPerColor    = WordsPerNewColor;

    AreVerticesValid  = AreNewVerticesValid;
    AreNormalsValid   = AreNewNormalsValid;
    AreTexCoordsValid = AreNewTexCoordsValid;
    AreColorsValid    = AreNewColorsValid;

    NumStrips = 1;
}

void CGeometryBlock::ResetNew()
{
    NewPrimType    = GL_INVALID_VALUE;
    NewArrayType   = kInvalidArray;
    NumNewVertices = NumNewNormals = NumNewTexCoords = NumNewColors = 0;

    NewVertices = NewNormals = NewTexCoords = NewColors = NULL;
    NewIndices = NewIStripLengths = NULL;
    NumNewIndices                 = 0;
    WordsPerNewVertex = WordsPerNewNormal = WordsPerNewTexCoord = WordsPerNewColor = 0;
    AreNewVerticesValid = AreNewNormalsValid = AreNewTexCoordsValid = AreNewColorsValid = false;
}

void CGeometryBlock::ResetCurStrip()
{
    Vertices[NumStrips] = Normals[NumStrips] = NULL;
    TexCoords[NumStrips] = Colors[NumStrips] = NULL;
    StripLengths[NumStrips]                  = 0;
    Indices[NumStrips]                       = 0;
    IStripLengths[NumStrips]                 = 0;
    NumIndices[NumStrips]                    = 0;
}

void CGeometryBlock::Reset()
{
    TotalVertices  = 0;
    WordsPerVertex = WordsPerNormal = WordsPerTexCoord = WordsPerColor = 0;
    AreVerticesValid = AreNormalsValid = AreTexCoordsValid = AreColorsValid = false;
    PrimType                                                                = GL_INVALID_VALUE;
    NumVertsPerPrim = NumVertsToRestartStrip = -1;
    NumStrips                                = 0;
    ResetCurStrip();
    ResetNew();
}

void CGeometryBlock::CommitPrimType()
{
    if (PrimType != NewPrimType) {
        if (!CGeomManager::IsUserPrimType(NewPrimType)) {
            switch (NewPrimType) {
            case GL_POINTS:
                NumVertsPerPrim        = 1;
                NumVertsToRestartStrip = 0;
                break;
            case GL_LINE_STRIP:
                NumVertsPerPrim        = 1;
                NumVertsToRestartStrip = 1;
                break;
            case GL_TRIANGLE_STRIP:
            case GL_TRIANGLE_FAN:
            case GL_QUAD_STRIP:
            case GL_POLYGON:
                // the number of verts per prim is used to determine where
                // (or where not) to break a list of primitives (in DrawArrays).
                // Let's pretend the triangle strips have 2 verts per primitive
                // so that they will only be broken on even vertex boundaries
                // because otherwise the face culling test gets out of sync...
                NumVertsPerPrim        = 2;
                NumVertsToRestartStrip = 2;
                break;
            case GL_LINES:
                NumVertsPerPrim        = 2;
                NumVertsToRestartStrip = 0;
                break;
            case GL_TRIANGLES:
                NumVertsPerPrim        = 3;
                NumVertsToRestartStrip = 0;
                break;
            case GL_QUADS:
                NumVertsPerPrim        = 4;
                NumVertsToRestartStrip = 0;
                break;
            default:
                mError("Unknown prim type: 0x%08x", NewPrimType);
            }

            if (NewPrimType == GL_TRIANGLE_FAN)
                StripsCanBeMerged = false;
            else
                StripsCanBeMerged = true;
        }

        PrimType = NewPrimType;
    }
}
