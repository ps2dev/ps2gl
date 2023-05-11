/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_gblock_h
#define ps2gl_gblock_h

#include "ps2s/types.h"
#include <stdio.h>

#include "GL/gl.h"
#include "ps2gl/debug.h"

/********************************************
 * CGeometryBlock
 *
 * This classes purpose in life is to accumulate similar, contiguous geometry, where 'similar'
 * means same prim type, same number of words per vertex/normal/etc.
 */

namespace ArrayType {
typedef enum { kLinear,
    kIndexed,
    kInvalidArray } tArrayType;
}

class CGeometryBlock {
private:
    int TotalVertices;
    char WordsPerVertex, WordsPerNormal, WordsPerTexCoord, WordsPerColor;
    char NumVertsToRestartStrip, NumVertsPerPrim;
    bool StripsCanBeMerged;
    bool AreVerticesValid, AreNormalsValid, AreTexCoordsValid, AreColorsValid;

    GLenum PrimType;
    ArrayType::tArrayType ArrayType;

    static const int kMaxNumStrips          = 40;
    static const unsigned int kContinueFlag = 0x80000000;
    unsigned char NumStrips;
    unsigned int StripLengths[kMaxNumStrips];
    const void* IStripLengths[kMaxNumStrips];
    const void* Indices[kMaxNumStrips];
    const void* Vertices[kMaxNumStrips];
    const void* Normals[kMaxNumStrips];
    const void* TexCoords[kMaxNumStrips];
    const void* Colors[kMaxNumStrips];
    unsigned char NumIndices[kMaxNumStrips];

    // new geometry we are trying to add to the block

    GLenum NewPrimType;
    ArrayType::tArrayType NewArrayType;

    const void *NewVertices, *NewNormals, *NewTexCoords, *NewColors;
    const void *NewIndices, *NewIStripLengths;
    int NumNewVertices, NumNewNormals, NumNewTexCoords, NumNewColors, NumNewIndices;
    char WordsPerNewVertex, WordsPerNewNormal;
    char WordsPerNewTexCoord, WordsPerNewColor;
    bool AreNewVerticesValid, AreNewNormalsValid;
    bool AreNewTexCoordsValid, AreNewColorsValid;

    void CommitPrimType();
    bool SameDataFormat();

    bool MergeNewLinear();
    bool MergeNewIndexed();

public:
    CGeometryBlock() { Reset(); }

    // get/set info about geometry

    inline void SetVerticesAreValid(bool valid) { AreNewVerticesValid = valid; }
    inline void SetNormalsAreValid(bool valid) { AreNewNormalsValid = valid; }
    inline void SetTexCoordsAreValid(bool valid) { AreNewTexCoordsValid = valid; }
    inline void SetColorsAreValid(bool valid) { AreNewColorsValid = valid; }

    inline bool GetVerticesAreValid() const { return AreVerticesValid; }
    inline bool GetNormalsAreValid() const { return AreNormalsValid; }
    inline bool GetTexCoordsAreValid() const { return AreTexCoordsValid; }
    inline bool GetColorsAreValid() const { return AreColorsValid; }

    inline int GetWordsPerVertex() const { return WordsPerVertex; }
    inline int GetWordsPerNormal() const { return WordsPerNormal; }
    inline int GetWordsPerTexCoord() const { return WordsPerTexCoord; }
    inline int GetWordsPerColor() const { return WordsPerColor; }

    inline void SetWordsPerVertex(char num) { WordsPerNewVertex = num; }
    inline void SetWordsPerNormal(char num) { WordsPerNewNormal = num; }
    inline void SetWordsPerTexCoord(char num) { WordsPerNewTexCoord = num; }
    inline void SetWordsPerColor(char num) { WordsPerNewColor = num; }

    inline void SetArrayType(ArrayType::tArrayType type) { NewArrayType = type; }
    inline ArrayType::tArrayType GetNewArrayType() const { return NewArrayType; }
    inline ArrayType::tArrayType GetArrayType() const { return ArrayType; }

    inline void SetNumIndices(unsigned int num) { NumNewIndices = num; }
    inline void SetIndices(const void* indices) { NewIndices = indices; }
    inline void SetIStripLengths(const void* strips) { NewIStripLengths = strips; }

