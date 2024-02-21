/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/drawenv.h"

#include "GL/ps2gl.h"

#include "ps2gl/clear.h"
#include "ps2gl/debug.h"
#include "ps2gl/dlgmanager.h"
#include "ps2gl/dlist.h"
#include "ps2gl/drawcontext.h"
#include "ps2gl/immgmanager.h"
#include "ps2gl/matrix.h"

/********************************************
 * CImmDrawContext methods
 */

CImmDrawContext::CImmDrawContext(CGLContext& context)
    : CDrawContext(context)
    , DrawEnv(NULL)
    , ClearEnv(NULL)
    , FrameIsDblBuffered(false)
    , DoSmoothShading(true)
    , DoClipping(true)
    , DoCullFace(false)
    , CullFaceDir(1)
    , RescaleNormals(false)
    , BlendIsEnabled(false)
    , AlphaTestIsEnabled(false)
    , DepthTestIsEnabled(false)
    , DrawInterlaced(true)
    , PolyMode(GL_FILL)
    , IsVertexXformValid(false)
    , Width(0)
    , Height(0)
{
    GSScale.set_identity();

    ClearEnv = new CClearEnv;

    DrawEnv = new GS::CDrawEnv(GS::kContext1);
    DrawEnv->SetDepthTestPassMode(GS::ZTest::kGEqual);
    using namespace GS::ABlend;
    DrawEnv->SetAlphaBlendFunc(kSourceRGB, kDestRGB, kSourceAlpha, kDestRGB, 0x80);
    DrawEnv->SetFogColor(255, 255, 255);

    // alpha test
    DrawEnv->DisableAlphaTest();
    DrawEnv->SetAlphaRefVal(0);
    DrawEnv->SetAlphaTestPassMode(GS::ATest::kAlways);
}

CImmDrawContext::~CImmDrawContext()
{
    // don't delete the frame mem areas -- they are created/destroyed by
    // the app

    delete ClearEnv;
    delete DrawEnv;
}

void CImmDrawContext::SetDrawBuffers(bool interlaced,
    GS::CMemArea* frame0Mem, GS::CMemArea* frame1Mem,
    GS::CMemArea* depthMem)
{
    Frame0Mem = frame0Mem;
    Frame1Mem = frame1Mem;
    ZBufMem   = depthMem;

    DrawInterlaced     = interlaced;
    FrameIsDblBuffered = (frame0Mem && frame1Mem);

    // see displaycontext for a comment on this..
    CurFrameMem  = Frame0Mem;
    LastFrameMem = Frame1Mem;

    int width = frame0Mem->GetWidth(), height = frame0Mem->GetHeight();
    Width  = width;
    Height = height;

    // get max depth buffer value

    int depthPsm = GS::kPsmz24;
    if (depthMem)
        depthPsm = depthMem->GetPixFormat();
    DepthBits    = 0;
    if (depthPsm == GS::kPsmz24)
        DepthBits = 24;
    else if (depthPsm == GS::kPsmz32)
        DepthBits = 28; // for fog
    else if (depthPsm == GS::kPsmz16)
        DepthBits = 16;
    else {
        mError("Unknown depth buffer format");
    }
    int maxDepthValue = (1 << DepthBits) - 1;

    // projection xform

    // -1 here flips mapping to: far_clip -> -1, near_clip -> 1
    GSScale.set_scale(cpu_vec_xyz(width / 2.0f, -1 * height / 2.0f, -1 * (float)maxDepthValue / 2.0f));
    SetVertexXformValid(false);

    // clear environment

    ClearEnv->SetDimensions(width, height);
    ClearEnv->SetFrameBufAddr(frame0Mem->GetWordAddr());
    ClearEnv->SetFrameBufPsm(frame0Mem->GetPixFormat());
    if (ZBufMem) {
        ClearEnv->SetDepthBufAddr(ZBufMem->GetWordAddr());
        ClearEnv->SetDepthBufPsm(ZBufMem->GetPixFormat());
    }

    // draw environment

    DrawEnv->SetFrameBufferAddr(CurFrameMem->GetWordAddr());
    DrawEnv->SetFrameBufferDim(width, height);
    DrawEnv->CalculateClippedFBXYOffsets(GS::kDontAddHalfPixel);
    DrawEnv->SetFrameBufferPSM(frame0Mem->GetPixFormat());

    // DrawEnv->SetScissorArea( 0, height * 0.25f, width, height * 0.25f );

    if (ZBufMem) {
        DrawEnv->EnableDepthTest();
        DrawEnv->SetDepthBufferAddr(ZBufMem->GetWordAddr());
        DrawEnv->SetDepthBufferPSM(ZBufMem->GetPixFormat());
    } else
        DrawEnv->DisableDepthTest();

    GLContext.DrawBufferChanged();
}

