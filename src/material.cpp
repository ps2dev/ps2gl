/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2gl/material.h"
#include "ps2gl/dlist.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/lighting.h"

/********************************************
 * CImmMaterial
 */

CImmMaterial::CImmMaterial(CGLContext& context)
    : CMaterial(context)
    , Ambient(0.2f, 0.2f, 0.2f, 1.0f)
    , Diffuse(0.8f, 0.8f, 0.8f, 1.0f)
    , Specular(0.0f, 0.0f, 0.0f, 1.0f)
    , Emission(0.0f, 0.0f, 0.0f, 1.0f)
    , Shininess(0.0f)
{
}

void CImmMaterial::SetSpecular(cpu_vec_xyzw specular)
{
    TellRendererMaterialChanged();
    Specular = specular;

    if (Specular != cpu_vec_4(0, 0, 0, 0))
        GLContext.GetImmLighting().MaterialHasSpecular();
    else {
        GLContext.SpecularEnabledChanged();
        GLContext.GetImmGeomManager().GetRendererManager().SpecularEnabledChanged(false);
    }
}

void CImmMaterial::LightsHaveSpecular()
{
    if (Specular != cpu_vec_4(0, 0, 0, 0)) {
        GLContext.SpecularEnabledChanged();
        GLContext.GetImmGeomManager().GetRendererManager().SpecularEnabledChanged(true);
    }
}

/********************************************
 * CDListMaterial
 */

class CSetMaterialPropCmd : public CDListCmd {
    cpu_vec_xyzw Value;
    GLenum Property;

public:
    CSetMaterialPropCmd(GLenum prop, cpu_vec_xyzw value)
        : Value(value)
        , Property(prop)
    {
    }
    CDListCmd* Play()
    {
        glMaterialfv(GL_FRONT, Property, reinterpret_cast<float*>(&Value));
        return CDListCmd::GetNextCmd(this);
    }
};

void CDListMaterial::SetAmbient(cpu_vec_xyzw ambient)
{
    CDList& dlist = pGLContext->GetDListManager().GetOpenDList();
    dlist += CSetMaterialPropCmd(GL_AMBIENT, ambient);
    TellRendererMaterialChanged();
}

void CDListMaterial::SetDiffuse(cpu_vec_xyzw diffuse)
{
    CDList& dlist = pGLContext->GetDListManager().GetOpenDList();
    dlist += CSetMaterialPropCmd(GL_DIFFUSE, diffuse);
    TellRendererMaterialChanged();
}

void CDListMaterial::SetSpecular(cpu_vec_xyzw specular)
{
    CDList& dlist = pGLContext->GetDListManager().GetOpenDList();
    dlist += CSetMaterialPropCmd(GL_SPECULAR, specular);
    TellRendererMaterialChanged();
    GLContext.SpecularEnabledChanged(); // maybe
}

void CDListMaterial::SetEmission(cpu_vec_xyzw emission)
{
    CDList& dlist = pGLContext->GetDListManager().GetOpenDList();
    dlist += CSetMaterialPropCmd(GL_EMISSION, emission);
    TellRendererMaterialChanged();
}

void CDListMaterial::SetShininess(float shine)
{
    CDList& dlist = pGLContext->GetDListManager().GetOpenDList();
    dlist += CSetMaterialPropCmd(GL_SHININESS, cpu_vec_xyzw(shine, 0, 0, 0));
    TellRendererMaterialChanged();
}

/********************************************
 * CMaterialManager
 */

void CMaterialManager::Color(cpu_vec_xyzw color)
{
    CurColor = color;

    if (UseColorMaterial) {
        switch (ColorMaterialMode) {
        case GL_EMISSION:
            CurMaterial->SetEmission(color);
            break;
        case GL_AMBIENT:
            CurMaterial->SetAmbient(color);
            break;
        case GL_DIFFUSE:
            CurMaterial->SetDiffuse(color);
            break;
        case GL_SPECULAR:
            CurMaterial->SetSpecular(color);
            break;
        case GL_AMBIENT_AND_DIFFUSE:
            CurMaterial->SetAmbient(color);
            CurMaterial->SetDiffuse(color);
            break;
        }
    }
    GLContext.CurMaterialChanged();
}

