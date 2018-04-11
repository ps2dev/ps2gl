/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2gl/texture.h"

#include "GL/ps2gl.h"

#include "ps2gl/debug.h"
#include "ps2gl/dlgmanager.h"
#include "ps2gl/dlist.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/immgmanager.h"
#include "ps2gl/metrics.h"

#include "kernel.h"

/********************************************
 * CTexManager
 */

CTexManager::CTexManager(CGLContext& context)
    : GLContext(context)
    , IsTexEnabled(false)
    , InsideDListDef(false)
    , Cursor(0)
    , LastTexSent(NULL)
    , CurClut(NULL)
    , TexMode(GS::TexMode::kModulate)
{
    // clear the texture name entries
    for (int i      = 0; i < NumTexNames; i++)
        TexNames[i] = NULL;

    // create the default texture
    DefaultTex = new CMMTexture(GS::kContext1);
    CurTexture = DefaultTex;
}

CTexManager::~CTexManager()
{
    delete DefaultTex;
    if (CurClut)
        delete CurClut;

    for (int i = 0; i < NumTexNames; i++) {
        if (TexNames[i])
            delete TexNames[i];
    }
}

class CSetTexEnabledCmd : public CDListCmd {
    bool IsEnabled;

public:
    CSetTexEnabledCmd(bool enabled)
        : IsEnabled(enabled)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetTexManager().SetTexEnabled(IsEnabled);
        return CDListCmd::GetNextCmd(this);
    }
};

void CTexManager::SetTexEnabled(bool yesNo)
{
    if (!InsideDListDef) {
        if (IsTexEnabled != yesNo) {
            GLContext.TexEnabledChanged();
            GLContext.GetImmGeomManager().GetRendererManager().TexEnabledChanged(yesNo);
            IsTexEnabled = yesNo;
        }
    } else {
        CDList& dlist = GLContext.GetDListManager().GetOpenDList();
        dlist += CSetTexEnabledCmd(yesNo);
        GLContext.TexEnabledChanged();
    }
}

class CSetTexModeCmd : public CDListCmd {
    GS::tTexMode Mode;

public:
    CSetTexModeCmd(GS::tTexMode mode)
        : Mode(mode)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetTexManager().SetTexMode(Mode);
        return CDListCmd::GetNextCmd(this);
    }
};

void CTexManager::SetTexMode(GS::tTexMode mode)
{
    if (!InsideDListDef)
        TexMode = mode;
    else {
        CDList& dlist = GLContext.GetDListManager().GetOpenDList();
        dlist += CSetTexModeCmd(mode);
    }
}

void CTexManager::GenTextures(GLsizei numNewTexNames, GLuint* newTexNames)
{
    for (int curTexName = 0; curTexName < numNewTexNames; curTexName++) {
        // find the next free tex name and assign it
        int i;
        for (i = 0; i < NumTexNames; i++) {
            // 0 is a reserved tex name in OGL -- don't alloc it
            if (Cursor != 0 && TexNames[Cursor] == NULL)
                break;

            IncCursor();
        }
        // did we go through all the names without finding any free ones?
        if (i == NumTexNames) {
            mError("No free texture names.  Time to write a less braindead tex manager.");

            // In release build, return sensible names on failure
            while (i < NumTexNames) {
                newTexNames[i] = 0;
                ++i;
            }
        } else {
            newTexNames[curTexName] = Cursor;
            IncCursor();
        }
    }
}

void CTexManager::UseCurTexture(CVifSCDmaPacket& renderPacket)
{
    if (IsTexEnabled) {
        GS::tPSM psm = CurTexture->GetPSM();
        // do we need to send the clut?
        if (psm == GS::kPsm8 || psm == GS::kPsm8h) {
            mErrorIf(CurClut == NULL,
                "Trying to use an indexed-color texture with no color table given!");
            CurClut->Load(renderPacket);
        }
        // use the texture
        CurTexture->SetTexMode(TexMode);
        if (CurTexture->GetPSM() == GS::kPsm8
            || CurTexture->GetPSM() == GS::kPsm8h)
            CurTexture->SetClut(*CurClut);
        CurTexture->Use(renderPacket);
    }
}

class CBindTextureCmd : public CDListCmd {
    unsigned int TexName;

public:
    CBindTextureCmd(unsigned int name)
        : TexName(name)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetTexManager().BindTexture(TexName);
        return CDListCmd::GetNextCmd(this);
    }
};

