#ifndef ps2gl_fixed_function_h
#define ps2gl_fixed_function_h

#pragma once
#include <string.h>
#include <stdint.h>
#ifndef GL_DIFFUSE
    #define GL_DIFFUSE 0x1201
#endif
// typedef unsigned int GLenum; //TODO???
#include <GL/gl.h>
#include "ps2gl/gmanager.h"
#include "ps2gl/gblock.h"
#include "ps2gl/renderermanager.h"
#include "ps2gl/glcontext.h"

typedef enum {
    FIXED_FUNCTION_ATTR_NONE = 0,
    FIXED_FUNCTION_ATTR_CONSTANT,
    FIXED_FUNCTION_ATTR_ARRAY
} FixedFunctionDataSrc;

typedef enum {
    FIXED_FUNCTION_COLOR_CONSTANT = 0,   // (currentColor * tint), lighting OFF
    FIXED_FUNCTION_COLOR_ARRAY,          // (vertexColor * tint),   lighting OFF
    FIXED_FUNCTION_COLOR_LIT             // lighting ON
} FixedFunctionColor; //TODO I get that this is for emphasis but it seems ugly idk

typedef struct {
    bool texture2dEnabled;
    bool lightingEnabled;
    bool colorMaterialEnabled;
    GLenum colorMaterialMode; // expected: GL_DIFFUSE or 0

    bool vertexArrayEnabled;
    bool normalArrayEnabled;
    bool texcoordArrayEnabled;
    bool colorArrayEnabled;

    bool diffuseTextureBound;   // a valid diffuse map is actually bound

    float currentColor[4];      // current GL color
    float currentNormal[3];     // current GL normal (rarely used in this policy)
    float currentTexCoord[2];   // current GL texcoord (we do not rely on this)

    // App-level tint (raylib tint, or 1,1,1,1 if already folded into currentColor)
    float tintRgba[4];

    bool immediateColorVariesInPrimitive;
} FixedFunctionConditions; //TODO: should we merge this with state somehow?

typedef struct {
    FixedFunctionDataSrc vertexSrc;     // always ARRAY
    FixedFunctionDataSrc normalSrc;     // ARRAY or NONE
    FixedFunctionDataSrc texcoordSrc;   // ARRAY or NONE
    FixedFunctionDataSrc colorSrc;      // CONSTANT or ARRAY (ARRAY also means “varies per vertex” in immediate mode)

    FixedFunctionColor  ffColor;

    bool V, N, T, C;

    bool textureFlag;
    bool lightingFlag;
    bool colorMaterialAffectsDiffuse; // true if per-vertex color should drive diffuse when lighting is on
} FixedFunctionState;

typedef enum {
    QW_NONE  = 0x0,  // ----
    QW_X     = 0x1,  // X---
    QW_XY    = 0x3,  // XY--
    QW_XYZ   = 0x7,  // XYZ-
    QW_XYZW  = 0xF   // XYZW
} QuadWords;

typedef struct {
    QuadWords vertices;   // legal: QW_XYZ or QW_XYZW // TODO: remove (qw == QW_XYZ) I THINK!!
    QuadWords normals;    // legal: QW_NONE or QW_XYZ
    QuadWords texcoords;  // legal: QW_NONE or QW_XY
    QuadWords colors;     // legal: QW_NONE or QW_XYZW
} LaneConfig;

static inline int verticesOk(QuadWords  qw) { return (qw == QW_XYZ)  || (qw == QW_XYZW); } // TODO: remove (qw == QW_XYZ)
static inline int normalsOk(QuadWords   qw) { return (qw == QW_NONE) || (qw == QW_XYZ);  }
static inline int texcoordsOk(QuadWords qw) { return (qw == QW_NONE) || (qw == QW_XY);   }
static inline int colorsOk(QuadWords    qw) { return (qw == QW_NONE) || (qw == QW_XYZW); }