    inline const void* GetVertices(int strip = 0)
    {
        mErrorIf(strip >= NumStrips, "Strip num is out of bounds");
        return Vertices[strip];
    }
    inline const void* GetNormals(int strip = 0)
    {
        mErrorIf(strip >= NumStrips, "Strip num is out of bounds");
        return Normals[strip];
    }
    inline const void* GetTexCoords(int strip = 0)
    {
        mErrorIf(strip >= NumStrips, "Strip num is out of bounds");
        return TexCoords[strip];
    }
    inline const void* GetColors(int strip = 0)
    {
        mErrorIf(strip >= NumStrips, "Strip num is out of bounds");
        return Colors[strip];
    }
    inline const void* GetIndices(int array)
    {
        mErrorIf(array >= NumStrips, "Strip num is out of bounds");
        return Indices[array];
    }
    inline const void* GetIStripLengths(int array)
    {
        mErrorIf(array >= NumStrips, "Strip num is out of bounds");
        return IStripLengths[array];
    }

    inline void SetVertices(const void* verts) { NewVertices = verts; }
    inline void SetNormals(const void* norms) { NewNormals = norms; }
    inline void SetTexCoords(const void* texcoords) { NewTexCoords = texcoords; }
    inline void SetColors(const void* colors) { NewColors = colors; }

    GLenum GetPrimType() const { return PrimType; }
    void SetPrimType(GLenum type) { NewPrimType = type; }

    inline int GetNumNewVertices() const { return NumNewVertices; }
    inline int GetNumNewNormals() const { return NumNewNormals; }
    inline int GetNumNewTexCoords() const { return NumNewTexCoords; }
    inline int GetNumNewColors() const { return NumNewColors; }

    inline int GetTotalVertices() const { return TotalVertices; }

    // adding geometry

    inline void AddVertices(int num = 1) { NumNewVertices += num; }
    inline void AddNormals(int num = 1) { NumNewNormals += num; }
    inline void AddTexCoords(int num = 1) { NumNewTexCoords += num; }
    inline void AddColors(int num = 1) { NumNewColors += num; }

    // prim

    // this is here for custom renderers/prim types.. use with caution!!
    inline void SetNumVertsPerPrim(int num) { NumVertsPerPrim = num; }
    inline int GetNumVertsPerPrim() { return NumVertsPerPrim; }

    // strip related

    // this is here for custom renderers/prim types.. use with caution!!
    inline void SetNumVertsToRestartStrip(int num) { NumVertsToRestartStrip = num; }
    inline void SetStripsCanBeMerged(bool merge) { StripsCanBeMerged = merge; }

    inline int GetNumStrips() const { return NumStrips; }
    /// can the strips in this block be merged into the same render
    /// buffer and be rendered with a single giftag/prim setting?
    /// (triangle fans can't)
    inline int GetNumVertsToRestartStrip() { return NumVertsToRestartStrip; }
    inline bool GetStripsCanBeMerged() const { return StripsCanBeMerged; }
    inline int GetStripLength(int num) const
    {
        mErrorIf(num >= NumStrips, "Strip num is out of bounds");
        return (int)(StripLengths[num] & ~kContinueFlag);
    }
    inline bool StripIsContinued(int num) const
    {
        mErrorIf(num >= NumStrips, "Strip num is out of bounds");
        return StripLengths[num] & kContinueFlag;
    }

    // "array" related (for indexed arrays)

    inline int GetNumArrays() const { return NumStrips; }
    inline int GetArrayLength(int array) const
    {
        return GetStripLength(array);
    }
    inline int GetNumIndices(int array) const
    {
        mErrorIf(array >= NumStrips, "Strip num is out of bounds");
        return NumIndices[array];
    }

    // reset

    void ResetCurStrip();
    void ResetNew();
    void Reset();

    // merge / commit related

    bool IsPending() const { return (PrimType != GL_INVALID_VALUE); }
    bool MergeNew();
    void MakeNewValuesCurrent();
    void AdjustNewGeomPtrs(int offset)
    {
        if (AreNewVerticesValid)
            NewVertices = (float*)NewVertices + offset * WordsPerNewVertex;
        if (AreNewNormalsValid)
            NewNormals = (float*)NewNormals + offset * WordsPerNewNormal;
        if (AreNewTexCoordsValid)
            NewTexCoords = (float*)NewTexCoords + offset * WordsPerNewTexCoord;
        if (AreNewColorsValid)
            NewColors = (float*)NewColors + offset * WordsPerNewColor;
    }
};

#endif // ps2gl_gblock_h