void CTexManager::BindTexture(GLuint texNameToBind)
{
    GLContext.TextureChanged();

    if (!InsideDListDef) {
        if (texNameToBind == 0) {
            // default texture
            CurTexture = DefaultTex;
        } else {
            if (TexNames[texNameToBind] == NULL)
                TexNames[texNameToBind] = new CMMTexture(GS::kContext1);
            CurTexture                  = TexNames[texNameToBind];
        }

        pglAddToMetric(kMetricsBindTexture);
    } else {
        CDList& dlist = GLContext.GetDListManager().GetOpenDList();
        dlist += CBindTextureCmd(texNameToBind);
    }
}

void CTexManager::DeleteTextures(GLsizei numToDelete, const GLuint* texNames)
{
    for (int i = 0; i < numToDelete; i++) {
        mErrorIf(TexNames[texNames[i]] == NULL,
            "Trying to delete a texture that doesn't exist!");
        GLuint texName = texNames[i];
        if (CurTexture == TexNames[texName])
            CurTexture = DefaultTex;
        delete TexNames[texName];
        TexNames[texName] = NULL;
    }
}

class CSetCurTexParamCmd : public CDListCmd {
    GLenum PName;
    int Param;

public:
    CSetCurTexParamCmd(GLenum pname, int param)
        : PName(pname)
        , Param(param)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetTexManager().SetCurTexParam(PName, Param);
        return CDListCmd::GetNextCmd(this);
    }
};

void CTexManager::SetCurTexParam(GLenum pname, GLint param)
{
    if (!InsideDListDef) {
        CMMTexture& tex = *CurTexture;

        switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
            tex.SetMinMode((GS::tMinMode)(param & 0xf));
            break;
        case GL_TEXTURE_MAG_FILTER:
            tex.SetMagMode((GS::tMagMode)(param & 0xf));
            break;
        case GL_TEXTURE_MIN_LOD:
        case GL_TEXTURE_MAX_LOD:
        case GL_TEXTURE_BASE_LEVEL:
        case GL_TEXTURE_MAX_LEVEL:
        case GL_TEXTURE_PRIORITY:
        case GL_TEXTURE_BORDER_COLOR:
            mNotImplemented();
            break;
        case GL_TEXTURE_WRAP_S:
            tex.SetWrapModeS((GS::tTexWrapMode)(param & 0xf));
            break;
        case GL_TEXTURE_WRAP_T:
            tex.SetWrapModeT((GS::tTexWrapMode)(param & 0xf));
            break;
        case GL_TEXTURE_WRAP_R:
            mNotImplemented("Sorry, only 2d textures.");
            break;
        }
    } else {
        CDList& dlist = GLContext.GetDListManager().GetOpenDList();
        dlist += CSetCurTexParamCmd(pname, param);
    }
}

class CSetCurTexImageCmd : public CDListCmd {
    tU128* Image;
    unsigned int Width, Height;
    GS::tPSM Psm;

public:
    CSetCurTexImageCmd(tU128* image, unsigned int w, unsigned int h,
        GS::tPSM psm)
        : Image(image)
        , Width(w)
        , Height(h)
        , Psm(psm)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetTexManager().SetCurTexImage(Image, Width, Height, Psm);
        return CDListCmd::GetNextCmd(this);
    }
};

void CTexManager::SetCurTexImage(tU128* imagePtr, tU32 w, tU32 h,
    GS::tPSM psm)
{
    GLContext.TextureChanged();

    if (!InsideDListDef) {
        CurTexture->SetImage(imagePtr, w, h, psm);
        if (psm != GS::kPsm24)
            CurTexture->SetUseTexAlpha(true);
        else
            CurTexture->SetUseTexAlpha(false);

        CurTexture->Free();
    } else {
        CDList& dlist = GLContext.GetDListManager().GetOpenDList();
        dlist += CSetCurTexImageCmd(imagePtr, w, h, psm);
    }
}

class CSetCurClutCmd : public CDListCmd {
    const void* Clut;
    int NumEntries;

public:
    CSetCurClutCmd(const void* clut, int numEntries)
        : Clut(clut)
        , NumEntries(numEntries)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetTexManager().SetCurClut(Clut, NumEntries);
        return CDListCmd::GetNextCmd(this);
    }
};

void CTexManager::SetCurClut(const void* clut, int numEntries)
{
    GLContext.TextureChanged();

    if (!InsideDListDef) {
        CMMClut* temp = CurClut;
        CurClut       = new CMMClut(clut);
        if (temp)
            delete temp;
    } else {
        CDList& dlist = GLContext.GetDListManager().GetOpenDList();
        dlist += CSetCurClutCmd(clut, numEntries);
    }
}

