#define _USE_MATH_DEFINES
#include <cmath>
#include "raytracer.h"
#include "objects.h"
#include "shadowInfo.h"
#include <iostream>
#include <thread>
#include <atomic>

#define DEG2RAD(degrees) ((degrees) * M_PI / 180.0)

void RayTracer::CreateCam2Wrld()
{
	cyVec3f cam2WrldZ = -camera.dir;
	cyVec3f cam2WrldY = camera.up;
	cyVec3f cam2WrldX = cam2WrldY.Cross(cam2WrldZ);

	this->cam2Wrld = cyMatrix4f(cam2WrldX, cam2WrldY, cam2WrldZ, camera.pos);
}

void RayTracer::RunThread(std::atomic<int>& nextTile, int totalTiles, int tilesX, int tilesY)
{
	const int scrHeight = renderImage.GetHeight();
	const int scrWidth = renderImage.GetWidth();
	const float camWidthRes = camera.imgWidth;
	const float camHeightRes = camera.imgHeight;
	float wrldImgHeight = 2.0f * 1 * tan((DEG2RAD(camera.fov)) / 2.0f);
	float wrldImgWidth = wrldImgHeight * (camWidthRes / camHeightRes);

	for (;;)
	{
		const int tileIndex = nextTile.fetch_add(1, std::memory_order_relaxed);
		if (tileIndex >= totalTiles) break;

		const int tileX = tileIndex % tilesX;
		const int tileY = tileIndex / tilesX;

		//Calculate tile coords
		const int x0 = tileX * tileSize;
		const int y0 = tileY * tileSize;
		const int x1 = std::min(x0 + tileSize, scrWidth);
		const int y1 = std::min(y0 + tileSize, scrHeight);

		for (int y = y0; y < y1; ++y) {
			for (int x = x0; x < x1; ++x) {

				int i = y * scrWidth + x;

				cyVec3f pixelPos = cyVec3f((-(wrldImgWidth / 2.0f) + (wrldImgWidth / camWidthRes) * (x + (1.0f / 2.0f))),    //x
					((wrldImgHeight / 2.0f) - (wrldImgHeight / camHeightRes) * (y + (1.0f / 2.0f))), //y
					(-1));                                                                           //z

				cyVec2f scrPos = cyVec2f((float)x, (float)y);
				CreateRay(i, pixelPos, scrPos);
			}
		}
	}
}

void RayTracer::BeginRender() 
{
	renderImage.ResetNumRenderedPixels();
	CreateCam2Wrld();

	//Multithreading
	const int numThreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	threads.reserve(numThreads);
	const int tilesX = (camera.imgWidth + tileSize - 1) / tileSize;
	const int tilesY = (camera.imgHeight + tileSize - 1) / tileSize;
	const int totalTiles = tilesX * tilesY;

	for (int t = 0; t < numThreads; t++)
		threads.emplace_back([this, totalTiles, tilesX, tilesY]() { RunThread(this->nextTile, totalTiles, tilesX, tilesY); });
	for (auto& th : threads) th.detach();

	//while(!renderImage.IsRenderDone())
	//{
	//	continue;
	//}
	//renderImage.ComputeZBufferImage();
	//renderImage.SaveZImage("testZ.png");
	//renderImage.SaveImage("prj_6.png");
}

void RayTracer::StopRender() 
{
	renderImage.ComputeZBufferImage();
	renderImage.SaveZImage("testZ.png");
	renderImage.SaveImage("testSpace.png");
}

void RayTracer::CreateRay(int index, cyVec3f pixelPos, cyVec2f scrPos)
{
	//Ray Generation
	Ray ray = Ray(camera.pos, pixelPos);
	ray.dir = cyVec3f(cam2Wrld * cyVec4f(ray.dir, 0));

	HitInfo hit;
	hit.Init();
	hit.node = &scene.rootNode;

	if (TraceRay(ray, hit, HIT_FRONT))
	{
		ShadowInfo info = ShadowInfo(scene.lights, scene.environment, this);
		info.SetHit(ray, hit);

		if (hit.node->GetMaterial())
		{
			renderImage.GetPixels()[index] = (Color24)hit.node->GetMaterial()->Shade(info);
		}
		else
		{
			renderImage.GetPixels()[index] = Color24(255, 255, 255);
		}
	}
	else
	{
		float u = scrPos.x / (float)camera.imgWidth;
		float v = scrPos.y / (float)camera.imgHeight;
		Color color = scene.background.Eval(Vec3f(u, v, 0.0));
		renderImage.GetPixels()[index] = (Color24)color;
	}


	renderImage.IncrementNumRenderPixel(1);
	renderImage.GetZBuffer()[index] = hit.z;
}

bool RayTracer::TraceRay(Ray const& ray, HitInfo& hInfo, int hitSide) const
{
	return TraverseTree(ray, &scene.rootNode, hInfo, hitSide);
}

bool RayTracer::TraceShadowRay(Ray const& ray, float t_max, int hitSide) const
{
	return TraverseTreeShadow(ray, &scene.rootNode, t_max);
}

bool RayTracer::TraverseTreeShadow(const Ray& ray, const Node* node, float t_max) const
{
	if (!node) return false;
	Ray transformedRay = node->ToNodeCoords(ray);

	// Check current node's object
	const Object* obj = node->GetNodeObj();
	if (obj)
	{
		if (obj->ShadowRay(transformedRay, t_max))
		{
			return true;
		}
	}

	// Traverse children
	for (int i = 0; i < node->GetNumChild(); i++)
	{
		if (TraverseTreeShadow(transformedRay, node->GetChild(i), t_max))
		{
			return true;
		}
	}

	return false;
}

bool RayTracer::TraverseTree(const Ray& ray, const Node* node, HitInfo& hitInfo, int hitSide) const
{
	if (!node) return false;
	bool hit = false;
	Ray transformedRay = node->ToNodeCoords(ray);

	// Check current node's object
	const Object* obj = node->GetNodeObj();
	if (obj)
	{
		HitInfo localHit;
		localHit.Init();
		if (obj->IntersectRay(transformedRay, localHit, hitSide))
		{
			if (localHit.z < hitInfo.z)
			{
				hitInfo = localHit;
				hit = true;
				node->FromNodeCoords(hitInfo);
				hitInfo.node = node;
			}
		}
	}

	// Traverse children
	for (int i = 0; i < node->GetNumChild(); i++)
	{
		HitInfo localHit;
		localHit.Init();
		if (TraverseTree(transformedRay, node->GetChild(i), localHit, hitSide))
		{
			if(localHit.z < hitInfo.z)
			{
				hitInfo = localHit;
				hit = true;
				node->FromNodeCoords(hitInfo);
			}
		}
	}

	return hit;
}
