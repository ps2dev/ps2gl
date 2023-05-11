/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_lighting_h
#define ps2gl_lighting_h

#include "GL/gl.h"

#include "ps2s/cpu_vector.h"

#include "ps2gl/debug.h"
#include "ps2gl/dlgmanager.h"
#include "ps2gl/glcontext.h"
#include "ps2gl/immgmanager.h"

class CGLContext;

/********************************************
 * CLight
 */

class CLight {
protected:
    CGLContext& GLContext;
    int LightNum;

public:
    CLight(CGLContext& context, int lightNum)
        : GLContext(context)
        , LightNum(lightNum)
    {
    }

    virtual void SetAmbient(cpu_vec_xyzw ambient)     = 0;
    virtual void SetDiffuse(cpu_vec_xyzw diffuse)     = 0;
    virtual void SetSpecular(cpu_vec_xyzw specular)   = 0;
    virtual void SetPosition(cpu_vec_xyzw position)   = 0;
    virtual void SetDirection(cpu_vec_xyzw direction) = 0;
    virtual void SetSpotDirection(cpu_vec_xyzw dir)   = 0;
    virtual void SetSpotCutoff(float cutoff)          = 0;
    virtual void SetSpotExponent(float exp)           = 0;
    virtual void SetConstantAtten(float atten)        = 0;
    virtual void SetLinearAtten(float atten)          = 0;
    virtual void SetQuadAtten(float atten)            = 0;

    virtual void SetEnabled(bool yesNo) = 0;
};

/********************************************
 * CImmLight
 */

class CImmLight : public CLight {
    cpu_vec_xyzw Ambient, Diffuse, Specular;
    cpu_vec_xyzw Position, SpotDirection;
    float SpotCutoff, SpotExponent;
    float ConstantAtten, LinearAtten, QuadAtten;
    bool bIsEnabled;

    // tLightType is defined in rendervsm.h
    tLightType Type;

    static int NumLights[3];

    inline void TellRendererLightPropChanged()
    {
        GLContext.LightPropChanged();
    }

    void CheckTypeChange(tLightType oldType);

public:
    CImmLight(CGLContext& context, int lightNum);

    void SetAmbient(cpu_vec_xyzw ambient)
    {
        Ambient = ambient;
        TellRendererLightPropChanged();
    }
    void SetDiffuse(cpu_vec_xyzw diffuse)
    {
        Diffuse = diffuse;
        TellRendererLightPropChanged();
    }
    void SetSpecular(cpu_vec_xyzw specular);
    void SetPosition(cpu_vec_xyzw position);
    void SetDirection(cpu_vec_xyzw direction);

    void SetSpotDirection(cpu_vec_xyzw dir)
    {
        SpotDirection = dir;
        TellRendererLightPropChanged();
    }
    void SetSpotCutoff(float cutoff)
    {
        tLightType oldType = Type;
        if (Type != kDirectional)
            Type = (cutoff == 180.0f) ? kPoint : kSpot;
        CheckTypeChange(oldType);
        SpotCutoff = cutoff;
        TellRendererLightPropChanged();
    }
    void SetSpotExponent(float exp)
    {
        SpotExponent = exp;
        TellRendererLightPropChanged();
    }

    void SetConstantAtten(float atten)
    {
        ConstantAtten = atten;
        TellRendererLightPropChanged();
    }
    void SetLinearAtten(float atten)
    {
        LinearAtten = atten;
        TellRendererLightPropChanged();
    }
    void SetQuadAtten(float atten)
    {
        QuadAtten = atten;
        TellRendererLightPropChanged();
    }

    void SetEnabled(bool enabled);

    inline cpu_vec_xyzw GetAmbient() const { return Ambient; }
    inline cpu_vec_xyzw GetDiffuse() const { return Diffuse; }
    inline cpu_vec_xyzw GetSpecular() const { return Specular; }
    inline cpu_vec_xyzw GetPosition() const { return Position; }

    inline cpu_vec_xyzw GetSpotDir() const { return SpotDirection; }
    inline float GetSpotCutoff() const { return SpotCutoff; }
    inline float GetSpotExponent() const { return SpotExponent; }

