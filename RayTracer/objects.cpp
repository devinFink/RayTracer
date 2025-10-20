///
/// \file       objects.cpp 
/// \author     Devin Fink
/// \date       September 13, 2025
///
/// \brief      Methods corresponding to objects defined in objects.h
///

#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include <limits>
#include "objects.h"

////////////////////////////////////////////////////////////////////////////////
// Sphere
////////////////////////////////////////////////////////////////////////////////

bool Sphere::IntersectRay(Ray const& ray, HitInfo& hInfo, int hitSide) const {
    cyVec3f q(0.0f, 0.0f, 0.0f);
    int r = 1;
	float eps = 0.002f;
    double a = ray.dir.Dot(ray.dir);
    double b = 2.0 * ray.dir.Dot(ray.p - q);
    double c = ray.p.Dot(ray.p) - r * r;
    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;

    double t1 = (-b - sqrt(discriminant)) / (2 * a);
    double t2 = (-b + sqrt(discriminant)) / (2 * a);

    if (t1 > eps && hitSide & HIT_FRONT) {
        if (hInfo.z > t1) {
            hInfo.z = t1;
            hInfo.p = ray.p + (ray.dir * t1);
            hInfo.N = hInfo.p.GetNormalized();
            float tu = (atan2(hInfo.p.y, hInfo.p.x) / (2 * M_PI)) + .5;
            float tv = asin(hInfo.p.z) / M_PI + .5;
            hInfo.uvw = cyVec3f(tu, tv, 0.0f);
            hInfo.front = true;
            return true;
        }
    }
    else if (t2 >= eps && hitSide & HIT_BACK) {
        if (hInfo.z > t2) {
            hInfo.z = t2;
            hInfo.p = ray.p + (ray.dir * t2);
            hInfo.N = hInfo.p.GetNormalized();
            float tu = (atan2(hInfo.p.y, hInfo.p.x) / (2 * M_PI)) + .5;
			float tv = asin(hInfo.p.z) / M_PI + .5;
			hInfo.uvw = cyVec3f(tu, tv, 0.0f);
            hInfo.front = false;
            return true;
        }
    }

    return false;
}

bool Sphere::ShadowRay(Ray const& ray, float t_max) const {
    cyVec3f q(0.0f, 0.0f, 0.0f);
    int r = 1;

    double a = ray.dir.Dot(ray.dir);
    double b = 2.0 * ray.dir.Dot(ray.p - q);
    double c = ray.p.Dot(ray.p) - r * r;
    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return false;

    double t1 = (-b - sqrt(discriminant)) / (2 * a);
    double t2 = (-b + sqrt(discriminant)) / (2 * a);

    if (t1 > 0.01 && t1 < t_max) return true;
    if (t2 > 0.01 && t2 < t_max) return true;

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// Box
////////////////////////////////////////////////////////////////////////////////
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
/// TODO: Switch to return a struct with bool hit, float tmin, float tmax if needed
/// 
bool hitAABB(Ray ray, cyVec3f boxMin, cyVec3f boxMax) {
    auto safeInv = [](float d) {
        return (fabs(d) > 1e-8f) ? (1.0f / d) : std::numeric_limits<float>::infinity();
        };

    cyVec3f invR = cyVec3f(safeInv(ray.dir.x), safeInv(ray.dir.y), safeInv(ray.dir.z));
    cyVec3f tbot = invR * (boxMin - ray.p);
    cyVec3f ttop = invR * (boxMax - ray.p);
    cyVec3f tmin = vecMin(ttop, tbot);
    cyVec3f tmax = vecMax(ttop, tbot);

    float t0 = std::max({ tmin.x, tmin.y, tmin.z });

    float t1 = std::min({ tmax.x, tmax.y, tmax.z });

    return t0 <= t1 && t1 >= 0.0f;
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
		hInfo.uvw = cyVec3f((hInfo.p.x + 1.0f) * 0.5f, (hInfo.p.y + 1.0f) * 0.5f, 0.0f);

        // Check if the intersection point is within the bounds of the plane
        if (hInfo.p.x < -1.0f || hInfo.p.x > 1.0f || hInfo.p.y < -1.0f || hInfo.p.y > 1.0f)
            return false;

        hInfo.front = (ray.dir.Dot(planeNorm) < 0);
        if ((hInfo.front && (hitSide & HIT_FRONT)) || (!hInfo.front && (hitSide & HIT_BACK)))
            return true;
    }

    return false;
}

bool Plane::ShadowRay(Ray const& ray, float t_max) const {
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

    if (TraceBVHNode(ray, temphit, hitSide, bvh.GetRootNodeID()))
    {
		hInfo = temphit;
        return true;
    }

    return false;
}


bool TriObj::ShadowRay(Ray const& ray, float t_max) const {
    return TraceBVHNodeShadow(ray, t_max, bvh.GetRootNodeID());
}

