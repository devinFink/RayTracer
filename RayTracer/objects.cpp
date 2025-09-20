///
/// \file       objects.cpp 
/// \author     Devin Fink
/// \date       September 13, 2025
///
/// \brief      Methods corresponding to objects defined in objects.h
///

#include <iostream>
#include <objects.h>
#include <cmath>
#include <limits>

cyVec3f vecMin(const cyVec3f& a, const cyVec3f& b) {
    return {
        std::min(a.x, b.x),
        std::min(a.y, b.y),
        std::min(a.z, b.z)
    };
}

cyVec3f vecMax(const cyVec3f& a, const cyVec3f& b) {
    return {
        std::max(a.x, b.x),
        std::max(a.y, b.y),
        std::max(a.z, b.z)
    };
}

/// Ray-AABB intersection
bool hitAABB(Ray ray, cyVec3f boxMin, cyVec3f boxMax) {
    auto safeInv = [](float d) {
        return (fabs(d) > 1e-8f) ? (1.0f / d) : std::numeric_limits<float>::infinity();
        };

    cyVec3f invR = cyVec3f(safeInv(ray.dir.x), safeInv(ray.dir.y), safeInv(ray.dir.z));
    cyVec3f tbot = invR * (boxMin - ray.p);
    cyVec3f ttop = invR * (boxMax - ray.p);
    cyVec3f tmin = vecMin(ttop, tbot);
    cyVec3f tmax = vecMax(ttop, tbot);

    cyVec2f t;
    t.x = std::max(tmin.x, tmin.y);
    t.y = std::max(tmin.x, tmin.z);
    float t0 = std::max(t.x, t.y);

    t.x = std::max(tmax.x, tmax.y);
    t.y = std::max(tmax.x, tmax.z);
    float t1 = std::min(t.x, t.y);

    return t0 <= t1 && t1 >= 0.0f;
}

////////////////////////////////////////////////////////////////////////////////
// Sphere
////////////////////////////////////////////////////////////////////////////////

bool Sphere::IntersectRay(Ray const& ray, HitInfo& hInfo, int hitSide) const {
    cyVec3f q(0.0f, 0.0f, 0.0f);
    int r = 1;

    cyVec3f oc = q - ray.p;
    double a = ray.dir.Dot(ray.dir);
    double b = 2.0 * ray.dir.Dot(ray.p - q);
    double c = ray.p.Dot(ray.p) - r * r;
    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;

    double t1 = (-b - sqrt(discriminant)) / (2 * a);
    double t2 = (-b + sqrt(discriminant)) / (2 * a);

    if (t1 > 0.0002 && hitSide & HIT_FRONT) {
        if (hInfo.z > t1) {
            hInfo.z = t1;
            hInfo.p = ray.p + (ray.dir * t1);
            hInfo.N = hInfo.p.GetNormalized();
            hInfo.front = true;
            return true;
        }
    }
    else if (t2 > 0.0002 && hitSide & HIT_BACK) {
        if (hInfo.z > t2) {
            hInfo.z = t2;
            hInfo.p = ray.p + (ray.dir * t2);
            hInfo.N = hInfo.p.GetNormalized();
            hInfo.front = false;
            return true;
        }
    }

    return false;
}

