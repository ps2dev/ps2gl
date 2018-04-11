/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "GL/ps2gl.h"

#include "ps2gl/displaycontext.h"
#include "ps2gl/dlist.h"

#include "ps2s/displayenv.h"

CDisplayContext::CDisplayContext(CGLContext& context)
    : GLContext(context)
    , Frame0Mem(NULL)
    , Frame1Mem(NULL)
    , CurFrameMem(NULL)
    , LastFrameMem(NULL)
    , DisplayEnv(NULL)
    , DisplayIsDblBuffered(true)
    , DisplayIsInterlaced(true)
{
    DisplayEnv = new GS::CDisplayEnv;
}

CDisplayContext::~CDisplayContext()
{
    // don't delete the frame mem areas -- they are created/destroyed by
    // the app

    delete DisplayEnv;
}

void CDisplayContext::SetDisplayBuffers(bool interlaced,
    GS::CMemArea* frame0Mem, GS::CMemArea* frame1Mem)
{
    Frame0Mem = frame0Mem;
    Frame1Mem = frame1Mem;

    DisplayIsDblBuffered = (frame0Mem && frame1Mem);
    DisplayIsInterlaced  = interlaced;

    // "current" means the frame being drawn to by the loop of code executing on the core
    // "last" will be the frame being displayed if drawing immediately, or the frame not
    // displayed now if building up a packet to be sent next frame.
    CurFrameMem  = Frame0Mem;
    LastFrameMem = Frame1Mem;

    int width = frame0Mem->GetWidth(), height = frame0Mem->GetHeight();
    int displayHeight = (DisplayIsInterlaced) ? height * 2 : height;

    DisplayEnv->SetFB2(frame0Mem->GetWordAddr(), width, 0, 0, frame0Mem->GetPixFormat());
    DisplayEnv->SetDisplay2(width, displayHeight);
    DisplayEnv->SendSettings();
}

void CDisplayContext::SwapBuffers()
{
    // flip frame buffer ptrs
    if (DisplayIsDblBuffered) {
        GS::CMemArea* temp = CurFrameMem;
        CurFrameMem        = LastFrameMem;
        LastFrameMem       = temp;

        // display the last completed frame (which is frame n-2 because we're not
        // drawing immediately but building up a packet)
        // remember this is immediately sent, not delayed through a packet
        DisplayEnv->SetFB2Addr(CurFrameMem->GetWordAddr());
        DisplayEnv->SendSettings();
    }
}

/********************************************
 * ps2gl C interface
 */

/**
 * @addtogroup pgl_api
 * @{
 */

/**
 * Tell ps2gl what areas in GS ram to display.
 * @param interlaced PGL_INTERLACED or PGL_NONINTERLACED
 * @param frame0_mem the first area if double-buffered, otherwise the only area
 * @param frame1_mem the second area if double-buffered, otherwise NULL
 */
void pglSetDisplayBuffers(int interlaced, pgl_area_handle_t frame0_mem, pgl_area_handle_t frame1_mem)
{
    pGLContext->GetDisplayContext().SetDisplayBuffers(interlaced,
        reinterpret_cast<GS::CMemArea*>(frame0_mem),
        reinterpret_cast<GS::CMemArea*>(frame1_mem));
}

/** @} */ // pgl_api

/********************************************
 * gl api
 */

void glPixelStorei(GLenum pname, int param)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    mNotImplemented();
}

void glReadPixels(int x, int y, int width, int height,
    GLenum format, GLenum type, void* pixels)
{
    GL_FUNC_DEBUG("%s(%d,%d,%d,%d,...)\n", __FUNCTION__, x, y, width, height);

    mNotImplemented();
}

void glViewport(GLint x, GLint y,
    GLsizei width, GLsizei height)
{
    GL_FUNC_DEBUG("%s(%d,%d,%d,%d)\n", __FUNCTION__, x, y, width, height);

    mNotImplemented();
}