void CImmDrawContext::SwapBuffers(bool fieldIsEven)
{
    // flip frame buffer ptrs
    if (FrameIsDblBuffered) {
        GS::CMemArea* temp = CurFrameMem;
        CurFrameMem        = LastFrameMem;
        LastFrameMem       = temp;

        // clear the new frame
        ClearEnv->SetFrameBufAddr(CurFrameMem->GetWordAddr());

        // draw to the new frame
        DrawEnv->SetFrameBufferAddr(CurFrameMem->GetWordAddr());

        GLContext.DrawEnvChanged();
    }

    if (DrawInterlaced) {
        // add a half-pixel offset if the frame we're going to build on the core will
        // be displayed in an odd field
        DrawEnv->CalculateClippedFBXYOffsets(!fieldIsEven);

        GLContext.DrawEnvChanged();
    }
}

const cpu_mat_44&
CImmDrawContext::GetVertexXform()
{
    if (!IsVertexXformValid) {
        IsVertexXformValid = true;
        VertexXform        = (GLContext.GetProjectionStack().GetTop()
            * GLContext.GetModelViewStack().GetTop());
        VertexXform = GSScale * VertexXform;
    }

    return VertexXform;
}

void CImmDrawContext::SetDoSmoothShading(bool yesNo)
{
    if (DoSmoothShading != yesNo) {
        DoSmoothShading = yesNo;
        GLContext.ShadingChanged();
    }
}

void CImmDrawContext::SetDoClipping(bool clip)
{
    if (DoClipping != clip) {
        DoClipping = clip;
        GLContext.ClippingEnabledChanged();
        GLContext.GetImmGeomManager().GetRendererManager().ClippingEnabledChanged(clip);
    }
}

void CImmDrawContext::SetDoCullFace(bool cull)
{
    if (DoCullFace != cull) {
        DoCullFace = cull;
        GLContext.CullFaceEnabledChanged();
        GLContext.GetImmGeomManager().GetRendererManager().CullFaceEnabledChanged(cull);
    }
}

void CImmDrawContext::SetCullFaceDir(int direction)
{
    if (CullFaceDir != direction) {
        CullFaceDir = direction;
        GLContext.CullFaceDirChanged();
    }
}

void CImmDrawContext::SetBlendEnabled(bool enabled)
{
    if (BlendIsEnabled != enabled) {
        BlendIsEnabled = enabled;
        GLContext.BlendEnabledChanged();
    }
}

void CImmDrawContext::SetRescaleNormals(bool rescale)
{
    if (RescaleNormals != rescale) {
        RescaleNormals = rescale;
        GLContext.LightPropChanged();
    }
}

void CImmDrawContext::SetDepthWriteEnabled(bool enabled)
{
    DrawEnv->SetDepthWriteEnabled(enabled);
    GLContext.DepthWriteEnabledChanged();
}

void CImmDrawContext::SetFrameBufferDrawMask(unsigned int mask)
{
    DrawEnv->SetFrameBufferDrawMask(mask);
    GLContext.FrameBufferDrawMaskChanged();
}

void CImmDrawContext::SetPolygonMode(GLenum mode)
{
    if (PolyMode != mode) {
        PolyMode = mode;
        GLContext.PolyModeChanged();
    }
}