bool Sphere::shadowRay(Ray const& ray, float t_max) const {
    cyVec3f q(0.0f, 0.0f, 0.0f);
    int r = 1;

    cyVec3f oc = q - ray.p;
    double a = ray.dir.Dot(ray.dir);
    double b = 2.0 * ray.dir.Dot(ray.p - q);
    double c = ray.p.Dot(ray.p) - r * r;
    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;

    double t1 = (-b - sqrt(discriminant)) / (2 * a);
    double t2 = (-b + sqrt(discriminant)) / (2 * a);

    if (t1 > 0.0002 && t1 < t_max) return true;
    if (t2 > 0.0002 && t2 < t_max) return true;

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// Plane
////////////////////////////////////////////////////////////////////////////////

bool Plane::IntersectRay(Ray const& ray, HitInfo& hInfo, int hitSide) const {
    cyVec3f planeNorm(0.0f, 0.0f, 1.0f);
    float t = -(ray.p.z / ray.dir.z);

    if (t > 0.0002 && hInfo.z > t) {
        hInfo.z = t;
        hInfo.N = planeNorm;
        hInfo.p = ray.p + (ray.dir * t);

        // Check if the intersection point is within the bounds of the plane
        if (hInfo.p.x < -1.0f || hInfo.p.x > 1.0f || hInfo.p.y < -1.0f || hInfo.p.y > 1.0f)
            return false;

        hInfo.front = (ray.dir.Dot(planeNorm) < 0);
        if ((hInfo.front && (hitSide & HIT_FRONT)) || (!hInfo.front && (hitSide & HIT_BACK)))
            return true;
    }

    return false;
}

bool Plane::shadowRay(Ray const& ray, float t_max) const {
    cyVec3f planeNorm(0.0f, 0.0f, 1.0f);
    float t = -(ray.p.z / ray.dir.z);

    if (t > 0.0002 && t < t_max) {
        cyVec3f p = ray.p + (ray.dir * t);

        // Check if the intersection point is within the bounds of the plane
        if (p.x < -1.0f || p.x > 1.0f || p.y < -1.0f || p.y > 1.0f)
            return false;

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// Triangle Mesh
////////////////////////////////////////////////////////////////////////////////

bool TriObj::IntersectRay(Ray const& ray, HitInfo& hInfo, int hitSide) const {
    HitInfo temphit;
    temphit.Init();
    bool hit = false;

    if (!hitAABB(ray, GetBoundMin(), GetBoundMax()))
        return false;

    for (int faceID = 0; faceID < NF(); faceID++) {
        if (IntersectTriangle(ray, temphit, hitSide, faceID)) {
            if (hInfo.z > temphit.z) {
                hInfo = temphit;
            }
            hit = true;
        }
    }

    return hit;
}

bool TriObj::shadowRay(Ray const& ray, float t_max) const {
    HitInfo temphit;
    temphit.Init();

    if (!hitAABB(ray, GetBoundMin(), GetBoundMax()))
        return false;

    for (int faceID = 0; faceID < NF(); faceID++) {
        if (IntersectTriangle(ray, temphit, HIT_FRONT_AND_BACK, faceID)) {
            if (temphit.z < t_max)
                return true;
        }
    }

    return false;
}

/*
* Moller-Trumbore intersection algorithm
*/
bool TriObj::IntersectTriangle(Ray const& ray, HitInfo& hInfo, int hitSide, unsigned int faceID) const {
    cyVec3f v0 = V(F(faceID).v[0]);
    cyVec3f v1 = V(F(faceID).v[1]);
    cyVec3f v2 = V(F(faceID).v[2]);
    const float epsilon = 0.0002f;

    cyVec3f edge1 = v1 - v0;
    cyVec3f edge2 = v2 - v0;
    cyVec3f h = ray.dir.Cross(edge2);
    float det = edge1.Dot(h);

    if (fabs(det) < epsilon) return false;

    float inv_det = 1.0f / det;
    cyVec3f s = ray.p - v0;
    float u = inv_det * s.Dot(h);

    // Branchless check for barycentric coordinates
    bool valid = (u >= 0.0f) && (u <= 1.0f);

    cyVec3f s_cross_e1 = s.Cross(edge1);
    float v = inv_det * ray.dir.Dot(s_cross_e1);

    valid &= (v >= 0.0f) && ((u + v) <= 1.0f);

    if (!valid) return false;

    float t = inv_det * edge2.Dot(s_cross_e1);

    if (t > epsilon) {
        hInfo.z = t;
        hInfo.p = ray.p + (ray.dir * t);

        // Interpolate normals using barycentric coordinates
        hInfo.N = ((1.0f - u - v) * vn[FN(faceID).v[0]] +
            u * vn[FN(faceID).v[1]] +
            v * vn[FN(faceID).v[2]]).GetNormalized();

        hInfo.front = ray.dir.Dot(hInfo.N) < 0;

        if ((hInfo.front && (hitSide & HIT_FRONT)) || (!hInfo.front && (hitSide & HIT_BACK)))
            return true;
    }

    return false;
}
