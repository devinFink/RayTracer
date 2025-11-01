#pragma once
#include "renderer.h"

class ShadowInfo : public ShadeInfo
{
public:
	ShadowInfo(std::vector<Light*> const& lightList, TexturedColor const& environment, RNG& r, RayTracer const* renderer) : ShadeInfo(lightList, environment, r), renderer(renderer) {};


	RayTracer const* renderer;
	float TraceShadowRay(Ray   const& ray, float t_max = BIGFLOAT) const override;
	Color TraceSecondaryRay(Ray const& ray, float& dist, bool reflection) const override;
	bool CanBounce() const override;

protected:
	int bounceC = 0;
};