/*
* Moller-Trumbore intersection algorithm
*/
bool TriObj::IntersectTriangle(Ray const& ray, HitInfo& hInfo, int hitSide, unsigned int faceID, cyVec2f& baryCoords) const {
    TriFace const& face = F(faceID);
    const float epsilon = 0.002f;

    // Fetch vertices only once
    const cyVec3f& v0 = V(face.v[0]);
    const cyVec3f& v1 = V(face.v[1]);
    const cyVec3f& v2 = V(face.v[2]);

    // Precompute edges
    cyVec3f edge1 = v1 - v0;
    cyVec3f edge2 = v2 - v0;

    // Use references to avoid temporaries
    const cyVec3f& dir = ray.dir;
    cyVec3f h = dir.Cross(edge2);
    float det = edge1.Dot(h);

    if (fabsf(det) < epsilon) return false;

    float inv_det = 1.0f / det;
    cyVec3f s = ray.p - v0;
    float u = inv_det * s.Dot(h);

    if (u < 0.0f || u > 1.0f) return false;

    cyVec3f s_cross_e1 = s.Cross(edge1);
    float v = inv_det * dir.Dot(s_cross_e1);

    if (v < 0.0f || (u + v) > 1.0f) return false;

    float t = inv_det * edge2.Dot(s_cross_e1);

    if (t <= epsilon || t >= hInfo.z) return false;

    hInfo.z = t;
    baryCoords.Set(u, v);
    return true;
}

bool TriObj::TraceBVHNode(Ray const& ray, HitInfo& hInfo, int hitSide, unsigned int nodeID) const {
    float const* bounds = bvh.GetNodeBounds(nodeID);
    cyVec3f boxMin(bounds[0], bounds[1], bounds[2]);
    cyVec3f boxMax(bounds[3], bounds[4], bounds[5]);

    if (!hitAABB(ray, boxMin, boxMax)) {
        return false;
    }

    // Check if this is a leaf node
    if (bvh.IsLeafNode(nodeID)) {
        bool hit = false;
        unsigned int elementCount = bvh.GetNodeElementCount(nodeID);
        unsigned int const* elements = bvh.GetNodeElements(nodeID);
        int closestFace = INT_MAX;
        cyVec2f baryCoords(0.0f, 0.0f);
		HitInfo tempHit;
        tempHit.Init();
        cyVec2f tempBary;

        // Test ray against each triangle in this leaf node
        for (unsigned int i = 0; i < elementCount; i++) 
        {
            unsigned int triangleIndex = elements[i];

            if (IntersectTriangle(ray, tempHit, hitSide, triangleIndex, tempBary)) 
            {
                hit = true;
                if (tempHit.z >= hInfo.z) continue;
				hInfo.z = tempHit.z;
                closestFace = triangleIndex;
                baryCoords = tempBary;
            }
        }
		if (!hit) return false;

        TriFace textureFace = FT(closestFace);
		TriFace normalFace = FN(closestFace);
		float u = baryCoords.x;
		float v = baryCoords.y;
		float w = 1.0f - u - v;
        cyVec3f uvw = (vt[textureFace.v[0]] * w) + 
                      (vt[textureFace.v[1]] * u) + 
                      (vt[textureFace.v[2]] * v);

        hInfo.N = (w * vn[normalFace.v[0]] +
            u * vn[normalFace.v[1]] +
            v * vn[normalFace.v[2]]).GetNormalized();

        hInfo.p = ray.p + ray.dir * hInfo.z;
        hInfo.uvw = uvw;
        hInfo.z = tempHit.z;
        hInfo.front = ray.dir.Dot(hInfo.N) < 0;

        return true;
    }
    else {
        unsigned int child1, child2;
        bvh.GetChildNodes(nodeID, child1, child2);

        HitInfo hitInfo1, hitInfo2;
        bool hit1 = TraceBVHNode(ray, hitInfo1, hitSide, child1);
        bool hit2 = TraceBVHNode(ray, hitInfo2, hitSide, child2);
        if(hitInfo1.z < hitInfo2.z) {
            hInfo = hitInfo1;
            return hit1;
        } else {
            hInfo = hitInfo2;
            return hit2;
		}
    }
}

bool TriObj::TraceBVHNodeShadow(Ray const& ray, float t_max, unsigned int nodeID) const {
    float const* bounds = bvh.GetNodeBounds(nodeID);
    cyVec3f boxMin(bounds[0], bounds[1], bounds[2]);
    cyVec3f boxMax(bounds[3], bounds[4], bounds[5]);
    if (!hitAABB(ray, boxMin, boxMax)) {
        return false;
    }
    // Check if this is a leaf node
    if (bvh.IsLeafNode(nodeID)) {
        unsigned int elementCount = bvh.GetNodeElementCount(nodeID);
        unsigned int const* elements = bvh.GetNodeElements(nodeID);
        HitInfo tempHit;
        tempHit.Init();
        cyVec2f tempBary;
        // Test ray against each triangle in this leaf node
        for (unsigned int i = 0; i < elementCount; i++)
        {
            unsigned int triangleIndex = elements[i];
            if (IntersectTriangle(ray, tempHit, HIT_FRONT_AND_BACK, triangleIndex, tempBary))
            {
                if (tempHit.z < t_max) {
                    return true;
                }
            }
        }
        return false;
    }
    else {
        unsigned int child1, child2;
        bvh.GetChildNodes(nodeID, child1, child2);
        bool hit1 = TraceBVHNodeShadow(ray, t_max, child1);
        if (hit1) return true;
        bool hit2 = TraceBVHNodeShadow(ray, t_max, child2);
        return hit2;
    }
}