static inline int ValidateLaneConfig(const LaneConfig* lanes, const char* where) {
    if (!verticesOk(lanes->vertices) || !normalsOk(lanes->normals) || !texcoordsOk(lanes->texcoords) || !colorsOk(lanes->colors)) {
        mError("%s: illegal lane masks (V=%x N=%x T=%x C=%x)", where, lanes->vertices, lanes->normals, lanes->texcoords, lanes->colors);
        return 0;
    }
    return 1;
}

static inline int QWToWords(QuadWords qw) {
    switch (qw) {
    case QW_NONE: return 0;
    case QW_X:    return 1;  //TODO: just for brevity
    case QW_XY:   return 2;
    case QW_XYZ:  return 3;
    case QW_XYZW: return 4;
    default:      return 0;
    }
}

static inline int LanePresent(QuadWords qw) { return (qw != QW_NONE); }

static inline FixedFunctionState evaluate(const FixedFunctionConditions* conditions)
{
    FixedFunctionState state;
    memset(&state, 0, sizeof(state));

    // V: always array & lane present
    state.vertexSrc = FIXED_FUNCTION_ATTR_ARRAY;
    state.V = true;

    // T: requires GL texture enable, a real diffuse bound, AND a texcoord array
    state.textureFlag   = (conditions->texture2dEnabled && conditions->diffuseTextureBound && conditions->texcoordArrayEnabled);
    state.texcoordSrc = state.textureFlag ? FIXED_FUNCTION_ATTR_ARRAY : FIXED_FUNCTION_ATTR_NONE;
    state.T          = (state.texcoordSrc == FIXED_FUNCTION_ATTR_ARRAY);

    // N: lighting is only meaningful if we have a normal array
    const bool lightingFeasible = (conditions->lightingEnabled && conditions->normalArrayEnabled);
    state.lightingFlag  = lightingFeasible;
    state.normalSrc = state.lightingFlag ? FIXED_FUNCTION_ATTR_ARRAY : FIXED_FUNCTION_ATTR_NONE;
    state.N        = (state.normalSrc == FIXED_FUNCTION_ATTR_ARRAY);

    // C: color path & lane selection
    const bool perVertexColorSupplyPresent = conditions->colorArrayEnabled || conditions->immediateColorVariesInPrimitive;

    if (!state.lightingFlag) {
        if (perVertexColorSupplyPresent) {
            state.colorSrc = FIXED_FUNCTION_ATTR_ARRAY;
            state.ffColor   = FIXED_FUNCTION_COLOR_ARRAY;   // (v.color * tint)
            state.C       = true;                              // need per-vertex color lane
        } else {
            state.colorSrc = FIXED_FUNCTION_ATTR_CONSTANT;
            state.ffColor   = FIXED_FUNCTION_COLOR_CONSTANT;// (currentColor * tint)
            state.C       = false;                             // constant → no C lane
        }
    } else {
        // Lighting ON: final color is computed. PVC only matters for material mapping.
        state.colorSrc = perVertexColorSupplyPresent ? FIXED_FUNCTION_ATTR_ARRAY : FIXED_FUNCTION_ATTR_CONSTANT;
        state.ffColor   = FIXED_FUNCTION_COLOR_LIT;
        state.C       = false;
    }

    // ColorMaterial routing (lighting must be ON, color material enabled and mode=DIFFUSE,
    // and there must be a per-vertex supply to make it meaningful)
    state.colorMaterialAffectsDiffuse =
        (state.lightingFlag &&
         conditions->colorMaterialEnabled &&
         conditions->colorMaterialMode == GL_DIFFUSE &&
         perVertexColorSupplyPresent);

    return state;
}

