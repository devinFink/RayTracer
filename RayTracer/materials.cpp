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
#include "photonmap.h"
#include <iostream>

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
Ray ReflectRay(SamplerInfo const& info, int HitSide, Color absorption, TexturedFloat glossiness)
{
	cyVec3f reflectView = info.V();
	cyVec3f norm = info.N();

	//Glossiness Sampling
	float offsetPhi = info.RandomFloat();
	float offsetTheta = info.RandomFloat();
	float phi = 2.0f * M_PI * fmod(Halton(info.CurrentPixelSample(), 2) + offsetPhi, 1.0f);
	float cosTheta = pow(fmod(Halton(info.CurrentPixelSample(), 3) + offsetTheta, 1.0f), 1.0f / (glossiness.GetValue() + 1.0f));
	float sinTheta = sqrt(1.0f - (cosTheta * cosTheta));

	float x = sinTheta * cos(phi);
	float y = sinTheta * sin(phi);
	float z = cosTheta;

	Vec3f tangent, bitangent;
	norm.GetOrthonormals(tangent, bitangent);

	Vec3f HTransformed = (x * tangent) + (y * bitangent) + (z * norm);

	//Find final direction
	float dot = HTransformed.Dot(reflectView);
	cyVec3f reflectionDir = (2 * dot) * HTransformed - reflectView;
	reflectionDir.Normalize();

	Ray reflect(info.P(), reflectionDir);

	return reflect;
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
Ray RefractRay(float ior, SamplerInfo const& info, Color absorption, TexturedFloat glossiness)
{
	cyVec3f refractView = info.V();
	float eta;
	float cosThetaT;
	cyVec3f refractDir;
	Ray refract;
	float eps = 1e-4f;

	//Glossiness Sampling
	float offsetPhi = info.RandomFloat();
	float offsetTheta = info.RandomFloat();
	float phi = 2.0f * M_PI * fmod(Halton(info.CurrentPixelSample(), 2) + offsetPhi, 1.0f);
	float cosTheta = pow(fmod(Halton(info.CurrentPixelSample(), 3) + offsetTheta, 1.0f), 1.0f / (glossiness.GetValue() + 1.0f));
	float sinTheta = sqrt(1.0f - (cosTheta * cosTheta));

	float x = sinTheta * cos(phi);
	float y = sinTheta * sin(phi);
	float z = cosTheta;

	Vec3f tangent, bitangent;
	info.N().GetOrthonormals(tangent, bitangent);

	Vec3f HTransformed = (x * tangent) + (y * bitangent) + (z * info.N());
	HTransformed.Normalize();

	//Check if the ray is entering the object or exiting
	if (info.IsFront())
	{
		//If entering, use 1/ior for refraction and calculate direction
		eta = 1 / ior;
		float etaSq = eta * eta;
		float NdotV = refractView.Dot(HTransformed);
		cosThetaT = sqrtf(1 - etaSq * (1 - NdotV * NdotV));
		refractDir = -eta * refractView - (cosThetaT - eta * NdotV) * HTransformed;
		refract = Ray(info.P(), refractDir.GetNormalized());
		float sign = refract.dir.Dot(info.N()) > 0.0f ? 1.0f : -1.0f;
		refract.p += info.N() * (eps * sign);
	}
	else
	{
		//If exiting, use original ior and calculate direction
		eta = ior;
		cyVec3f H_back = -HTransformed;
		cyVec3f negN = -info.N();
		float NdotV = refractView.Dot(H_back);
		float etaSq = eta * eta;
		float cosThetaSquared = (1.0 - etaSq * (1 - NdotV * NdotV));

		//Total Internal Reflection check
		if (cosThetaSquared < 0.0f)
		{
			return ReflectRay(info, HIT_FRONT_AND_BACK, absorption, glossiness);
		}

		cosThetaT = sqrtf(cosThetaSquared);
		refractDir = -eta * refractView - (cosThetaT - eta * NdotV) * H_back;
		refract = Ray(info.P(), refractDir.GetNormalized());
		float sign = refract.dir.Dot(negN) > 0.0f ? 1.0f : -1.0f;
		refract.p += negN * (eps * sign);
	}

	return refract;
}

Color SampleIndirectDiffuseUnweighted(ShadeInfo const& info)
{
	float offset = info.RandomFloat();
	float offset2 = info.RandomFloat();
	Color totalLight(0, 0, 0);

	for (int i = 0; i < info.mcSamples; i++)
	{
		// Uniform hemisphere sampling
		float u1 = fmod(info.GetHaltonPhi(info.CurrentPixelSample() * info.mcSamples + i) + offset, 1.0f);
		float u2 = fmod(info.GetHaltonTheta(info.CurrentPixelSample() * info.mcSamples + i) + offset2, 1.0f);

		float phi = 2.0f * M_PI * u2; 
		float cosTheta = u1;       
		float sinTheta = sqrt(1.0f - u1 * u1);  

		float x = sinTheta * cos(phi);
		float y = sinTheta * sin(phi);
		float z = cosTheta;

		// Build tangent frame around surface normal
		Vec3f N = info.N();
		Vec3f tangent, bitangent;
		N.GetOrthonormals(tangent, bitangent);

		// Transform to world space
		Vec3f direction = (x * tangent) + (y * bitangent) + (z * N);
		direction.Normalize();

		// Trace the indirect ray
		Ray indirectRay(info.P(), direction);
		float dist;
		Color incomingLight = info.TraceSecondaryRay(indirectRay, dist, false);

		totalLight += incomingLight * cosTheta;
	}

	return totalLight / (float)info.mcSamples;
}

Color SampleIndirectDiffuseCosin(ShadeInfo const& info) 
{
	// Cosine-weighted hemisphere sampling
	float offset = info.RandomFloat();
	float offset2 = info.RandomFloat();
	Color totalLight(0, 0, 0);

	for (int i = 0; i < info.mcSamples; i++)
	{
		float u1 = fmod(info.GetHaltonPhi(info.CurrentPixelSample() * info.mcSamples + i) + offset, 1.0f);
		float u2 = fmod(info.GetHaltonTheta(info.CurrentPixelSample() * info.mcSamples + i) + offset2, 1.0f);

		// Cosine-weighted sampling
		float r = sqrt(u1);
		float phi = 2.0f * M_PI * u2;

		float x = r * cos(phi);
		float y = r * sin(phi);
		float z = sqrt(1.0f - u1);

		// Build tangent frame around surface normal
		Vec3f N = info.N();
		Vec3f tangent, bitangent;
		N.GetOrthonormals(tangent, bitangent);

		// Transform to world space
		Vec3f direction = (x * tangent) + (y * bitangent) + (z * N);
		direction.Normalize();

		// Trace the indirect ray
		Ray indirectRay(info.P(), direction);
		float dist;
		totalLight += info.TraceSecondaryRay(indirectRay, dist, false);
	}


	return totalLight / info.mcSamples;
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
	//Summation Colors
	Color finalColor(0, 0, 0), ambientLight(0, 0, 0), reflectCol(0, 0, 0), refractCol(0, 0, 0),
		  fresnel(0, 0, 0), indirect(0,0,0), irradiance(0,0,0);

	cyVec3f photonDir(0, 0, 0);

	//Material Colors
	Color reflection = this->Reflection().Eval(info.UVW());
	Color refraction = this->Refraction().Eval(info.UVW());
	Color kd = this->Diffuse().Eval(info.UVW());
	Color ks = this->Specular().Eval(info.UVW());
	float alpha = this->Glossiness().Eval(info.UVW());
	Color emission = this->Emission().Eval(info.UVW());

	//Energy Conservation
	float diffScalar = (1 / M_PI);
	float specScalar = (alpha + 2) / (8 * M_PI);
	Color cKd = kd * diffScalar;
	Color cKs = ks * specScalar;
	Color fullReflection = reflection;
	float matior = this->ior;

	bool needsRefraction = (matior > 0.0f && info.CanBounce() && refraction != Color(0, 0, 0));

	//Sum Refraction Colors
	if (needsRefraction)
	{
		float dist;
		Ray refract = RefractRay(this->ior, info, this->absorption, this->glossiness);

		refractCol = info.TraceSecondaryRay(refract, dist, false);
		if (dist > 0.0f && (absorption.r > 0.0f || absorption.g > 0.0f || absorption.b > 0.0f)) {
			refractCol.r *= expf(-absorption.r * dist);
			refractCol.g *= expf(-absorption.g * dist);
			refractCol.b *= expf(-absorption.b * dist);
		}

		TexturedColor texRefractCol = refractCol;
		refractCol = refraction * texRefractCol.Eval(info.UVW());

		//Fresnel Effect
		float iorRatio = (1.0f - matior) / (1.0f + matior);
		Color fresnel = refraction * (iorRatio * iorRatio);
		fullReflection = fullReflection + fresnel;
		refractCol = refractCol * (Color(1, 1, 1) - fullReflection);
	}

	//Sum Reflection colors
	if (fullReflection != Color(0,0,0) && info.CanBounce())
	{
		float dist;
		Ray reflect = ReflectRay(info, HIT_FRONT_AND_BACK, this->absorption, this->glossiness);
		reflectCol = info.TraceSecondaryRay(reflect, dist, true);
		if (dist > 0.0f && (absorption.r > 0.0f || absorption.g > 0.0f || absorption.b > 0.0f)) {
			reflectCol.r *= expf(-absorption.r * dist);
			reflectCol.g *= expf(-absorption.g * dist);
			reflectCol.b *= expf(-absorption.b * dist);
		}

		TexturedColor texReflectCol = reflectCol;
		reflectCol = reflection * texReflectCol.Eval(info.UVW());
	}
	
	//Sum Diffuse + Specular Colors
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
			//Calculate Half vector and angles
			cyVec3f h = (lightDir + info.V()).GetNormalized();
			float cosphi = std::max(info.N().Dot(h), 0.0f);
			float costheta = std::max(lightDir.Dot(info.N()), 0.0f);

			//Full BRDF
			finalColor += (lightIntensity * ((costheta * cKd) + (cKs * pow(cosphi, alpha))));
		}
	}

	//Sum Monte Carlo Global Illumination
	//if (info.CanMCBounce()) {
	//	indirect = SampleIndirectDiffuseCosin(info) * kd;
	//}
	//else
	//{
	//	info.GetRenderer()->GetPhotonMap()->EstimateIrradiance<128>(irradiance, photonDir, 3.0f, info.P(), info.N(), 1.0f);
	//	indirect += (1.0f / M_PI) * kd * irradiance;
	//}

	info.GetRenderer()->GetPhotonMap()->EstimateIrradiance<128>(irradiance, photonDir, 3.0f, info.P(), info.N(), 1.0f);
	indirect += (1.0f / M_PI) * kd * irradiance;

	//Summing final components
	finalColor += indirect;
	finalColor += reflectCol;
	finalColor += refractCol;
	finalColor += emission;
	return finalColor;
}



