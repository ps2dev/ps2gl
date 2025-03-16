/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_gmanager_h
#define ps2gl_gmanager_h

#include "GL/gl.h"
#include "ps2s/cpu_matrix.h"
#include "ps2s/cpu_vector.h"
#include "ps2s/gs.h"
#include "ps2s/packet.h"

#include "ps2gl/gblock.h"
#include "ps2gl/renderermanager.h"

/********************************************
 * constants
 */

/********************************************
 * CVertArray
 */

class CVertArray {
    void *Vertices, *Normals, *TexCoords, *Colors;
    bool VerticesAreValid, NormalsAreValid, TexCoordsAreValid, ColorsAreValid;
    char WordsPerVertex, WordsPerNormal, WordsPerTexCoord, WordsPerColor;

public:
    CVertArray();

    inline bool GetVerticesAreValid() const { return VerticesAreValid; }
    inline bool GetNormalsAreValid() const { return NormalsAreValid; }
    inline bool GetTexCoordsAreValid() const { return TexCoordsAreValid; }
    inline bool GetColorsAreValid() const { return ColorsAreValid; }

    inline void SetVerticesValid(bool valid) { VerticesAreValid = valid; }
    inline void SetNormalsValid(bool valid) { NormalsAreValid = valid; }
    inline void SetTexCoordsValid(bool valid) { TexCoordsAreValid = valid; }
    inline void SetColorsValid(bool valid) { ColorsAreValid = valid; }

    inline void* GetVertices() const { return Vertices; }
    inline void* GetNormals() const { return Normals; }
    inline void* GetTexCoords() const { return TexCoords; }
    inline void* GetColors() const { return Colors; }

    inline void SetVertices(void* newPtr) { Vertices = newPtr; }
    inline void SetNormals(void* newPtr) { Normals = newPtr; }
    inline void SetTexCoords(void* newPtr) { TexCoords = newPtr; }
    inline void SetColors(void* newPtr) { Colors = newPtr; }

    inline int GetWordsPerVertex() const { return WordsPerVertex; }
    inline int GetWordsPerNormal() const { return WordsPerNormal; }
    inline int GetWordsPerTexCoord() const { return WordsPerTexCoord; }
    inline int GetWordsPerColor() const { return WordsPerColor; }

    inline void SetWordsPerVertex(int numWords) { WordsPerVertex = numWords; }
    inline void SetWordsPerNormal(int numWords) { WordsPerNormal = numWords; }
    inline void SetWordsPerTexCoord(int numWords) { WordsPerTexCoord = numWords; }
    inline void SetWordsPerColor(int numWords) { WordsPerColor = numWords; }

    //        // for indexed arrays only

    //        inline void SetIndices( void *newPtr ) { Indices = newPtr; }
    //        inline void SetStripLengths( void *newPtr ) { StripLengths = newPtr; }
    //        inline void SetNumIndices( int num ) { NumIndices = num; }
    //        inline void SetNumStrips( int num ) { NumStrips = num; }

    //        inline void* GetIndices() { return Indices; }
    //        inline void* GetStripLengths() { return StripLengths; }
    //        inline int GetNumIndices() { return NumIndices; }
    //        inline int GetNumStrips() { return NumStrips; }
};

/********************************************
 * types
 */

typedef struct {
    uint64_t requirements;
    uint64_t rendererReqMask;
    bool mergeContiguous;
} tUserPrimEntry;

/********************************************
 * CGeomManager - contains code common to the display list and immediate renderers
 */

class CVifSCDmaPacket;
class CGLContext;
class CDList;

class CGeomManager {
protected:
    CGLContext& GLContext;

    // vertex array geometry
    static CVertArray* VertArray;

    static const unsigned int kMaxUserPrimTypes = PGL_MAX_CUSTOM_PRIM_TYPES;
    static tUserPrimEntry UserPrimTypes[kMaxUserPrimTypes];

    // GL state
    cpu_vec_xyz CurNormal;
    float CurTexCoord[2];
    static bool DoNormalize;

    GLenum Prim;

    bool InsideBeginEnd;

    bool LastArrayAccessWasIndexed, LastArrayAccessIsValid;

    bool UserRenderContextChanged;

    static inline void CheckPrimAccess(GLenum prim)
    {
        prim &= 0x7fffffff;
        mErrorIf(prim >= kMaxUserPrimTypes,
            "trying to access prim %d; max number of custom prim types is %d\n",
            prim, kMaxUserPrimTypes);
    }

public:
    CGeomManager(CGLContext& context);

    // user prim types

    static inline bool IsUserPrimType(unsigned int prim) { return (prim & 0x80000000); }

    static inline void RegisterUserPrimType(GLenum prim,
        uint64_t requirements,
        uint64_t rendererReqMask,
        bool mergeContiguous)
    {
        CheckPrimAccess(prim);
        prim &= 0x7fffffff;
        UserPrimTypes[prim].requirements    = requirements;
        UserPrimTypes[prim].rendererReqMask = rendererReqMask;
        UserPrimTypes[prim].mergeContiguous = mergeContiguous;
    }

    static inline uint64_t GetUserPrimRequirements(GLenum prim)
    {
        CheckPrimAccess(prim);
        prim &= 0x7fffffff;
        return UserPrimTypes[prim].requirements;
    }

    static inline bool GetUserPrimMerge(GLenum prim)
    {
        CheckPrimAccess(prim);
        prim &= 0x7fffffff;
        return UserPrimTypes[prim].mergeContiguous;
    }

    static inline uint64_t GetUserPrimReqMask(GLenum prim)
    {
        CheckPrimAccess(prim);
        prim &= 0x7fffffff;
        return UserPrimTypes[prim].rendererReqMask;
    }

    void SetUserRenderContextChanged() { UserRenderContextChanged = true; }

    // GL state

    inline cpu_vec_xyz GetCurNormal() const { return CurNormal; }
    inline void SetCurNormal(cpu_vec_xyz normal) { CurNormal = normal; }

    inline const float* GetCurTexCoord() const { return CurTexCoord; }
    inline void SetCurTexCoord(float u, float v)
    {
        CurTexCoord[0] = u;
        CurTexCoord[1] = v;
    }

    // we're not really supporting this
    void SetDoNormalize(bool normalize) { DoNormalize = normalize; }

    inline CVertArray& GetVertArray() { return *VertArray; }

    // user state

    virtual void EnableCustom(uint64_t flag)  = 0;
    virtual void DisableCustom(uint64_t flag) = 0;

    // rendering interface

    virtual void BeginGeom(GLenum mode)       = 0;
    virtual void Vertex(cpu_vec_xyzw newVert) = 0;
    virtual void Normal(cpu_vec_xyz normal)   = 0;
    virtual void TexCoord(float u, float v) = 0;
    virtual void Color(cpu_vec_xyzw color) = 0;
    virtual void EndGeom()                 = 0;
    virtual void DrawArrays(GLenum mode, int first, int count) = 0;
    virtual void DrawIndexedArrays(GLenum primType,
        int numIndices, const unsigned char* indices,
        int numVertices)
        = 0;
    virtual void Flush() = 0;
};

#endif // ps2gl_gmanager_h
