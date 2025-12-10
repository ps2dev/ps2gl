#ifndef ps2gl_fixed_function_h
#define ps2gl_fixed_function_h

//TODO: this is half composed, i need to really justify the existinance of this if going this course...
// it may purely be something that would "help" me, and thus i am uncertain if its truly justified to rewrite all the logic.
// This will be the biggest merge conflict once i perhaps integrate it.
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
    FIXED_FUNCTION_COLOR_CONSTANT = 0,
    FIXED_FUNCTION_COLOR_ARRAY,
    FIXED_FUNCTION_COLOR_LIT
} FixedFunctionColor; //TODO I get that this is for emphasis but it seems ugly idk

typedef struct {
    bool texture2dEnabled;
    bool lightingEnabled;
    bool colorMaterialEnabled;
    GLenum colorMaterialMode;

    bool vertexArrayEnabled;
    bool normalArrayEnabled;
    bool texcoordArrayEnabled;
    bool colorArrayEnabled;

    bool diffuseTextureBound;

    float currentColor[4];
    float currentNormal[3];
    float currentTexCoord[2];

    float tintRgba[4];

    bool immediateColorVariesInPrimitive;
} FixedFunctionConditions; //TODO: should we merge this with state somehow?

typedef struct {
    FixedFunctionDataSrc vertexSrc;
    FixedFunctionDataSrc normalSrc;
    FixedFunctionDataSrc texcoordSrc;
    FixedFunctionDataSrc colorSrc;

    FixedFunctionColor  ffColor;

    bool V, N, T, C;

    bool textureFlag;
    bool lightingFlag;
    bool colorMaterialAffectsDiffuse;
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

    state.vertexSrc = FIXED_FUNCTION_ATTR_ARRAY;
    state.V = true;

    state.textureFlag   = (conditions->texture2dEnabled && conditions->diffuseTextureBound && conditions->texcoordArrayEnabled);
    state.texcoordSrc = state.textureFlag ? FIXED_FUNCTION_ATTR_ARRAY : FIXED_FUNCTION_ATTR_NONE;
    state.T          = (state.texcoordSrc == FIXED_FUNCTION_ATTR_ARRAY);

    const bool lightingFeasible = (conditions->lightingEnabled && conditions->normalArrayEnabled);
    state.lightingFlag  = lightingFeasible;
    state.normalSrc = state.lightingFlag ? FIXED_FUNCTION_ATTR_ARRAY : FIXED_FUNCTION_ATTR_NONE;
    state.N        = (state.normalSrc == FIXED_FUNCTION_ATTR_ARRAY);

    const bool perVertexColorSupplyPresent = conditions->colorArrayEnabled || conditions->immediateColorVariesInPrimitive;

    if (!state.lightingFlag) {
        if (perVertexColorSupplyPresent) {
            state.colorSrc = FIXED_FUNCTION_ATTR_ARRAY;
            state.ffColor   = FIXED_FUNCTION_COLOR_ARRAY;
            state.C       = true;
        } else {
            state.colorSrc = FIXED_FUNCTION_ATTR_CONSTANT;
            state.ffColor   = FIXED_FUNCTION_COLOR_CONSTANT;
            state.C       = false;
        }
    } else {
        state.colorSrc = perVertexColorSupplyPresent ? FIXED_FUNCTION_ATTR_ARRAY : FIXED_FUNCTION_ATTR_CONSTANT;
        state.ffColor   = FIXED_FUNCTION_COLOR_LIT;
        state.C       = false;
    }

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
    bool diffuseTextureIsBound,
    const float tintRgba[4],
    bool immediateColorVariesInPrimitive)
{
    memset(conditions, 0, sizeof(*conditions));

    conditions->texture2dEnabled     = glContext.IsTextureEnabled();
    conditions->lightingEnabled      = glContext.IsLightingEnabled();
    conditions->colorMaterialEnabled = glContext.IsColorMaterialEnabled();
    conditions->colorMaterialMode    = glContext.GetColorMaterialMode();

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
    if (state.colorMaterialAffectsDiffuse) {
        rendererManager.PerVtxMaterialChanged(RendererProps::kDiffuse);
    } else {
        rendererManager.PerVtxMaterialChanged(RendererProps::kNoMaterial);
    }

}

#endif // ps2gl_fixed_function_h
