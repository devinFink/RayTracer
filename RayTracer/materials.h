
//-------------------------------------------------------------------------------
///
/// \file       materials.h 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    4.0
/// \date       August 25, 2025
///
/// \brief Example source for CS 6620 - University of Utah.
///
//-------------------------------------------------------------------------------

#ifndef _MATERIALS_H_INCLUDED_
#define _MATERIALS_H_INCLUDED_

#include "scene.h"

//-------------------------------------------------------------------------------

class MtlBasePhongBlinn : public Material
{
public:
    MtlBasePhongBlinn() : diffuse(0.5f, 0.5f, 0.5f), specular(0.7f, 0.7f, 0.7f), glossiness(20.0f),
        reflection(0, 0, 0), refraction(0, 0, 0), absorption(0, 0, 0), ior(1.5f) {
    }

    void SetDiffuse(Color const& d) { diffuse = d; }
    void SetSpecular(Color const& s) { specular = s; }
    void SetGlossiness(float        g) { glossiness = g; }

    void SetReflection(Color const& r) { reflection = r; }
    void SetRefraction(Color const& r) { refraction = r; }
    void SetAbsorption(Color const& a) { absorption = a; }
    void SetIOR(float        i) { ior = i; }

    const Color& Diffuse() const { return diffuse; }
    const Color& Specular() const { return specular; }
    float        Glossiness() const { return glossiness; }

    const Color& Reflection() const { return reflection; }
    const Color& Refraction() const { return refraction; }
    const Color& Absorption() const { return absorption; }
    float        IOR() const { return ior; }

protected:
    Color diffuse, specular;
    float glossiness;
    Color reflection, refraction;
    Color absorption;
    float ior;  // index of refraction
};

//-------------------------------------------------------------------------------

class MtlPhong : public MtlBasePhongBlinn
{
public:
    Color Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights, int bounceCount) const override;
    void SetViewportMaterial(int subMtlID = 0) const override;    // used for OpenGL display
};

//-------------------------------------------------------------------------------

class MtlBlinn : public MtlBasePhongBlinn
{
public:
    Color Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights, int bounceCount) const override;
    void SetViewportMaterial(int subMtlID = 0) const override;    // used for OpenGL display
};

//-------------------------------------------------------------------------------

class MtlMicrofacet : public Material
{
public:
    MtlMicrofacet() : baseColor(0.5f, 0.5f, 0.5f), roughness(1.0f), metallic(0.0f), ior(1.5f),
        transmittance(0, 0, 0), absorption(0, 0, 0) {
    }

    void SetBaseColor(Color const& c) { baseColor = c; }
    void SetRoughness(float        r) { roughness = r; }
    void SetMetallic(float        m) { metallic = m; }
    void SetIOR(float        i) { ior = i; }
    void SetTransmittance(Color const& t) { transmittance = t; }
    void SetAbsorption(Color const& a) { absorption = a; }

    Color Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights, int bounceCount) const override;
    void SetViewportMaterial(int subMtlID = 0) const override;    // used for OpenGL display

private:
    Color baseColor;    // albedo for dielectrics, F0 for metals
    float roughness;
    float metallic;
    float ior;  // index of refraction
    Color transmittance;
    Color absorption;
};

//-------------------------------------------------------------------------------

#endif