static inline void capture(
    FixedFunctionConditions* conditions,
    CGLContext& glContext,
    const CVertArray& vertArray,
    bool diffuseTextureIsBound,              // see §3 for how to feed this
    const float tintRgba[4],                 // see §3 for how to feed this
    bool immediateColorVariesInPrimitive)    // pass true in EndGeom() if colors changed mid-primitive
{
    memset(conditions, 0, sizeof(*conditions));

    conditions->texture2dEnabled     = glContext.IsTextureEnabled();
    conditions->lightingEnabled      = glContext.IsLightingEnabled();
    conditions->colorMaterialEnabled = glContext.IsColorMaterialEnabled();
    conditions->colorMaterialMode    = glContext.GetColorMaterialMode();

    // Arrays
    conditions->vertexArrayEnabled   = vertArray.GetVerticesAreValid();
    conditions->normalArrayEnabled   = vertArray.GetNormalsAreValid();
    conditions->texcoordArrayEnabled = vertArray.GetTexCoordsAreValid();
    conditions->colorArrayEnabled    = vertArray.GetColorsAreValid() && vertArray.GetWordsPerColor() == 4;

    conditions->diffuseTextureBound  = diffuseTextureIsBound;

    //cpu_vec_xyzw current = glContext.GetGeomManager().GetCurGeomColor();
    cpu_vec_xyzw currentColor = glContext.GetCurrentGeomColor();
    conditions->currentColor[0] = currentColor[0];
    conditions->currentColor[1] = currentColor[1];
    conditions->currentColor[2] = currentColor[2];
    conditions->currentColor[3] = currentColor[3];

    //TODO WHAT? WHY WOULDNT WE JUST GET FROM THE actual defaults? (I ADDED BELOW AND FOR TEXCOORd/...
    // conditions->currentNormal[0] = 0.0f;
    // conditions->currentNormal[1] = 0.0f;
    // conditions->currentNormal[2] = 1.0f;
    cpu_vec_xyz currentNormal = glContext.GetCurrentNormal();
    conditions->currentNormal[0] = currentNormal[0];
    conditions->currentNormal[1] = currentNormal[1];
    conditions->currentNormal[2] = currentNormal[2];

    const float* currentTexCoord = glContext.GetCurrentTexCoord();
    conditions->currentTexCoord[0] = currentTexCoord[0];
    conditions->currentTexCoord[1] = currentTexCoord[1];

    memcpy(conditions->tintRgba, tintRgba, sizeof(float)*4);
    conditions->immediateColorVariesInPrimitive = immediateColorVariesInPrimitive;
}

static inline void apply(
    const FixedFunctionState& state,
    CGeometryBlock& geometry,
    CRendererManager& rendererManager,
    CGLContext& glContext) //TODO what would we use glContext here for again???
{
    //TODO: can we integrate the QuadWord stuff and validation here? would it be helpful? i would like to have at least those enums rather than the shitty ints here that are ambigious...
    geometry.SetWordsPerVertex(4);
    geometry.SetWordsPerNormal(state.N ? 3 : 0);
    geometry.SetWordsPerTexCoord(state.T ? 2 : 0);
    geometry.SetWordsPerColor(state.C ? 4 : 0);

    geometry.SetVerticesAreValid(true);
    geometry.SetNormalsAreValid(state.N);
    geometry.SetTexCoordsAreValid(state.T);
    geometry.SetColorsAreValid(state.C);
    //TODO: still this i am ughhhhhhhhhhhhhhhh
    // Hook ColorMaterial (diffuse) mapping in lit path
    // This mirrors what your SyncColorMaterial used to do but is now explicit and single-sourced.
    if (state.colorMaterialAffectsDiffuse) {
        rendererManager.PerVtxMaterialChanged(RendererProps::kDiffuse);
    } else {
        rendererManager.PerVtxMaterialChanged(RendererProps::kNoMaterial);
    }

    // TODO: do something here? to strictly align renderer choice with *effective* lighting,
    //  you can keep using GLContext as-is (works functionally), or go further:
    //   - add an "effective lighting" override the renderers consult.
    //  OTHERWISE DO NOTHING???? WHAT???
}

#endif // ps2gl_fixed_function_h
