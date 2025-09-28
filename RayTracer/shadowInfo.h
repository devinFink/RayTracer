#pragma once
#include "renderer.h"

class ShadowInfo : public ShadeInfo
{
public:
	ShadowInfo(std::vector<Light*> const& lightList, RayTracer  const* renderer) : ShadeInfo(lightList), renderer(renderer) {};


	RayTracer const* renderer;
	float TraceShadowRay(Ray   const& ray, float t_max = BIGFLOAT) const override;
	Color TraceSecondaryRay(Ray const& ray, float& dist) const override;
	bool CanBounce() const override;

};