class CSetGsTextureCmd : public CDListCmd {
    GS::CMemArea& Texture;

public:
    CSetGsTextureCmd(GS::CMemArea& tex)
        : Texture(tex)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetTexManager().SetGsTexture(Texture);
        return CDListCmd::GetNextCmd(this);
    }
};

void CTexManager::SetGsTexture(GS::CMemArea& area)
{
    GLContext.TextureChanged();

    if (!InsideDListDef)
        CurTexture->SetImage(area);
    else {
        CDList& dlist = GLContext.GetDListManager().GetOpenDList();
        dlist += CSetGsTextureCmd(area);
    }
}

/********************************************
 * CMMTexture methods
 */

CMMTexture::CMMTexture(GS::tContext context)
    : CTexture(context)
    , pImageMem(NULL)
    , XferImage(false)
    , IsResident(false)
{
    // always load the clut
    // FIXME:  this is obviously not the best way to do
    // things...fix this once there is reasonable clut
    // allocation
    SetClutLoadConditions(1);
}

CMMTexture::~CMMTexture()
{
    delete pImageMem;
}

/**
 * Use the given image in main ram as the texture.
 */
void CMMTexture::SetImage(tU128* imagePtr, tU32 w, tU32 h, GS::tPSM psm)
{
    if (pImageMem) {
        // we are being re-initialized
        delete pImageMem;
        CTexture::Reset();
    }

    CTexture::SetImage(imagePtr, w, h, psm, NULL);

    // create a memarea for the image
    tU32 bufWidth = gsrTex0.tb_width * 64;
    pImageMem     = new GS::CMemArea(bufWidth, h, psm, GS::kAlignBlock);

    XferImage = true;
}

/**
 * Texture from the given gs memory area.  This means that no texture
 * will be uploaded; only the register settings will be sent to the gs.
 */
void CMMTexture::SetImage(const GS::CMemArea& area)
{
    CTexEnv::SetPSM(area.GetPixFormat());
    CTexEnv::SetDimensions(area.GetWidth(), area.GetHeight());
    SetImageGsAddr(area.GetWordAddr());

    XferImage = false;
}

void CMMTexture::ChangePsm(GS::tPSM psm)
{
    // we only want changes like 8 -> 8h, or 4hh -> 4, not 8 -> 32

    using namespace GS;

    // printf("changing (%d,%d) into ", GetPSM(), psm);

    if (GS::GetBitsPerPixel(psm) == GS::GetBitsPerPixel(GetPSM())) {
        // printf("%d\n", psm);
        CTexEnv::SetPSM(psm);
        pImageUploadPkt->ChangePsm(psm);
    } else {
        // on the other hand, don't put an 8h into a 32

        int bpp = GS::GetBitsPerPixel(GetPSM());
        if (bpp == 8 && GetPSM() == GS::kPsm8h) {
            // printf("%d\n", kPsm8 );
            ChangePsm(GS::kPsm8);
        } else if (bpp == 4 && GetPSM() != GS::kPsm4) {
            // printf("%d\n", kPsm4 );
            ChangePsm(GS::kPsm4);
        } else {
            // printf("<no change>\n");
        }
    }
}

void CMMTexture::Load(bool waitForEnd)
{
    mErrorIf(pImageMem == NULL,
        "Trying to load a texture that hasn't been defined!");
    // first set the gs address and flush the cache
    if (!pImageMem->IsAllocated()) {
        pImageMem->Alloc();
        if (GetPSM() != pImageMem->GetPixFormat())
            ChangePsm(pImageMem->GetPixFormat());
        SetImageGsAddr(pImageMem->GetWordAddr());
        IsResident = false;
    }
    if (!IsResident) {
        FlushCache(0);
        // send the image
        SendImage(waitForEnd, Packet::kDontFlushCache);
        IsResident = true;

        pglAddToMetric(kMetricsTextureUploadCount);
    }
}

// these two should be templates or something..