void CImmDrawContext::SetAlphaTestEnabled(bool enabled)
{
    if (AlphaTestIsEnabled != enabled) {
        AlphaTestIsEnabled = enabled;

        if (enabled) {
            DrawEnv->EnableAlphaTest();
        } else {
            DrawEnv->DisableAlphaTest();
        }

        GLContext.AlphaTestEnabledChanged();
    }
}

void CImmDrawContext::SetDepthTestEnabled(bool enabled)
{
    if (DepthTestIsEnabled != enabled) {
        DepthTestIsEnabled = enabled;

        if (enabled) {
            DrawEnv->EnableDepthTest();
        } else {
            DrawEnv->DisableDepthTest();
        }

        GLContext.DepthTestEnabledChanged();
    }
}

void CImmDrawContext::SetInterlacingOffset(float yPixels)
{
    if (DrawEnv->GetInterlacedPixelOffset() != yPixels) {
        DrawEnv->SetInterlacedPixelOffset(yPixels);
        bool offset = (DrawInterlaced && GLContext.GetCurrentFieldIsEven());
        DrawEnv->CalculateClippedFBXYOffsets(offset);

        GLContext.DrawEnvChanged();
    }
}

// I hate to do it, but...
#define mCombineBlendFactors(_src, _dest) \
    ((unsigned int)(_src) << 16) | (unsigned int)(_dest)

void CImmDrawContext::SetBlendMode(GLenum source, GLenum dest)
{
    unsigned int blendFactor = mCombineBlendFactors(source, dest);

    switch (blendFactor) {
    case mCombineBlendFactors(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA):
        using namespace GS::ABlend;
        DrawEnv->SetAlphaBlendFunc(kSourceRGB, kDestRGB, kSourceAlpha, kDestRGB, 0x80);
        GLContext.BlendModeChanged();
        break;
    case mCombineBlendFactors(GL_SRC_ALPHA, GL_ONE):
        DrawEnv->SetAlphaBlendFunc(kSourceRGB, kZero, kSourceAlpha, kDestRGB, 0x80);
        GLContext.BlendModeChanged();
        break;
    // the following is actually subtractive blending, which
    // should be GL_MINUS_ALPHA, GL_ONE but there is no minus alpha in GL
    case mCombineBlendFactors(GL_ONE_MINUS_SRC_ALPHA, GL_ONE):
        DrawEnv->SetAlphaBlendFunc(kZero, kSourceRGB, kSourceAlpha, kDestRGB, 0x80);
        GLContext.BlendModeChanged();
        break;
    default:
        mNotImplemented("alpha blending mode: source = %d, dest = %d", source, dest);
    }
}

#undef mCombineBlendFactors

void CImmDrawContext::SetAlphaFunc(GLenum func, GLclampf ref)
{
    GS::tAlphaTestPassMode ePassMode;

    switch (func) {
    case GL_NEVER:
        ePassMode = GS::ATest::kNever;
        break;

    case GL_LESS:
        ePassMode = GS::ATest::kLess;
        break;

    case GL_EQUAL:
        ePassMode = GS::ATest::kEqual;
        break;

    case GL_LEQUAL:
        ePassMode = GS::ATest::kLEqual;
        break;

    case GL_GREATER:
        ePassMode = GS::ATest::kGreater;
        break;

    case GL_NOTEQUAL:
        ePassMode = GS::ATest::kNotEqual;
        break;

    case GL_GEQUAL:
        ePassMode = GS::ATest::kGEqual;
        break;

    case GL_ALWAYS:
        ePassMode = GS::ATest::kAlways;
        break;

    default:
        mError("Unknown alpha test function");
        return;
    }

    DrawEnv->SetAlphaRefVal((unsigned int)(ref * 0xff));
    DrawEnv->SetAlphaTestPassMode(ePassMode);
    DrawEnv->SetAlphaTestFailAction(GS::ATest::kKeep);

    GLContext.AlphaTestFuncChanged();
}

