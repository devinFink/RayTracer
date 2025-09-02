#include "materials.h"
#include "lights.h"
#include "raytracer.h"

float GenLight::Shadow(Ray const& ray, float t_max)
{
	HitInfo h;
	if (TraverseTree(ray, treeRoot, h))
	{
		if(h.z +.1 < t_max)
			return 0.0;
	}

	return 1.0;
}

Color MtlPhong::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights) const
{
	return Color(255, 255, 255);
}

Color MtlBlinn::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights) const
{
	Color finalColor = Color(0, 0, 0);
	Color ambientLight = Color(0, 0, 0);

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
	return finalColor;
}

Color MtlMicrofacet::Shade(Ray const& ray, HitInfo const& hInfo, LightList const& lights) const
{
	return Color(255, 255, 255);
}