void CMMTexture::Load(CSCDmaPacket& packet)
{
    mErrorIf(pImageMem == NULL,
        "Trying to load a texture that hasn't been defined!");
    if (!pImageMem->IsAllocated()) {
        pImageMem->Alloc();
        if (GetPSM() != pImageMem->GetPixFormat())
            ChangePsm(pImageMem->GetPixFormat());
        SetImageGsAddr(pImageMem->GetWordAddr());
        IsResident = false;
    }
    if (!IsResident) {
        SendImage(packet);
        IsResident = true;

        pglAddToMetric(kMetricsTextureUploadCount);
    }
}
void CMMTexture::Load(CVifSCDmaPacket& packet)
{
    mErrorIf(pImageMem == NULL,
        "Trying to load a texture that hasn't been defined!");
    if (!pImageMem->IsAllocated()) {
        pImageMem->Alloc();
        if (GetPSM() != pImageMem->GetPixFormat())
            ChangePsm(pImageMem->GetPixFormat());
        SetImageGsAddr(pImageMem->GetWordAddr());
        IsResident = false;
    }
    if (!IsResident) {
        //        printf("allocing memarea of psm %d (%dx%d)...", pImageMem->GetPixFormat(),
        //  	     GetW(), GetH() );
        //        printf("at addr %d\n", pImageMem->GetWordAddr() /2048);
        SendImage(packet);
        IsResident = true;

        pglAddToMetric(kMetricsTextureUploadCount);
    }
}

// again, should be templates..

void CMMTexture::Use(bool waitForEnd)
{
    if (XferImage)
        Load();
    SendSettings(waitForEnd, Packet::kDontFlushCache);
}
void CMMTexture::Use(CSCDmaPacket& packet)
{
    if (XferImage)
        Load(packet);
    SendSettings(packet);
}
void CMMTexture::Use(CVifSCDmaPacket& packet)
{
    if (XferImage)
        Load(packet);
    SendSettings(packet);
}

void CMMTexture::Free(void)
{
    pImageMem->Free();
    IsResident = false;
}

void CMMTexture::BindToSlot(GS::CMemSlot& slot)
{
    //     slot.Bind(*pImageMem, 0);
    //     if ( GetPSM() != pImageMem->GetPixFormat() )
    //        ChangePsm(pImageMem->GetPixFormat());
    //     SetImageGsAddr( pImageMem->GetWordAddr() );
    //     IsResident = false;
}

/********************************************
 * CMMClut
 */

void CMMClut::Load(CVifSCDmaPacket& packet)
{
    if (!GsMem.IsAllocated()) {
        GsMem.Alloc();
        SetGsAddr(GsMem.GetWordAddr());
        Send(packet);

        pglAddToMetric(kMetricsClutUploadCount);
    }
}

/********************************************
 * gl api
 */

void glGenTextures(GLsizei numNewTexNames, GLuint* newTexNames)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    CTexManager& texManager = pGLContext->GetTexManager();
    texManager.GenTextures(numNewTexNames, newTexNames);
}

void glBindTexture(GLenum target, GLuint texName)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    mErrorIf(target != GL_TEXTURE_2D, "There are only 2D textures in ps2gl");

    CTexManager& texManager = pGLContext->GetTexManager();
    texManager.BindTexture(texName);
}

void glDeleteTextures(GLsizei numToDelete, const GLuint* texNames)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    CTexManager& texManager = pGLContext->GetTexManager();
    texManager.DeleteTextures(numToDelete, texNames);
}

void glTexImage2D(GLenum target,
    GLint level,
    GLint internalFormat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    const GLvoid* pixels)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    if (target == GL_PROXY_TEXTURE_2D) {
        mNotImplemented();
        return;
    }
    if (level > 0) {
        mNotImplemented("mipmapping");
        return;
    }
    if (border > 0) {
        mNotImplemented("texture borders");
        return;
    }
    if ((unsigned int)pixels & 0xf) {
        mNotImplemented("texture data needs to be aligned to at least 16 bytes, "
                        "preferably 9 quads");
    }

    GS::tPSM psm = GS::kInvalidPsm;
    switch (format) {
    case GL_RGBA:
        if (type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT_8_8_8_8)
            psm = GS::kPsm32;
        else if (type == GL_UNSIGNED_SHORT_5_5_5_1)
            psm = GS::kPsm16;
        else {
            mNotImplemented("RGBA textures should have a type of GL_UNSIGNED_BYTE, "
                            "GL_UNSIGNED_INT_8_8_8_8, or GL_UNSIGNED_SHORT_5_5_5_1");
        }
        break;
    case GL_RGB:
        if (type == GL_UNSIGNED_BYTE)
            psm = GS::kPsm24;
        else {
            mNotImplemented("RGB textures should have a type of GL_UNSIGNED_BYTE");
        }
        break;
    case GL_COLOR_INDEX:
        if (type == GL_UNSIGNED_BYTE)
            psm = GS::kPsm8;
        else {
            mNotImplemented("indexed textures should have a type of GL_UNSIGNED_BYTE");
        }
        break;
    default:
        mError("Unknown texture format");
    }

    if (psm != GS::kInvalidPsm) {
        CTexManager& tm = pGLContext->GetTexManager();
        tm.SetCurTexImage((tU128*)pixels, width, height, psm);
    }
}