class CSetUseColorMaterialCmd : public CDListCmd {
    bool UseCM;

public:
    CSetUseColorMaterialCmd(bool use)
        : UseCM(use)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetMaterialManager().SetUseColorMaterial(UseCM);
        return CDListCmd::GetNextCmd(this);
    }
};

void CMaterialManager::SetUseColorMaterial(bool yesNo)
{
    if (!InDListDef) {
        UseColorMaterial = yesNo;
        if (yesNo)
            Color(CurColor);
        GLContext.CurMaterialChanged();
    } else {
        CDList& dlist = GLContext.GetDListManager().GetOpenDList();
        dlist += CSetUseColorMaterialCmd(yesNo);
        GLContext.CurMaterialChanged();
    }
}

class CSetColorMatModeCmd : public CDListCmd {
    GLenum Mode;

public:
    CSetColorMatModeCmd(GLenum mode)
        : Mode(mode)
    {
    }
    CDListCmd* Play()
    {
        pGLContext->GetMaterialManager().SetColorMaterialMode(Mode);
        return CDListCmd::GetNextCmd(this);
    }
};

void CMaterialManager::SetColorMaterialMode(GLenum mode)
{
    if (!InDListDef) {
        ColorMaterialMode = mode;
        Color(CurColor);
        GLContext.CurMaterialChanged();
    } else {
        CDList& dlist = GLContext.GetDListManager().GetOpenDList();
        dlist += CSetColorMatModeCmd(mode);
        GLContext.CurMaterialChanged();
    }
}

/********************************************
 * gl interface
 */

void glMaterialfv(GLenum face, GLenum pname, const GLfloat* params)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    CMaterial& material = pGLContext->GetMaterialManager().GetCurMaterial();

    switch (face) {
    case GL_BACK:
        mNotImplemented("GL_BACK - only front material is available");
        break;
    case GL_FRONT_AND_BACK:
        mNotImplemented("GL_FRONT_AND_BACK - only front material is available");
    case GL_FRONT:
        switch (pname) {
        case GL_AMBIENT:
            material.SetAmbient(cpu_vec_xyzw(params[0], params[1], params[2], params[3]));
            break;
        case GL_DIFFUSE:
            material.SetDiffuse(cpu_vec_xyzw(params[0], params[1], params[2], params[3]));
            break;
        case GL_SPECULAR:
            material.SetSpecular(cpu_vec_xyzw(params[0], params[1], params[2], params[3]));
            break;
        case GL_EMISSION:
            material.SetEmission(cpu_vec_xyzw(params[0], params[1], params[2], params[3]));
            break;
        case GL_SHININESS:
            material.SetShininess(*params);
            break;
        case GL_AMBIENT_AND_DIFFUSE:
            material.SetAmbient(cpu_vec_xyzw(params[0], params[1], params[2], params[3]));
            material.SetDiffuse(cpu_vec_xyzw(params[0], params[1], params[2], params[3]));
            break;
        default:
            mError("shouldn't get here");
        }
        break;
    default:
        mError("shouldn't get here");
    }
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    CMaterial& material = pGLContext->GetMaterialManager().GetCurMaterial();

    switch (face) {
    case GL_BACK:
        mNotImplemented("GL_BACK - only front material is available");
        break;
    case GL_FRONT_AND_BACK:
        mNotImplemented("GL_FRONT_AND_BACK - only front material is available");
    case GL_FRONT:
        if (pname == GL_SHININESS)
            material.SetShininess(param);
        else {
            mError("shoudn't get here");
        }
        break;
    default:
        mError("shouldn't get here");
    }
}

void glColorMaterial(GLenum face, GLenum mode)
{
    GL_FUNC_DEBUG("%s\n", __FUNCTION__);

    pGLContext->GetMaterialManager().SetColorMaterialMode(mode);
}
