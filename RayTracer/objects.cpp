#include<objects.h>
#include <iostream>

Sphere theSphere;

bool Sphere::IntersectRay(Ray const& ray, HitInfo& hInfo, int hitSide) const
{
    cyVec3f q = cyVec3f(0.0f, 0.0f, 0.0f);
    int r = 1;

    float a = ray.dir.Dot(ray.dir);
    float b = 2.0 * (ray.dir.Dot(ray.p - q));
    float c = ray.p.Dot(ray.p) - r * r;
    float discriminant = b * b - 4 * a * c;

    float t = (-b - sqrt(discriminant)) / (2 * a);

    if(t > 0)
    {
        if(hInfo.z > t)
        {
            hInfo.z = t;
        }

        return true;
    }
}
