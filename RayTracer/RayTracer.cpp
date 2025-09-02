#define _USE_MATH_DEFINES
#include "raytracer.h"
#include "viewport.h"
#include "lights.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <algorithm>

#define DEG2RAD(degrees) ((degrees) * M_PI / 180.0)
Node* treeRoot = nullptr;

bool TraverseTree(const Ray& ray, Node* node, HitInfo& hitInfo)
{
	if (!node) return false;
	bool hit = false;
	Ray transformedRay = node->ToNodeCoords(ray);

	// Check current node's object
	const Object* obj = node->GetNodeObj();
	if (obj)
	{
		if (obj->IntersectRay(transformedRay, hitInfo))
		{
			hit = true;
			node->FromNodeCoords(hitInfo);
			hitInfo.node = node;
		}
	}

	// Traverse children
	for (int i = 0; i < node->GetNumChild(); i++)
	{
		if (TraverseTree(transformedRay, node->GetChild(i), hitInfo))
		{
			hit = true;
			node->FromNodeCoords(hitInfo);
		}
	}

	return hit;
}

cyMatrix4f CreateCam2Wrld(RenderScene* scene)
{
	cyVec3f cam2WrldZ = -scene->camera.dir;
	cyVec3f cam2WrldY = scene->camera.up;
	cyVec3f cam2WrldX = cam2WrldY.Cross(cam2WrldZ);

	cyMatrix4f cam2Wrld = cyMatrix4f(cam2WrldX, cam2WrldY, cam2WrldZ, scene->camera.pos);
	return cam2Wrld;
}

void TraceRay(RenderScene* scene, int pixel, cyMatrix4f& cam2Wrld, cyVec3f pixelPos)
{
	//Ray Generation
	Ray ray = Ray(scene->camera.pos, pixelPos);
	ray.dir = cyVec3f(cam2Wrld * cyVec4f(ray.dir, 0));

	HitInfo hit;
	hit.Init();

	if (TraverseTree(ray, &scene->rootNode, hit))
	{
		if (hit.node->GetMaterial())
		{
			scene->renderImage.GetPixels()[pixel] = (Color24)hit.node->GetMaterial()->Shade(ray, hit, scene->lights);
		}
		else
		{
			scene->renderImage.GetPixels()[pixel] = Color24(255, 255, 255);
		}
	}
	else
	{
		scene->renderImage.GetPixels()[pixel] = Color24(0, 0, 0);
	}

	
	scene->renderImage.IncrementNumRenderPixel(1);
	scene->renderImage.GetZBuffer()[pixel] = hit.z;
}

void BeginRender(RenderScene* scene)
{
	scene->renderImage.ResetNumRenderedPixels();
	const int scrHeight = scene->renderImage.GetHeight();
	const int scrWidth = scene->renderImage.GetWidth();
	const float camWidthRes = scene->camera.imgWidth;
	const float camHeightRes = scene->camera.imgHeight;
	const int scrSize = scrHeight * scrWidth;
	treeRoot = &scene->rootNode;

	int l = 1;

	float wrldImgHeight = 2.0f * l * tan((DEG2RAD(scene->camera.fov)) / 2.0f);
	float wrldImgWidth = wrldImgHeight * (camWidthRes / camHeightRes);

	cyMatrix4f cam2Wrld = CreateCam2Wrld(scene);

	//Multithreading
	const int numThreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	threads.reserve(numThreads);
	constexpr int tileSize = 16;
	const int tilesX = (camWidthRes + tileSize - 1) / tileSize;
	const int tilesY = (camHeightRes + tileSize - 1) / tileSize;
	const int totalTiles = tilesX * tilesY;
	std::atomic<int> nextTile{ 0 };


	//runThread Lambda
	auto runThread = [&]
	{
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
												 (-l));                                                                           //z

					TraceRay(scene, i, cam2Wrld, pixelPos);
				}
			}
		}
	};

	for (int t = 0; t < numThreads; t++)
		threads.emplace_back(runThread);
	for (auto& th : threads) th.join();

	scene->renderImage.ComputeZBufferImage();
	scene->renderImage.SaveZImage("projectThreeZ.png");
	scene->renderImage.SaveImage("projectThree.png");
}

void StopRender()
{
	return;
}
