///
/// \file       lights.cpp
/// \author     Devin Fink
/// \date       November 15. 2025
///
/// \Implementation of the Light related class functions
///

#define _USE_MATH_DEFINES
#include <cmath>
#include "materials.h"
#include "lights.h"
#include "raytracer.h"
#include "shadowInfo.h"
#include "photonmap.h"



Color PointLight::Illuminate(ShadeInfo const& sInfo, Vec3f& dir)  const
{
	Vec3f toShadingPoint = sInfo.P() - position;
	toShadingPoint.Normalize();
	HaltonSeq<128> haltonX(2);
	HaltonSeq<128> haltonY(3);
	float randXOffset = sInfo.RandomFloat();
	float randYOffset = sInfo.RandomFloat();
	int numSamples = 0;
	const float twoPi = 2 * M_PI;

	float summedLight = 0.0f;
	Vec3f tangent, bitangent;
	toShadingPoint.GetOrthonormals(tangent, bitangent);

	for (int i = 0; i < sInfo.maxShadowSamples; i++)
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

		if (numSamples == sInfo.minShadowSamples && summedLight == numSamples)
		{
			break;
		}
	}

	//Attenuation
	dir = position - sInfo.P();
	float dist = dir.Length();
	dir.Normalize();
	summedLight /= (float)numSamples;
	Color fullIntensity = intensity * summedLight;
	if (attenuation)
		return fullIntensity / (dist * dist);
	else
		return fullIntensity;
}

void PointLight::RandomPhoton(RNG& rng, Ray& r, Color& c) const {
	// Sample sphere surface
	float u1  = rng.RandomFloat();
	float u2  = rng.RandomFloat();

	float phi = 2.0f * M_PI * u2;
	float cosTheta = 1.0f - 2.0f * u1;
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

	Vec3f spherePoint(
		sinTheta * cos(phi),
		sinTheta * sin(phi),
		cosTheta
	);

	Vec3f photonOrigin = position + (spherePoint * size);
	Vec3f surfaceNormal = spherePoint;

	// Sample hemisphere (cosine-weighted)
	float u3 = rng.RandomFloat();
	float u4 = rng.RandomFloat();

	float r_hemi = sqrt(u3);
	float phi_hemi = 2.0f * M_PI * u4;

	Vec3f tangent, bitangent;
	surfaceNormal.GetOrthonormals(tangent, bitangent);

	Vec3f photonDir = (r_hemi * cos(phi_hemi) * tangent) +
					  (r_hemi * sin(phi_hemi) * bitangent) +
					  (sqrt(1.0f - u3) * surfaceNormal);
	photonDir.Normalize();

	r = Ray(photonOrigin, photonDir);
	c = intensity * 4 *  M_PI * size * size;
}


//ShadowInfo Methods -----------------------------------------------------------------------------------


/**
 * Determines whether a point is shadowed along a given ray.
 *
 * @param ray   Shadow ray starting from the surface point toward the light.
 * @param t_max Maximum distance to test (typically distance to the light).
 * @return 0.0 if the ray is occluded (in shadow), 1.0 if unoccluded (lit).
 */
inline float ShadowInfo::TraceShadowRay(Ray const& ray, float t_max) const
{
	return renderer->TraceShadowRay(ray, t_max) ? 0.0f : 1.0f;
}

/**
* Returns if a given shade ray can bounce again
*/
inline bool ShadowInfo::CanBounce() const
{
	return bounceC < renderer->bounceCount;
}

/**
* Returns if a given shade ray can preform another monte carlo pass
*/
inline bool ShadowInfo::CanMCBounce() const
{
	return bounceC < renderer->monteCarloBounces;
}

/**
* Returns the correct index in this shadeInfos haltonPhi sequence
*/
float ShadowInfo::GetHaltonPhi(int index) const {
	return haltonPhi[index];
}

/**
* Returns the correct index in this shadeInfos haltonTheta sequence
*/
float ShadowInfo::GetHaltonTheta(int index) const {
	return haltonTheta[index];
}


//----------------------------------------------------------------

/**
* Traverses the scene with a secondary ray, checking bounce counts and evaluating the environment if needed
* 
* @param ray			Secondary ray to trace
* @param dist			Output variable for storing the distance from the object hit
* @bool  reflection		If this ray is a reflection ray or refraction ray 
*/
Color ShadowInfo::TraceSecondaryRay(Ray const& ray, float& dist, bool reflection) const
{
	HitInfo hit;
	hit.Init();

	if (reflection)
	{
		if (ray.dir.Dot(this->N()) < 0) {
			ShadowInfo si = *this;
			hit = hInfo;
			si.SetHit(ray, hit);
			si.bounceC++;
			auto* mat = hit.node->GetMaterial();
			return mat->Shade(si);
		}
	}

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