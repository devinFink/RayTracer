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

/**
 * Determines whether a point is shadowed along a given ray.
 * Casts a shadow ray and checks for intersections up to a maximum distance.
 *
 * @param ray   Shadow ray starting from the surface point toward the light.
 * @param t_max Maximum distance to test (typically distance to the light).
 * @return 0.0 if the ray is occluded (in shadow), 1.0 if unoccluded (lit).
 */
float GenLight::Shadow(Ray const& ray, float t_max)
{
	HitInfo h;
	h.Init();
	if (TraverseTree(ray, treeRoot, h, HIT_FRONT_AND_BACK))
	{
		if (h.z <= t_max)
			return 0.0;
	}

	return 1.0;
}

Color MtlPhong::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights, int bounceCount) const
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
 */
Color ReflectRay(const Ray& ray, const HitInfo& hInfo, const LightList& lights, int bounceCount, int HitSide)
{
	if (bounceCount < 0) return Color(0, 0, 0);

	HitInfo reflectHit;
	reflectHit.Init();

	cyVec3f reflectView = -Normalize(ray.dir);
	cyVec3f reflectionDir = 2 * hInfo.N.Dot(reflectView) * hInfo.N - reflectView;
	Ray reflect(hInfo.p, reflectionDir.GetNormalized());

	if (TraverseTree(reflect, treeRoot, reflectHit, HitSide) && (reflectHit.node->GetMaterial()))
	{
		return reflectHit.node->GetMaterial()->Shade(reflect, reflectHit, lights, bounceCount - 1);
	}

	return Color(0, 0, 0);
}

/**
 * Traces a refracted ray through a surface using Snell's law.
 * Handles entering/exiting, total internal reflection, and recursive shading.
 *
 * @param ior         Index of refraction of the material.
 * @param ray         Incident ray at the intersection.
 * @param hInfo       Surface hit information.
 * @param lights      Scene light list for shading.
 * @param bounceCount Remaining recursion depth.
 * @return Color contribution from the refracted (or reflected) ray.
 */
Color RefractRay(float ior, const Ray& ray, const HitInfo& hInfo, const LightList& lights, int bounceCount, Color absorption)
{
	Color refractCol(0, 0, 0);
	if (bounceCount < 0) return refractCol;
	HitInfo refractHit;
	refractHit.Init();

	cyVec3f refractView = -Normalize(ray.dir);
	float eta;
	float cosThetaT;
	cyVec3f refractDir;
	Ray refract;
	float eps = 1e-4f;

	if (hInfo.front)
	{
		eta = 1 / ior;
		cosThetaT = sqrt(1 - pow(eta, 2) * (1 - pow(refractView.Dot(hInfo.N), 2)));
		refractDir = -eta * refractView - (cosThetaT - eta * (refractView.Dot(hInfo.N))) * hInfo.N;
		refract = Ray(hInfo.p, refractDir.GetNormalized());
		float sign = refract.dir.Dot(hInfo.N) > 0.0f ? 1.0f : -1.0f;
		refract.p += hInfo.N * (eps * sign);
	}
	else
	{
		eta = ior / 1;
		float cosThetaSquared = (1 - pow(eta, 2) * (1 - pow(refractView.Dot(-hInfo.N), 2)));
		if (cosThetaSquared < 0.0f)
		{
			return ReflectRay(ray, hInfo, lights, bounceCount - 1, HIT_FRONT_AND_BACK);
		}
		cosThetaT = sqrt(cosThetaSquared);
		refractDir = -eta * refractView - (cosThetaT - eta * (refractView.Dot(-hInfo.N))) * -hInfo.N;
		refract = Ray(hInfo.p, refractDir.GetNormalized());
		float sign = refract.dir.Dot(-hInfo.N) > 0.0f ? 1.0f : -1.0f;
		refract.p += -hInfo.N * (eps * sign);
	}

	if (TraverseTree(refract, treeRoot, refractHit, HIT_FRONT_AND_BACK) && (refractHit.node->GetMaterial()))
	{
		refractCol = refractHit.node->GetMaterial()->Shade(refract, refractHit, lights, bounceCount - 1);

		if (refractHit.node == hInfo.node)
		{
			refractCol.r *= exp(-absorption.r * refractHit.z);
			refractCol.g *= exp(-absorption.g * refractHit.z);
			refractCol.b *= exp(-absorption.b * refractHit.z);
		}
	}
	return refractCol;
}

/**
 * Computes the shaded color at a surface point using the Blinn-Phong model.
 * Includes diffuse, specular, ambient, reflection, and refraction contributions,
 * with recursive ray tracing for reflective and refractive materials.
 *
 * @param ray         Incoming ray that hit the surface.
 * @param hInfo       Surface hit information.
 * @param lights      List of lights in the scene.
 * @param bounceCount Remaining recursion depth for reflection/refraction.
 * @return Final shaded color at the hit point.
 */
Color MtlBlinn::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights, int bounceCount) const
{
	Color finalColor(0, 0, 0);
	Color ambientLight(0, 0, 0);
	Color reflectCol(0, 0, 0);
	Color refractCol(0, 0, 0);
	Color fresnel(0, 0, 0);
	Color fullReflection = this->Reflection();



	if (this->ior > 0.0f && bounceCount > 0)
	{
		refractCol = this->refraction * RefractRay(this->ior, ray, hInfo, lights, bounceCount, this->absorption);

		//Fresnel Effect
		fresnel = this->refraction * pow((1 - this->ior) / (1 + this->ior), 2);
		fullReflection = fullReflection + fresnel;
		refractCol = refractCol * (Color(1, 1, 1) - fullReflection);
	}

	if (fullReflection != Color(0, 0, 0) && bounceCount > 0)
	{
		reflectCol = fullReflection * ReflectRay(ray, hInfo, lights, bounceCount, HIT_FRONT);
	}

	float alpha = this->Glossiness();
	Color kd = this->Diffuse();
	Color ks = this->Specular();

	for (int i = 0; i < lights.size(); i++)
	{
		if (lights[i]->IsAmbient())
		{
			ambientLight += lights[i]->Illuminate(hInfo.p, hInfo.N);
		}
		else
		{
			cyVec3f h = (((-lights[i]->Direction(hInfo.p)) - ray.dir).GetNormalized());
			float cosphi = std::max(hInfo.N.Dot(h), 0.0f);
			float costheta = std::max((-lights[i]->Direction(hInfo.p)).Dot(hInfo.N), 0.0f);

			Color color = (lights[i]->Illuminate(hInfo.p, hInfo.N) * ((costheta * kd) + (ks * pow(cosphi, alpha))));
			finalColor = finalColor + color;
		}
	}

	finalColor += (ambientLight * this->Diffuse());
	finalColor += reflectCol;
	finalColor += refractCol;
	return finalColor;
}

Color MtlMicrofacet::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights, int bounceCount) const
{
	return Color(255, 255, 255);
}