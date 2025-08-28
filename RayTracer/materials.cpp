#include "materials.h"

Color MtlPhong::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights) const
{
	return Color(0, 0, 0);
}

Color MtlBlinn::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights) const
{
	return Color(0, 0, 0);
}

Color MtlMicrofacet::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights) const
{
	return Color(0, 0, 0);
}