void CImmDrawContext::SetDepthFunc(GLenum func)
{
    /*
     * NOTE:
     *   The PS2 does not support GL_LESS/GL_LEQUAL
     *   but this is what 99% of OpenGL programs use.
     *
     *   As a result depth is inverted.
     *   See glDepthFunc/glFrustum/glOrtho
     */
    GS::tZTestPassMode ePassMode;

    switch (func) {
    case GL_NEVER:
        ePassMode = GS::ZTest::kNever;
        break;

    case GL_LESS:
        ePassMode = GS::ZTest::kGreater;
        break;

    case GL_LEQUAL:
        ePassMode = GS::ZTest::kGEqual;
        break;

    case GL_ALWAYS:
        ePassMode = GS::ZTest::kAlways;
        break;

    case GL_EQUAL:
    case GL_GREATER:
    case GL_NOTEQUAL:
    case GL_GEQUAL:
    default:
        mError("Unknown/unsupported depth test function");
        return;
    }

    DrawEnv->SetDepthTestPassMode(ePassMode);

    GLContext.DepthTestFuncChanged();
}

/********************************************
 * CDListDrawContext methods
 */

class CSetDrawBuffers : public CDListCmd {
    GS::CMemArea *Frame0, *Frame1, *Depth;
    bool Interlaced;

public:
    CSetDrawBuffers(bool inter, GS::CMemArea* frame0, GS::CMemArea* frame1, GS::CMemArea* depth)
        : Frame0(frame0)
        , Frame1(frame1)
        , Depth(depth)
        , Interlaced(inter)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetDrawBuffers(Interlaced, Frame0, Frame1, Depth);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetDrawBuffers(bool interlaced,
    GS::CMemArea* frame0Mem, GS::CMemArea* frame1Mem,
    GS::CMemArea* depthMem)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetDrawBuffers(interlaced,
        frame0Mem, frame1Mem,
        depthMem);
    GLContext.DrawBufferChanged();
}

class CSetDoSmoothShadingCmd : public CDListCmd {
    bool DoSS;

public:
    CSetDoSmoothShadingCmd(bool yesNo)
        : DoSS(yesNo)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetDoSmoothShading(DoSS);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetDoSmoothShading(bool yesNo)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetDoSmoothShadingCmd(yesNo);
    GLContext.ShadingChanged();
}

class CSetDoClippingCmd : public CDListCmd {
    bool DoClip;

public:
    CSetDoClippingCmd(bool clip)
        : DoClip(clip)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetDoClipping(DoClip);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetDoClipping(bool clip)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetDoClippingCmd(clip);
    GLContext.ClippingEnabledChanged();
}

class CSetDoCullFaceCmd : public CDListCmd {
    bool DoCull;

public:
    CSetDoCullFaceCmd(bool cull)
        : DoCull(cull)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetDoCullFace(DoCull);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetDoCullFace(bool cull)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetDoCullFaceCmd(cull);
    GLContext.CullFaceEnabledChanged();
}

class CSetCullFaceDir : public CDListCmd {
    int CullDir;

public:
    CSetCullFaceDir(int dir)
        : CullDir(dir)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetCullFaceDir(CullDir);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetCullFaceDir(int direction)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetCullFaceDir(direction);
    GLContext.CullFaceDirChanged();
}

class CSetBlendEnabledCmd : public CDListCmd {
    bool Enabled;

public:
    CSetBlendEnabledCmd(bool enabled)
        : Enabled(enabled)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetBlendEnabled(Enabled);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetBlendEnabled(bool enabled)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetBlendEnabledCmd(enabled);
    GLContext.BlendEnabledChanged();
}

class CSetAlphaTestEnabledCmd : public CDListCmd {
    bool Enabled;

public:
    CSetAlphaTestEnabledCmd(bool enabled)
        : Enabled(enabled)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetAlphaTestEnabled(Enabled);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetAlphaTestEnabled(bool enabled)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetAlphaTestEnabledCmd(enabled);
    GLContext.AlphaTestEnabledChanged();
}

class CSetDepthTestEnabledCmd : public CDListCmd {
    bool Enabled;

public:
    CSetDepthTestEnabledCmd(bool enabled)
        : Enabled(enabled)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetDepthTestEnabled(Enabled);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetDepthTestEnabled(bool enabled)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetDepthTestEnabledCmd(enabled);
    GLContext.DepthTestEnabledChanged();
}