    inline float GetConstantAtten() const { return ConstantAtten; }
    inline float GetLinearAtten() const { return LinearAtten; }
    inline float GetQuadAtten() const { return QuadAtten; }

    inline bool IsEnabled() const { return bIsEnabled; }
    inline bool IsDirectional() const { return (Type == kDirectional); }
    inline bool IsPoint() const { return (Type == kPoint); }
    inline bool IsSpot() const { return (Type == kSpot); }
};

/********************************************
 * CDListLight
 */

class CDListLight : public CLight {
    inline void TellRendererLightPropChanged()
    {
        GLContext.LightPropChanged();
    }
    inline void TellRendererLightsEnabledChanged()
    {
        GLContext.NumLightsChanged();
    }

public:
    CDListLight(CGLContext& context, int lightNum)
        : CLight(context, lightNum)
    {
    }

    void SetAmbient(cpu_vec_xyzw ambient);
    void SetDiffuse(cpu_vec_xyzw diffuse);
    void SetSpecular(cpu_vec_xyzw specular);
    void SetPosition(cpu_vec_xyzw position);
    void SetDirection(cpu_vec_xyzw direction);

    void SetSpotDirection(cpu_vec_xyzw dir);
    void SetSpotCutoff(float cutoff);
    void SetSpotExponent(float exp);

    void SetConstantAtten(float atten);
    void SetLinearAtten(float atten);
    void SetQuadAtten(float atten);

    void SetEnabled(bool yesNo);
};

/********************************************
 * CLighting
 */

class CLighting {
protected:
    CGLContext& GLContext;
    static const int NumLights = 8;

public:
    CLighting(CGLContext& context)
        : GLContext(context)
    {
    }
    virtual ~CLighting()
    {
    }

    virtual CLight& GetLight(int num) = 0;

    virtual void SetLightingEnabled(bool enabled)      = 0;
    virtual void SetGlobalAmbient(cpu_vec_xyzw newAmb) = 0;
};

/********************************************
 * CImmLighting
 */

class CImmLighting : public CLighting {
    cpu_vec_xyzw CurrentColor;
    cpu_vec_xyzw GlobalAmbient;
    CImmLight Light0, Light1, Light2, Light3, Light4, Light5, Light6, Light7;
    CImmLight* Lights[NumLights];
    bool IsEnabled;
    int NumLightsWithNonzeroSpecular;

    inline void TellRendererLightPropChanged()
    {
        GLContext.LightPropChanged();
    }

public:
    CImmLighting(CGLContext& context);

    CImmLight& GetImmLight(int num)
    {
        mAssert(num < NumLights);
        return *Lights[num];
    }
    CLight& GetLight(int num) { return GetImmLight(num); }

    void SetLightingEnabled(bool enabled)
    {
        GLContext.LightingEnabledChanged();
        GLContext.GetImmGeomManager().GetRendererManager().LightingEnabledChanged(enabled);
        IsEnabled = enabled;
    }
    bool GetLightingEnabled() const { return IsEnabled; }

    void SetGlobalAmbient(cpu_vec_xyzw newAmb)
    {
        GlobalAmbient = newAmb;
        TellRendererLightPropChanged();
    }
    cpu_vec_xyzw GetGlobalAmbient() { return GlobalAmbient; }

    void SpecularChanged();
    void MaterialHasSpecular();
};

/********************************************
 * CDListLighting
 */

class CDListLighting : public CLighting {
    CDListLight Light0, Light1, Light2, Light3, Light4, Light5, Light6, Light7;
    CDListLight* Lights[NumLights];

    inline void TellRendererLightPropChanged()
    {
        GLContext.LightPropChanged();
    }

public:
    CDListLighting(CGLContext& context);

    CDListLight& GetDListLight(int num)
    {
        mAssert(num < NumLights);
        return *Lights[num];
    }
    CLight& GetLight(int num) { return GetDListLight(num); }

    void SetLightingEnabled(bool enabled);
    void SetGlobalAmbient(cpu_vec_xyzw newAmb);
};

#endif // ps2gl_lighting_h
