/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_material_h
#define ps2gl_material_h

#include "GL/gl.h"

#include "ps2s/cpu_vector.h"

#include "ps2gl/debug.h"
#include "ps2gl/dlgmanager.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/immgmanager.h"

class CGLContext;

class CMaterial {
protected:
    CGLContext& GLContext;

public:
    CMaterial(CGLContext& context)
        : GLContext(context)
    {
    }

    virtual void SetAmbient(cpu_vec_xyzw ambient)   = 0;
    virtual void SetDiffuse(cpu_vec_xyzw diffuse)   = 0;
    virtual void SetSpecular(cpu_vec_xyzw specular) = 0;
    virtual void SetEmission(cpu_vec_xyzw emission) = 0;
    virtual void SetShininess(float shine)          = 0;
};

class CImmMaterial : public CMaterial {
    cpu_vec_xyzw Ambient, Diffuse, Specular, Emission;
    float Shininess;

    inline void TellRendererMaterialChanged()
    {
        GLContext.CurMaterialChanged();
    }

public:
    CImmMaterial(CGLContext& context);

    void SetAmbient(cpu_vec_xyzw ambient)
    {
        Ambient = ambient;
        TellRendererMaterialChanged();
    }
    void SetDiffuse(cpu_vec_xyzw diffuse)
    {
        Diffuse = diffuse;
        TellRendererMaterialChanged();
    }
    void SetSpecular(cpu_vec_xyzw specular);
    void SetEmission(cpu_vec_xyzw emission)
    {
        Emission = emission;
        TellRendererMaterialChanged();
    }
    void SetShininess(float shine)
    {
        Shininess = shine;
        TellRendererMaterialChanged();
    }

    inline cpu_vec_xyzw GetAmbient() const { return Ambient; }
    inline cpu_vec_xyzw GetDiffuse() const { return Diffuse; }
    inline cpu_vec_xyzw GetSpecular() const { return Specular; }
    inline cpu_vec_xyzw GetEmission() const { return Emission; }
    inline float GetShininess() const { return Shininess; }

    void LightsHaveSpecular();
};

class CDListMaterial : public CMaterial {
    inline void TellRendererMaterialChanged()
    {
        GLContext.CurMaterialChanged();
    }

public:
    CDListMaterial(CGLContext& context)
        : CMaterial(context)
    {
    }

    void SetAmbient(cpu_vec_xyzw ambient);
    void SetDiffuse(cpu_vec_xyzw diffuse);
    void SetSpecular(cpu_vec_xyzw specular);
    void SetEmission(cpu_vec_xyzw emission);
    void SetShininess(float shine);
};

class CMaterialManager {
    CGLContext& GLContext;

    CImmMaterial ImmMaterial;
    CDListMaterial DListMaterial;
    CMaterial* CurMaterial;

    cpu_vec_xyzw CurColor;
    GLenum ColorMaterialMode;
    bool UseColorMaterial;
    bool InDListDef;

public:
    CMaterialManager(CGLContext& context)
        : GLContext(context)
        , ImmMaterial(context)
        , DListMaterial(context)
        , CurMaterial(&ImmMaterial)
        , CurColor(1, 1, 1, 1)
        , ColorMaterialMode(GL_AMBIENT_AND_DIFFUSE)
        , UseColorMaterial(false)
        , InDListDef(false)
    {
        ImmMaterial.SetDiffuse(cpu_vec_xyzw(0.8f, 0.8f, 0.8f, 1.0f));
    }

    CMaterial& GetCurMaterial() { return *CurMaterial; }
    CImmMaterial& GetImmMaterial() { return ImmMaterial; }
    CDListMaterial& GetDListMaterial() { return DListMaterial; }
    cpu_vec_xyzw GetCurColor() const { return CurColor; }
    GLenum GetColorMaterialMode() const { return ColorMaterialMode; }
    bool GetColorMaterialEnabled() const { return UseColorMaterial; }

    void Color(cpu_vec_xyzw color);
    void SetUseColorMaterial(bool yesNo);
    void SetColorMaterialMode(GLenum mode);

    void BeginDListDef()
    {
        CurMaterial = &DListMaterial;
        InDListDef  = true;
    }
    void EndDListDef()
    {
        CurMaterial = &ImmMaterial;
        InDListDef  = false;
    }
};

#endif // ps2gl_material_h
