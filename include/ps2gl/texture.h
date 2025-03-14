/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_texture_h
#define ps2gl_texture_h

#include "ps2s/gsmem.h"
#include "ps2s/texture.h"

#include "GL/gl.h"

/********************************************
 * CTexManager
 */

class CGLContext;
class CMMTexture;
class CMMClut;
class CVifSCDmaPacket;

class CTexManager {
    CGLContext& GLContext;

    bool IsTexEnabled;
    bool InsideDListDef;

    static const int NumTexNames = 512; // :TODO: Make configurable
    CMMTexture* TexNames[NumTexNames];
    unsigned int Cursor;

    CMMTexture *DefaultTex, *CurTexture, *LastTexSent;
    CMMClut* CurClut;
    GS::tTexMode TexMode;

    void IncCursor() { Cursor = (Cursor + 1) & (NumTexNames - 1); }

public:
    CTexManager(CGLContext& context);
    ~CTexManager();

    void SetTexEnabled(bool yesNo);
    bool GetTexEnabled() const { return IsTexEnabled; }

    void GenTextures(GLsizei numNewTexNames, GLuint* newTexNames);
    void BindTexture(GLuint texNameToBind);
    void DeleteTextures(GLsizei numToDelete, const GLuint* texNames);

    CMMTexture& GetCurTexture() const { return *CurTexture; }

    CMMTexture& GetNamedTexture(GLuint tex) const
    {
        mErrorIf(TexNames[tex] == NULL, "Trying to access a null texture");
        return *TexNames[tex];
    }

    void UseCurTexture(CVifSCDmaPacket& renderPacket);

    void SetTexMode(GS::tTexMode mode);

    void SetCurTexParam(GLenum pname, GLint param);
    void SetCurTexImage(uint128_t* imagePtr, uint32_t w, uint32_t h,
        GS::tPSM psm);
    void SetGsTexture(GS::CMemArea& area);
    void SetCurClut(const void* clut, int numEntries);

    void BeginDListDef() { InsideDListDef = true; }
    void EndDListDef() { InsideDListDef = false; }
};

/********************************************
 * CMMClut
 */

/**
 * A memory-managed color lookup table.
 */
class CMMClut : public GS::CClut {
    GS::CMemArea GsMem;

public:
    CMMClut(const void* table, int numEntries = 256)
        : GS::CClut(table, numEntries)
        , GsMem(16, 16, GS::kPsm32, GS::kAlignPage)
    {
    }

    ~CMMClut() {}

    void Load(CVifSCDmaPacket& packet);
};

/********************************************
 * CMMTexture (Mem Managed Texture)
 */

namespace GS {
class CMemArea;
}

class CMMTexture : public GS::CTexture {
    GS::CMemArea* pImageMem;
    bool XferImage;
    bool IsResident;

public:
    CMMTexture(GS::tContext context);
    ~CMMTexture();

    void SetImage(const GS::CMemArea& area);
    void SetImage(uint128_t* imagePtr, uint32_t w, uint32_t h, GS::tPSM psm, uint32_t* clutPtr = NULL);

    void SetClut(const CMMClut& clut)
    {
        CTexEnv::SetClutGsAddr(clut.GetGsAddr());
    }

    void ChangePsm(GS::tPSM psm);

    // the following Load methods will check to see if a texture is resident and
    // transfer it if necessary.  The Use methods invoke the corresponding Load but
    // also transfer the gs register settings that belong to the texture

    // warning! these two will flush the data cache!
    void Load(bool waitForEnd = true);
    void Use(bool waitForEnd = false);

    void Load(CSCDmaPacket& packet);
    void Load(CVifSCDmaPacket& packet);

    void Use(CSCDmaPacket& packet);
    void Use(CVifSCDmaPacket& packet);

    void BindToSlot(GS::CMemSlot& slot);
    void Free(void);
};

#endif // ps2gl_texture_h
