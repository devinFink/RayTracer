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
		RayTracer() {}
		~RayTracer() {}
		void BeginRender() override;
		void StopRender() override;

		bool TraceRay(Ray const& ray, HitInfo& hInfo, int hitSide = HIT_FRONT_AND_BACK) const override;
		void CreateCam2Wrld();
		void CreateRay(int i, cyVec3f pixelPos);

	private:
		const int tileSize = 32;
		std::atomic<int> nextTile{ 0 };
		cyMatrix4f cam2Wrld{};
		void RunThread(std::atomic<int>& nextTile, int totalTiles, int tilesX, int tilesY);
};