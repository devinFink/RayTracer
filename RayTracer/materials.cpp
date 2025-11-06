///
/// \file       materials.cpp
/// \author     Devin Fink
/// \date       September 13. 2025
///
/// \Implementation of the materials.h file functions
///


#define _USE_MATH_DEFINES
#include <cmath>
#include "materials.h"
#include "lights.h"
#include "raytracer.h"
#include "shadowInfo.h"


Color PointLight::Illuminate(ShadeInfo const& sInfo, Vec3f& dir)  const
{
	Vec3f toShadingPoint = sInfo.P() - position;
	toShadingPoint.Normalize();
	HaltonSeq<64> haltonX(2);
	HaltonSeq<64> haltonY(3);
	float randXOffset = sInfo.RandomFloat();
	float randYOffset = sInfo.RandomFloat();
	int numSamples = 0;
	const float twoPi = 2 * M_PI;

	float summedLight = 0.0f;
	Vec3f tangent, bitangent;
	toShadingPoint.GetOrthonormals(tangent, bitangent);

	for (int i = 0; i < maxSamples; i++)
	{
		float discX = haltonX[i] + randXOffset;
		float discY = haltonY[i] + randYOffset;
		discX -= (discX > 1.0f) ? 1.0f : 0.0f;
		discY -= (discY > 1.0f) ? 1.0f : 0.0f;


		float r = sqrt(discX) * size;
		float angle = twoPi * discY;
		float offsetU = r * cosf(angle);
		float offsetV = r * sinf(angle);

		Vec3f lightSamplePoint = position + (tangent * offsetU) + (bitangent * offsetV);

		Vec3f toLight = lightSamplePoint - sInfo.P();
		float dist = toLight.Length();
		Vec3f shadowRayDir = toLight / dist;

		summedLight += sInfo.TraceShadowRay(Ray(sInfo.P(), shadowRayDir), dist);
		numSamples++;

		if (numSamples == minSamples && summedLight == numSamples)
		{
			break;
		}
	}

	dir = position - sInfo.P();
	dir.Normalize();
	summedLight /= (float)numSamples;
	return intensity * summedLight;
}

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
	return bounceC < renderer->bounceCount;
}

Color ShadowInfo::TraceSecondaryRay(Ray const& ray, float& dist, bool reflection) const
{
	HitInfo hit;
	hit.Init();

	if (renderer->TraceRay(ray, hit, HIT_FRONT_AND_BACK))
	{
		if (!hit.light) {
			auto* mat = hit.node->GetMaterial();
			if (mat)
			{
				ShadowInfo si = *this;
				si.SetHit(ray, hit);
				si.bounceC++;
				si.IsFront() ? dist = si.Depth() : dist = 0;
				return mat->Shade(si);
			}
		}
		else {
			dist = hit.z;
			return Color::White();
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
	cyVec3f reflectView = info.V();
	float dot = info.N().Dot(reflectView);
	cyVec3f reflectionDir = (2 * dot) * info.N() - reflectView;
	reflectionDir.Normalize();

	Ray reflect(info.P(), reflectionDir);
	float dist;

	Color reflectCol = info.TraceSecondaryRay(reflect, dist, true);
	if (dist > 0.0f && (absorption.r > 0.0f || absorption.g > 0.0f || absorption.b > 0.0f)) {
		reflectCol.r *= expf(-absorption.r * dist);
		reflectCol.g *= expf(-absorption.g * dist);
		reflectCol.b *= expf(-absorption.b * dist);
	}

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
	cyVec3f refractView = info.V();
	float eta;
	float cosThetaT;
	cyVec3f refractDir;
	Ray refract;
	float eps = 1e-4f;

	if (info.IsFront())
	{
		eta = 1 / ior;
		float etaSq = eta * eta;
		float NdotV = refractView.Dot(info.N());
		cosThetaT = sqrtf(1 - etaSq * (1 - NdotV * NdotV));
		refractDir = -eta * refractView - (cosThetaT - eta * NdotV) * info.N();
		refract = Ray(info.P(), refractDir.GetNormalized());
		float sign = refract.dir.Dot(info.N()) > 0.0f ? 1.0f : -1.0f;
		refract.p += info.N() * (eps * sign);
	}
	else
	{
		eta = ior;
		cyVec3f negN = -info.N();
		float NdotV = refractView.Dot(negN);
		float etaSq = eta * eta;
		float cosThetaSquared = (1 - etaSq * (1 - NdotV * NdotV));

		if (cosThetaSquared < 0.0f)
		{
			return ReflectRay(info, HIT_FRONT_AND_BACK, absorption);
		}

		cosThetaT = sqrtf(cosThetaSquared);
		refractDir = -eta * refractView - (cosThetaT - eta * NdotV) * negN;
		refract = Ray(info.P(), refractDir.GetNormalized());
		float sign = refract.dir.Dot(negN) > 0.0f ? 1.0f : -1.0f;
		refract.p += negN * (eps * sign);
	}

	float dist;
	Color refractCol = info.TraceSecondaryRay(refract, dist, false);
	if (dist > 0.0f && (absorption.r > 0.0f || absorption.g > 0.0f || absorption.b > 0.0f)) {
		refractCol.r *= expf(-absorption.r * dist);
		refractCol.g *= expf(-absorption.g * dist);
		refractCol.b *= expf(-absorption.b * dist);
	}
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
	Color kd = this->Diffuse().Eval(info.UVW());
	Color ks = this->Specular().Eval(info.UVW());
	float alpha = this->Glossiness().Eval(info.UVW());

	
	Color fullReflection = reflection;
	float matior = this->ior;

	bool needsRefraction = (matior > 0.0f && info.CanBounce() && refraction != Color(0, 0, 0));

	if (needsRefraction)
	{
		refractCol = refraction * RefractRay(this->ior, info, this->absorption).Eval(info.UVW());

		//Fresnel Effect
		float iorRatio = (1.0f - matior) / (1.0f + matior);
		Color fresnel = refraction * (iorRatio * iorRatio);
		fullReflection = fullReflection + fresnel;
		refractCol = refractCol * (Color(1, 1, 1) - fullReflection);
	}

	if (fullReflection != Color(0,0,0) && info.CanBounce())
	{
		reflectCol = fullReflection * ReflectRay(info, HIT_FRONT_AND_BACK, this->absorption).Eval(info.UVW());
	}
	
	for (int i = 0; i < info.NumLights(); i++)
	{
		const Light* light = info.GetLight(i);
		cyVec3f lightDir;
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