class CSetInterlacingOffsetCmd : public CDListCmd {
    float Offset;

public:
    CSetInterlacingOffsetCmd(float offset)
        : Offset(offset)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetInterlacingOffset(Offset);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetInterlacingOffset(float yPixels)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetInterlacingOffsetCmd(yPixels);
    GLContext.DrawEnvChanged();
}

class CSetAlphaFuncCmd : public CDListCmd {
    GLenum Func;
    GLclampf Ref;

public:
    CSetAlphaFuncCmd(GLenum func, GLclampf ref)
        : Func(func)
        , Ref(ref)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetAlphaFunc(Func, Ref);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetAlphaFunc(GLenum func, GLclampf ref)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetAlphaFuncCmd(func, ref);
    GLContext.AlphaTestFuncChanged();
}

class CSetDepthFuncCmd : public CDListCmd {
    GLenum Func;

public:
    CSetDepthFuncCmd(GLenum func)
        : Func(func)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetDepthFunc(Func);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetDepthFunc(GLenum func)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetDepthFuncCmd(func);
    GLContext.DepthTestFuncChanged();
}

class CSetRescaleNormalsCmd : public CDListCmd {
    bool Rescale;

public:
    CSetRescaleNormalsCmd(bool rescale)
        : Rescale(rescale)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetDrawContext().SetRescaleNormals(Rescale);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetRescaleNormals(bool rescale)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetRescaleNormalsCmd(rescale);
    GLContext.LightPropChanged();
}

class CSetBlendMode : public CDListCmd {
    GLenum Source, Dest;

public:
    CSetBlendMode(GLenum s, GLenum d)
        : Source(s)
        , Dest(d)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetBlendMode(Source, Dest);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetBlendMode(GLenum source, GLenum dest)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetBlendMode(source, dest);
    GLContext.BlendModeChanged();
}

class CSetDepthWriteEnabledCmd : public CDListCmd {
    bool Enabled;

public:
    CSetDepthWriteEnabledCmd(bool enabled)
        : Enabled(enabled)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetDepthWriteEnabled(Enabled);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetDepthWriteEnabled(bool enabled)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetDepthWriteEnabledCmd(enabled);
    GLContext.DepthWriteEnabledChanged();
}

class CSetFrameBufferDrawMaskCmd : public CDListCmd {
    unsigned int Mask;

public:
    CSetFrameBufferDrawMaskCmd(unsigned int mask)
        : Mask(mask)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetImmDrawContext().SetFrameBufferDrawMask(Mask);
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListDrawContext::SetFrameBufferDrawMask(unsigned int mask)
{
    GLContext.GetDListGeomManager().Flush();

    GLContext.GetDListManager().GetOpenDList() += CSetFrameBufferDrawMaskCmd(mask);
    GLContext.FrameBufferDrawMaskChanged();
}

/********************************************
 * gl api
 */

void glDepthFunc(GLenum func)
{
    GL_FUNC_DEBUG("%s(0x%x)\n", __FUNCTION__, func);

    pGLContext->GetDrawContext().SetDepthFunc(func);
}

void glDrawBuffer(GLenum mode)
{
    GL_FUNC_DEBUG("%s(0x%x)\n", __FUNCTION__, mode);

    mNotImplemented();
}

void glReadBuffer(GLenum mode)
{
    GL_FUNC_DEBUG("%s(0x%x)\n", __FUNCTION__, mode);

    mNotImplemented();
}

void glClipPlane(GLenum plane, const GLdouble* equation)
{
    GL_FUNC_DEBUG("%s(0x%x,%f)\n", __FUNCTION__, plane, equation);

    mNotImplemented();
}

// alpha blending

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    GL_FUNC_DEBUG("%s(0x%x,0x%x)\n", __FUNCTION__, sfactor, dfactor);

    pGLContext->GetDrawContext().SetBlendMode(sfactor, dfactor);
}

// alpha test

void glAlphaFunc(GLenum func, GLclampf ref)
{
    GL_FUNC_DEBUG("%s(0x%x,%f)\n", __FUNCTION__, func, ref);

    pGLContext->GetDrawContext().SetAlphaFunc(func, ref);
}

