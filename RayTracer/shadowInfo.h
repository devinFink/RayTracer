#pragma once
#include "renderer.h"

class ShadowInfo : public ShadeInfo
{
public:
	ShadowInfo(std::vector<Light*> const& lightList, TexturedColor const& environment, RayTracer  const* renderer) : ShadeInfo(lightList, environment), renderer(renderer) {};


	RayTracer const* renderer;
	float TraceShadowRay(Ray   const& ray, float t_max = BIGFLOAT) const override;
	Color TraceSecondaryRay(Ray const& ray, float& dist) const override;
	bool CanBounce() const override;
};