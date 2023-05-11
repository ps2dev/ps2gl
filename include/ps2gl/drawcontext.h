/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_drawcontext_h
#define ps2gl_drawcontext_h

/********************************************
 * includes
 */

#include "ps2s/cpu_matrix.h"

#include "ps2gl/glcontext.h"
#include "ps2gl/immgmanager.h"

namespace GS {
class CDrawEnv;
class CMemArea;
}

/********************************************
 * CDrawContext
 */

class CDrawContext {
protected:
    CGLContext& GLContext;

public:
    inline CDrawContext(CGLContext& context)
        : GLContext(context)
    {
    }
    virtual ~CDrawContext()
    {
    }

    virtual void SetDoSmoothShading(bool yesNo)            = 0;
    virtual void SetDoClipping(bool clip)                  = 0;
    virtual void SetDoCullFace(bool cull)                  = 0;
    virtual void SetCullFaceDir(int direction)             = 0;
    virtual void SetRescaleNormals(bool rescale)           = 0;
    virtual void SetBlendEnabled(bool enabled)             = 0;
    virtual void SetDepthWriteEnabled(bool enabled)        = 0;
    virtual void SetFrameBufferDrawMask(unsigned int mask) = 0;
    virtual void SetAlphaTestEnabled(bool enabled)         = 0;
    virtual void SetDepthTestEnabled(bool enabled)         = 0;
    virtual void SetInterlacingOffset(float yPixels)       = 0;
    virtual void SetPolygonMode(GLenum mode)               = 0;

    virtual void SetBlendMode(GLenum source, GLenum dest) = 0;
    virtual void SetAlphaFunc(GLenum func, GLclampf ref)  = 0;
    virtual void SetDepthFunc(GLenum func)                = 0;

    virtual void SetDrawBuffers(bool interlaced,
        GS::CMemArea* frame0Mem, GS::CMemArea* frame1Mem,
        GS::CMemArea* depthMem)
        = 0;
};

/********************************************
 * CImmDrawContext
 */

class CClearEnv;

class CImmDrawContext : public CDrawContext {
public:
    GS::CDrawEnv* DrawEnv;

    GS::CMemArea *Frame0Mem, *Frame1Mem, *ZBufMem;
    GS::CMemArea *CurFrameMem, *LastFrameMem;

    CClearEnv* ClearEnv;

    bool FrameIsDblBuffered;

    // gl state
    bool DoSmoothShading;
    bool DoClipping;
    bool DoCullFace;
    int CullFaceDir; // 1 or -1
    bool RescaleNormals;
    bool BlendIsEnabled;
    bool AlphaTestIsEnabled;
    bool DepthTestIsEnabled;
    bool DrawInterlaced;
    GLenum PolyMode;
    int DepthBits;

    // current vertex xform
    cpu_mat_44 VertexXform;
    cpu_mat_44 GSScale;
    bool IsVertexXformValid;

    int Width, Height;

public:
    CImmDrawContext(CGLContext& context);
    virtual ~CImmDrawContext();

    GS::CDrawEnv& GetDrawEnv() { return *DrawEnv; }
    void SwapBuffers(bool fieldIsEven);

    inline CClearEnv& GetClearEnv() { return *ClearEnv; }

    const cpu_mat_44& GetVertexXform();
    inline void SetVertexXformValid(bool valid)
    {
        IsVertexXformValid = valid;
        if (!valid)
            GLContext.XformChanged();
    }

    int GetFBWidth() const { return Width; }
    int GetFBHeight() const { return Height; }

    int GetDepthBits() const { return DepthBits; }
    void SetDepthBits(int depth) { DepthBits = depth; }

    // virtuals

    void SetBlendMode(GLenum source, GLenum dest);
    void SetAlphaFunc(GLenum func, GLclampf ref);
    void SetDepthFunc(GLenum func);

    inline bool GetDoSmoothShading() const { return DoSmoothShading; }
    void SetDoSmoothShading(bool yesNo);

    inline bool GetDoClipping() const { return DoClipping; }
    void SetDoClipping(bool clip);

    inline bool GetDoCullFace() const { return DoCullFace; }
    void SetDoCullFace(bool cull);

    inline int GetCullFaceDir() const { return CullFaceDir; }
    void SetCullFaceDir(int direction);

    inline bool GetBlendEnabled() const { return BlendIsEnabled; }
    void SetBlendEnabled(bool enabled);

    inline bool GetAlphaTestEnabled() const { return AlphaTestIsEnabled; }
    void SetAlphaTestEnabled(bool enabled);

    inline bool GetDepthTestEnabled() const { return DepthTestIsEnabled; }
    void SetDepthTestEnabled(bool enabled);

    void SetInterlacingOffset(float yPixels);

    void SetDepthWriteEnabled(bool enabled);
    void SetFrameBufferDrawMask(unsigned int mask);

    inline GLenum GetPolygonMode() const { return PolyMode; }
    void SetPolygonMode(GLenum mode);

    inline bool GetRescaleNormals() const { return RescaleNormals; }
    void SetRescaleNormals(bool rescale);

    void SetDrawBuffers(bool interlaced,
        GS::CMemArea* frame0Mem, GS::CMemArea* frame1Mem,
        GS::CMemArea* depthMem);
};

/********************************************
 * CDListDrawContext
 */

class CDListDrawContext : public CDrawContext {
public:
    CDListDrawContext(CGLContext& context)
        : CDrawContext(context)
    {
    }

    void SetBlendMode(GLenum source, GLenum dest);
    void SetAlphaFunc(GLenum func, GLclampf ref);
    void SetDepthFunc(GLenum func);

    void SetDoSmoothShading(bool yesNo);
    void SetDoClipping(bool clip);
    void SetDoCullFace(bool cull);
    void SetCullFaceDir(int direction);
    void SetRescaleNormals(bool rescale);
    void SetBlendEnabled(bool enabled);
    void SetAlphaTestEnabled(bool enabled);
    void SetDepthTestEnabled(bool enabled);
    void SetInterlacingOffset(float yPixels);
    void SetDepthWriteEnabled(bool enabled);
    void SetFrameBufferDrawMask(unsigned int mask);
    void SetPolygonMode(GLenum mode) { assert(false); }

    void SetDrawBuffers(bool interlaced,
        GS::CMemArea* frame0Mem, GS::CMemArea* frame1Mem,
        GS::CMemArea* depthMem);
};

#endif // ps2gl_drawcontext_h
