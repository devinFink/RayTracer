#include <iostream>
#include<objects.h>
#include <iostream>

Sphere theSphere;

bool Sphere::IntersectRay(Ray const& ray, HitInfo& hInfo, int hitSide) const
{
	cyVec3f origin = cyVec3f(0.0f, 0.0f, 0.0f);
	int r = 1;

	cyVec3f oc = origin - ray.p;
    float a = ray.dir.Dot(ray.dir);
    float b = -2.0 * ray.dir.Dot(oc);
    float c = oc.Dot(oc) - r * r;
    float discriminant = b * b - 4 * a * c;
    return (discriminant >= 0);
}
