#pragma once
///
/// \file       renderer.h 
/// \author     Devin Fink
/// \version    1.0
/// \date       September 27, 2025
///
/// \brief PImplementation of Renderer Class

#include "renderer.h"
#include "rng.h"

class RayTracer : public Renderer
{
	public:
		const int bounceCount = 5;
		RayTracer() {}
		~RayTracer() {}
		void BeginRender() override;
		void StopRender() override;

		bool TraceRay(Ray const& ray, HitInfo& hInfo, int hitSide = HIT_FRONT_AND_BACK) const override;
		bool TraceShadowRay(Ray const& ray, float t_max, int hitSide = HIT_FRONT_AND_BACK) const override;
		bool TraverseTree(const Ray& ray, const Node* node, HitInfo& hitInfo, int hitSide) const;
		bool TraverseTreeShadow(const Ray& ray, const Node* node, float t_max) const;
		void CreateCam2Wrld();
		Color CreateRay(int i, cyVec3f pixelPos, cyVec2f scrPos);

	private:
		const int tileSize = 64;
		std::atomic<int> nextTile{ 0 };
		float tValues[71] = { 0, 12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228,
								   2.201, 2.179, 2.160, 2.145, 2.131, 2.120, 2.110, 2.101, 2.093, 2.086,
								   2.080, 2.074, 2.069, 2.064, 2.060, 2.056, 2.052, 2.048, 2.045, 2.042,
								   2.040, 2.037, 2.035, 2.032, 2.030, 2.028, 2.026, 2.024, 2.023, 2.021,
								   2.020, 2.018, 2.017, 2.015, 2.014, 2.013, 2.012, 2.011, 2.010, 2.009,
								   2.000, 2.000, 2.000, 2.000, 2.000, 2.000, 2.000, 2.000, 2.000, 2.000,
								   1.994, 1.994, 1.994, 1.994, 1.994, 1.994, 1.994, 1.994, 1.994, 1.994 };
		cyMatrix4f cam2Wrld{};
		void RunThread(std::atomic<int>& nextTile, int totalTiles, int tilesX, int tilesY);
};