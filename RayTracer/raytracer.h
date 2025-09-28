#pragma once
///
/// \file       renderer.h 
/// \author     Devin Fink
/// \version    1.0
/// \date       September 27, 2025
///
/// \brief PImplementation of Renderer Class

#include "renderer.h"
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
		void CreateRay(int i, cyVec3f pixelPos);

	private:
		const int tileSize = 32;
		std::atomic<int> nextTile{ 0 };
		cyMatrix4f cam2Wrld{};
		void RunThread(std::atomic<int>& nextTile, int totalTiles, int tilesX, int tilesY);
};