void glColorTable(GLenum target, GLenum internalFormat,
    GLsizei width, GLenum format, GLenum type,
    const GLvoid* table)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    mWarnIf(target != GL_COLOR_TABLE,
        "glColorTable only supports GL_COLOR_TABLE");
    // ignore internalFormat
    mErrorIf(width != 16 && width != 256,
        "A color table must contain either 16 or 256 entries");
    mErrorIf(format != GL_RGB && format != GL_RGBA,
        "The pixel format of color tables must be either GL_RGB or GL_RGBA");
    mWarnIf(type != GL_UNSIGNED_INT && type != GL_UNSIGNED_INT_8_8_8_8,
        "The type of color table data must be either GL_UNSIGNED_INT or"
        "GL_UNSIGNED_INT_8_8_8_8");
    mErrorIf((unsigned int)table & (16 - 1),
        "Color tables in ps2gl need to be 16-byte aligned in memory..");

    CTexManager& tm = pGLContext->GetTexManager();
    tm.SetCurClut(table, width);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    if (target != GL_TEXTURE_2D) {
        mNotImplemented("Sorry, only 2d textures.");
        return;
    }

    pGLContext->GetTexManager().SetCurTexParam(pname, param);
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    glTexParameteri(target, pname, (GLint)param);
}

void glTexParameteriv(GLenum target, GLenum pname, GLint* param)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    glTexParameteri(target, pname, *param);
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* param)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    glTexParameteri(target, pname, (GLint)*param);
}

void glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    CTexManager& tm = pGLContext->GetTexManager();

    switch (param) {
    case GL_MODULATE:
        tm.SetTexMode((GS::tTexMode)(param & 0xf));
        break;
    case GL_DECAL:
        mWarn("GL_DECAL functions exactly as GL_REPLACE right now.");
    case GL_REPLACE:
        tm.SetTexMode((GS::tTexMode)(param & 0xf));
        break;
    case GL_BLEND:
        mNotImplemented();
        break;
    }
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    glTexEnvi(target, pname, (GLint)param);
}

void glTexEnvfv(GLenum target, GLenum pname, GLfloat* param)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    glTexEnvi(target, pname, (GLint)*param);
}

void glTexEnviv(GLenum target, GLenum pname, GLint* param)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    glTexEnvi(target, pname, *param);
}

void glTexSubImage2D(GLenum target, int level,
    int xoffset, int yoffset, int width, int height,
    GLenum format, GLenum type,
    const void* pixels)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    mNotImplemented();
}

void glCopyTexImage2D(GLenum target, int level,
    GLenum iformat,
    int x, int y, int width, int height,
    int border)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    mNotImplemented();
}

void glCopyTexSubImage2D(GLenum target, int level,
    int xoffset, int yoffset, int x, int y,
    int width, int height)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    mNotImplemented();
}

/********************************************
 * ps2gl C interface
 */

/**
 * @addtogroup pgl_api
 * @{
 */

/**
 * Free the named GL texture object.  Note that this only frees
 * any GS ram the texture is using, not main ram.
 */
void pglFreeTexture(GLuint texId)
{
    CTexManager& texManager = pGLContext->GetTexManager();
    CMMTexture& texture     = texManager.GetNamedTexture(texId);
    texture.Free();
}

/**
 * Bind the named GL texture object to the given GS memory slot.
 * This functions allows the application to bypass the GS memory
 * manager.
 */
void pglBindTextureToSlot(GLuint texId, pgl_slot_handle_t mem_slot)
{
    CTexManager& texManager = pGLContext->GetTexManager();
    CMMTexture& texture     = texManager.GetNamedTexture(texId);
    texture.BindToSlot(*reinterpret_cast<GS::CMemSlot*>(mem_slot));
}

/**
 * Texture from the given memory area.  Used in the same context as
 * glTexImage2D(), this call would probably be used with procedural
 * textures.
 */
void pglTextureFromGsMemArea(pgl_area_handle_t tex_area_handle)
{
    CTexManager& texManager = pGLContext->GetTexManager();
    GS::CMemArea* texArea   = reinterpret_cast<GS::CMemArea*>(tex_area_handle);
    texManager.SetGsTexture(*texArea);
}

/** @} */
