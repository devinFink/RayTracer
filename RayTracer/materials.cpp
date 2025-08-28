#include "materials.h"

Color MtlPhong::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights) const
{
	return Color(255, 255, 255);
}

Color MtlBlinn::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights) const
{
	Color finalColor = Color(0, 0, 0);
	for (int i = 0; i < lights.size(); i++)
	{
		float alpha = 15.0f;
		Color kd = this->diffuse;
		Color ks = this->specular;
		cyVec3f h = (lights[i]->Direction(hInfo.p) - ray.dir).GetNormalized();
		float cosphi = std::max(hInfo.N.Dot(h), 0.0f);
		float costheta = std::max(lights[i]->Direction(hInfo.p).Dot(hInfo.N), 0.0f);

		Color color = (1 * ((costheta * kd) + (ks * pow(cosphi, alpha))) + (.2 * kd));
		finalColor = finalColor + color;
	}

	return Color(255, 255, 255);
}

Color MtlMicrofacet::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights) const
{
	return Color(255, 255, 255);
}