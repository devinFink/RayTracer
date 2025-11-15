#pragma once
#include "renderer.h"

class ShadowInfo : public ShadeInfo
{
public:
	ShadowInfo(std::vector<Light*> const& lightList, TexturedColor const& environment, RNG& r, RayTracer const* renderer) 
		: ShadeInfo(lightList, environment, r), renderer(renderer) {
		haltonPhi = HaltonSeq<1000>(2);
		haltonTheta = HaltonSeq<1000>(3);
	};


	RayTracer const* renderer;
	float TraceShadowRay(Ray   const& ray, float t_max = BIGFLOAT) const override;
	Color TraceSecondaryRay(Ray const& ray, float& dist, bool reflection) const override;
	bool CanBounce() const override;
	bool CanMCBounce() const override; 
	float GetHaltonPhi(int index) const override;
	float GetHaltonTheta(int index) const override;


protected:
	int bounceC = 0;

	//Random halton Sequences
	HaltonSeq<1000> haltonPhi;
	HaltonSeq<1000> haltonTheta;
};