// depth test

void glDepthMask(GLboolean enabled)
{
    GL_FUNC_DEBUG("%s(%d)\n", __FUNCTION__, enabled);

    CImmDrawContext& drawContext = pGLContext->GetImmDrawContext();
    drawContext.SetDepthWriteEnabled(enabled);
}

void glShadeModel(GLenum mode)
{
    GL_FUNC_DEBUG("%s(0x%x)\n", __FUNCTION__, mode);

    CImmDrawContext& drawContext = pGLContext->GetImmDrawContext();
    drawContext.SetDoSmoothShading((mode == GL_FLAT) ? false : true);
}

void glCullFace(GLenum mode)
{
    GL_FUNC_DEBUG("%s(0x%x)\n", __FUNCTION__, mode);

    mWarnIf(mode == GL_FRONT_AND_BACK, "GL_FRONT_AND_BACK culling is not supported");
    CImmDrawContext& drawContext = pGLContext->GetImmDrawContext();
    drawContext.SetCullFaceDir((mode == GL_FRONT) ? 1 : -1);
}

void glColorMask(GLboolean r_enabled, GLboolean g_enabled,
    GLboolean b_enabled, GLboolean a_enabled)
{
    GL_FUNC_DEBUG("%s(%d,%d,%d,%d)\n", __FUNCTION__, r_enabled, g_enabled, b_enabled, a_enabled);

    CImmDrawContext& drawContext = pGLContext->GetImmDrawContext();
    unsigned int mask            = 0;
    if (r_enabled == GL_FALSE)
        mask |= 0xff;
    if (g_enabled == GL_FALSE)
        mask |= 0xff00;
    if (b_enabled == GL_FALSE)
        mask |= 0xff0000;
    if (a_enabled == GL_FALSE)
        mask |= 0xff000000;
    drawContext.SetFrameBufferDrawMask(mask);
}

void glScissor(int x, int y, int width, int height)
{
    GL_FUNC_DEBUG("%s(%d,%d,%d,%d)\n", __FUNCTION__, x, y, width, height);

    mNotImplemented();
}

void glClearAccum(float red, float green, float blue, float alpha)
{
    GL_FUNC_DEBUG("%s(%f,%f,%f,%f)\n", __FUNCTION__, red, green, blue, alpha);

    mNotImplemented();
}

void glClearStencil(int s)
{
    GL_FUNC_DEBUG("%s(%d)\n", __FUNCTION__, s);

    mNotImplemented();
}

void glPolygonMode(GLenum face, GLenum mode)
{
    GL_FUNC_DEBUG("%s(0x%x,0x%x)\n", __FUNCTION__, face, mode);

    CImmDrawContext& drawContext = pGLContext->GetImmDrawContext();
    drawContext.SetPolygonMode(mode);
}

void glPolygonOffset(GLfloat factor, GLfloat units)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    mNotImplemented();
}

void glGetPolygonStipple(GLubyte* mask)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    mNotImplemented();
}

/********************************************
 * ps2gl api
 */

/**
 * @addtogroup pgl_api
 * @{
 */

/**
 * Set the area(s) in gs mem to draw.  If two frame buffers are given they will
 * be used as double buffers.
 * @param interlaced PGL_INTERLACED or PGL_NONINTERLACED
 * @param frame0_mem the first or only buffer
 * @param frame1_mem NULL if single-buffered
 * @param depth_mem the depth buffer; NULL for none
 */
void pglSetDrawBuffers(int interlaced,
    pgl_area_handle_t frame0_mem, pgl_area_handle_t frame1_mem,
    pgl_area_handle_t depth_mem)
{
    pGLContext->GetDrawContext().SetDrawBuffers(interlaced,
        reinterpret_cast<GS::CMemArea*>(frame0_mem),
        reinterpret_cast<GS::CMemArea*>(frame1_mem),
        reinterpret_cast<GS::CMemArea*>(depth_mem));
}

void pglSetInterlacingOffset(float yPixels)
{
    pGLContext->GetDrawContext().SetInterlacingOffset(yPixels);
}

/** @} */ // pgl_api
