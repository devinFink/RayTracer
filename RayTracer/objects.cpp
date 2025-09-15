///
/// \file       objects.cpp 
/// \author     Devin FInk
/// \date       Septmeber 13, 2025
///
/// \Methods corresponding to objects defined in objects.h
///

#include <iostream>
#include <objects.h>

bool Sphere::IntersectRay(Ray const& ray, HitInfo& hInfo, int hitSide) const
{
    cyVec3f q = cyVec3f(0.0f, 0.0f, 0.0f);
	int r = 1;

	cyVec3f oc = q - ray.p;
    double a = ray.dir.Dot(ray.dir);
    double b = 2.0 * (ray.dir.Dot(ray.p - q));
    double c = ray.p.Dot(ray.p) - r * r;
    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;

    double t1 = (-b - sqrt(discriminant)) / (2 * a);
    double t2 = (-b + sqrt(discriminant)) / (2 * a);

    if(t1 > 0.0002 && hitSide & HIT_FRONT)
    {
        if(hInfo.z > t1)
        {
            hInfo.z = t1;
            hInfo.p = ray.p + (ray.dir * t1);
            hInfo.N = hInfo.p.GetNormalized();
            hInfo.front = true;
            return true;
        }
    }
    else if(t2 > 0.0002 && hitSide & HIT_BACK)
    {
        if (hInfo.z > t2)
        {
            hInfo.z = t2;
            hInfo.p = ray.p + (ray.dir * t2);
            hInfo.N = hInfo.p.GetNormalized();
            hInfo.front = false;
            return true;
        }
    }

    return false;
}