bool MtlBlinn::GenerateSample(SamplerInfo const& sInfo, Vec3f& dir, Info& si) const {
	float random = sInfo.RandomFloat();
	float diffuseProb(diffuse.GetValue().Gray());
	float reflectProb(specular.GetValue().Gray());
	float refractProb(refraction.GetValue().Gray());

	//Go with Diffuse
	if(random < diffuseProb) {
		float u1 = sInfo.RandomFloat();
		float u2 = sInfo.RandomFloat();

		float phi = 2.0f * M_PI * u2;
		float cosTheta = u1;
		float sinTheta = sqrt(1.0f - u1 * u1);

		float x = sinTheta * cos(phi);
		float y = sinTheta * sin(phi);
		float z = cosTheta;

		// Build tangent frame around surface normal
		Vec3f N = sInfo.N();
		Vec3f tangent, bitangent;
		N.GetOrthonormals(tangent, bitangent);

		// Transform to world space
		Vec3f direction = (x * tangent) + (y * bitangent) + (z * N);
		direction.Normalize();
		dir = direction;
		si.mult = diffuse.GetValue();
		si.prob = diffuseProb;
		si.lobe = DIFFUSE;
		return true;
	}
	//Reflect Photon
	else if(random < diffuseProb + reflectProb) {
		Ray reflect = ReflectRay(sInfo, HIT_FRONT_AND_BACK, absorption, glossiness);
		dir = reflect.dir;
		si.mult = specular.GetValue() * dir.Dot(sInfo.N());
		si.lobe = SPECULAR;
		si.prob = reflectProb;
		return true;
	}
	//Refract Photon
	else if(random < diffuseProb + reflectProb + refractProb) {
		Ray refract = RefractRay(ior, sInfo, absorption, glossiness);
		dir = refract.dir;
		si.mult = refraction.GetValue() * std::abs(dir.Dot(sInfo.N()));
		si.lobe = TRANSMISSION;
		si.prob = refractProb;
		return true;
	}
	//Destroy Photon
	else {
		return false;
	}
}

bool MtlMicrofacet::GenerateSample(SamplerInfo const& sInfo, Vec3f& dir, Info& si) const {
	return false;
}

Color MtlMicrofacet::Shade(ShadeInfo const &shade) const
{
	return Color(1.0f, 1.0f, 1.0f);
}

bool MtlPhong::GenerateSample(SamplerInfo const& sInfo, Vec3f& dir, Info& si) const {
	return false;
}

Color MtlPhong::Shade(ShadeInfo const& info) const
{
	return Color(1.0f, 1.0f, 1.0f);
}