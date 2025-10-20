///
/// \file       materials.cpp
/// \author     Devin Fink
/// \date       September 13. 2025
///
/// \Implementation of the materials.h file functions
///

#include "materials.h"
#include "lights.h"
#include "raytracer.h"
#include "shadowInfo.h"

/**
 * Determines whether a point is shadowed along a given ray.
 * Casts a shadow ray and checks for intersections up to a maximum distance.
 *
 * @param ray   Shadow ray starting from the surface point toward the light.
 * @param t_max Maximum distance to test (typically distance to the light).
 * @return 0.0 if the ray is occluded (in shadow), 1.0 if unoccluded (lit).
 */
float ShadowInfo::TraceShadowRay(Ray const& ray, float t_max) const
{
	if (renderer->TraceShadowRay(ray, t_max)) return 0.0;

	return 1.0;
}

bool ShadowInfo::CanBounce() const
{
	return bounceS < renderer->bounceCount;
}

Color ShadowInfo::TraceSecondaryRay(Ray const& ray, float& dist) const
{
	HitInfo hit;
	hit.Init();

	if (renderer->TraceRay(ray, hit, HIT_FRONT_AND_BACK))
	{
		auto* mat = hit.node->GetMaterial();
		if (mat)
		{
			ShadowInfo si = *this;
			si.SetHit(ray, hit);
			si.IncrementBounce();
			si.IsFront() ? dist = si.Depth() : dist = 0;
			return mat->Shade(si);
		}
	}
	else
	{
		Color color = this->EvalEnvironment(ray.dir);
		return color;
	}

	dist = BIGFLOAT;
	return Color(0, 0, 0);
}

Color MtlPhong::Shade(ShadeInfo const& info) const
{
	return Color(255, 255, 255);
}

/**
 * Traces a reflected ray from a surface using the reflection equation.
 * Handles recursive shading until the bounce limit is reached.
 *
 * @param ray         Incident ray at the intersection.
 * @param hInfo       Surface hit information.
 * @param lights      Scene light list for shading.
 * @param bounceCount Remaining recursion depth.
 * @param HitSide     Flags specifying which surface sides to consider.
 * @return Color contribution from the reflected ray.
// */
TexturedColor ReflectRay(ShadeInfo const& info, int HitSide, Color absorption)
{
	Color reflectCol(0, 0, 0);
	cyVec3f reflectView = info.V();
	float dot = info.N().Dot(reflectView);
	cyVec3f reflectionDir = (2 * dot) * info.N() - reflectView;
	reflectionDir.Normalize();

	Ray reflect(info.P(), reflectionDir);
	float dist;

	reflectCol = info.TraceSecondaryRay(reflect, dist);
	reflectCol.r *= exp(-absorption.r * dist);
	reflectCol.g *= exp(-absorption.g * dist);
	reflectCol.b *= exp(-absorption.b * dist);

	return reflectCol;
}

/**
 * Traces a refracted ray through a surface using Snell's law.
 * Handles entering/exiting, total internal reflection, and recursive shading.
 *
 * @param ior         Index of refraction of the material.
 * @param hInfo       Surface hit information.
 * @param absorption  Absorption color.
 * @return Color contribution from the refracted (or reflected) ray.
 */
TexturedColor RefractRay(float ior, ShadeInfo const& info, Color absorption)
{
	Color refractCol(0, 0, 0);

	cyVec3f refractView = info.V();
	float eta;
	float cosThetaT;
	cyVec3f refractDir;
	Ray refract;
	float eps = 1e-4f;

	if (info.IsFront())
	{
		eta = 1 / ior;
		cosThetaT = sqrt(1 - pow(eta, 2) * (1 - pow(refractView.Dot(info.N()), 2)));
		refractDir = -eta * refractView - (cosThetaT - eta * (refractView.Dot(info.N()))) * info.N();
		refract = Ray(info.P(), refractDir.GetNormalized());
		float sign = refract.dir.Dot(info.N()) > 0.0f ? 1.0f : -1.0f;
		refract.p += info.N() * (eps * sign);
	}
	else
	{
		eta = ior / 1;
		float cosThetaSquared = (1 - pow(eta, 2) * (1 - pow(refractView.Dot(-info.N()), 2)));
		if (cosThetaSquared < 0.0f)
		{
			return ReflectRay(info, HIT_FRONT_AND_BACK, absorption);
		}
		cosThetaT = sqrt(cosThetaSquared);
		refractDir = -eta * refractView - (cosThetaT - eta * (refractView.Dot(-info.N()))) * -info.N();
		refract = Ray(info.P(), refractDir.GetNormalized());
		float sign = refract.dir.Dot(-info.N()) > 0.0f ? 1.0f : -1.0f;
		refract.p += -info.N() * (eps * sign);
	}

	float dist;
	refractCol = info.TraceSecondaryRay(refract, dist);
	refractCol.r *= exp(-absorption.r * dist);
	refractCol.g *= exp(-absorption.g * dist);
	refractCol.b *= exp(-absorption.b * dist);
	return refractCol;
}

/**
 * Computes the shaded color at a surface point using the Blinn-Phong model.
 * Includes diffuse, specular, ambient, reflection, and refraction contributions,
 * with recursive ray tracing for reflective and refractive materials.
 *
 * @param ShadeInfo     Surface hit information with lights and view direction
 * @return Final shaded color at the hit point.
 */
Color MtlBlinn::Shade(ShadeInfo const &info) const
{
	Color finalColor(0, 0, 0);
	Color ambientLight(0, 0, 0);
	Color reflectCol(0, 0, 0);
	Color refractCol(0, 0, 0);
	Color fresnel(0, 0, 0);
	Color reflection = this->Reflection().Eval(info.UVW());
	Color refraction = this->Refraction().Eval(info.UVW());

	
	Color fullReflection = reflection;;
	float matior = this->ior;
	if (matior > 0.0f && info.CanBounce())
	{
		refractCol = refraction * RefractRay(this->ior, info, this->absorption).Eval(info.UVW());

		//Fresnel Effect
		fresnel = refraction * pow((1 - matior) / (1 + matior), 2);
		fullReflection = fullReflection + fresnel;
		refractCol = refractCol * (Color(1, 1, 1) - fullReflection);
	}

	if (fullReflection != Color(0, 0, 0) && info.CanBounce())
	{
		reflectCol = fullReflection * ReflectRay(info, HIT_FRONT, this->absorption).Eval(info.UVW());
	}
	

	float alpha = this->Glossiness().Eval(info.UVW());
	Color kd = this->Diffuse().Eval(info.UVW());
	Color ks = this->Specular().Eval(info.UVW());

	for (int i = 0; i < info.NumLights(); i++)
	{
		const Light* light = info.GetLight(i);
		cyVec3f lightDir{};
		Color lightIntensity = light->Illuminate(info, lightDir);
		if (light->IsAmbient())
		{
			ambientLight += lightIntensity;
		}
		else
		{
			cyVec3f h = (lightDir + info.V()).GetNormalized();
			float cosphi = std::max(info.N().Dot(h), 0.0f);
			float costheta = std::max(lightDir.Dot(info.N()), 0.0f);

			Color color = (lightIntensity * ((costheta * kd) + (ks * pow(cosphi, alpha))));
			finalColor += color;
		}
	}


	finalColor += (ambientLight * kd);
	finalColor += reflectCol;
	finalColor += refractCol;
	return finalColor;
}

Color MtlMicrofacet::Shade(ShadeInfo const &shade) const
{
	return Color(255, 255, 255);
}