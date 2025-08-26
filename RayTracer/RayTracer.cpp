#define _USE_MATH_DEFINES
#include "raytracer.h"
#include <cmath>
#include <iostream>
#include <scene.h>
#include "viewport.h"
#include <thread>

#define DEG2RAD(degrees) ((degrees) * M_PI / 180.0)

Color24 RayColor(const Ray& r) {
	auto a = 0.5 * (r.dir + 1.0);
	cyVec3f colorVector = (1.0 - a) * cyVec3f(1.0, 1.0, 1.0) + a * cyVec3f(0.5, 0.7, 1.0);
	Color24 color = Color24(colorVector.x, colorVector.y, colorVector.z);
	return color;
}

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
		}
	}

	// Traverse children
	for (int i = 0; i < node->GetNumChild(); i++)
	{
		if (TraverseTree(transformedRay, node->GetChild(i), hitInfo))
			hit = true;
	}

	return hit;
}

cyMatrix4f CreateCam2Wrld(RenderScene* scene)
{
	cyVec3f cam2WrldZ = scene->camera.dir;
	cyVec3f cam2WrldY = scene->camera.up;
	cyVec3f cam2WrldX = cam2WrldZ.Cross(cam2WrldY);

	cyMatrix4f cam2Wrld = cyMatrix4f(cyVec4f(cam2WrldX, 0), cyVec4f(cam2WrldY, 0), cyVec4f(cam2WrldZ, 0), cyVec4f(scene->camera.pos, 1));
	return cam2Wrld.GetTranspose();
}

void trace_ray(RenderScene* scene, int pixel, cyMatrix4f& cam2Wrld, cyVec3f pixelPos)
{
	//Ray Generation
	cyVec3f rayDir = pixelPos - scene->camera.pos;
	Ray ray = Ray(scene->camera.pos, rayDir);
	ray.dir = cyVec3f(cam2Wrld * cyVec4f(ray.dir, 1));
	ray.p = cyVec3f(cam2Wrld * cyVec4f(ray.p, 1));

	HitInfo hit;
	hit.Init();

	if (TraverseTree(ray, &scene->rootNode, hit))
	{
		scene->renderImage.GetPixels()[pixel] = Color24(255, 255, 255);
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

	int l = 1;

	float wrldImgHeight = 2.0f * l * tan((DEG2RAD(scene->camera.fov)) / 2.0f);
	float wrldImgWidth = wrldImgHeight * (camWidthRes / camHeightRes);

	cyMatrix4f cam2Wrld = CreateCam2Wrld(scene);

	//Multithreading
	uint32_t num_threads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	threads.reserve(num_threads);
	int chunk = 128;
	std::atomic<int> nextPixel{ 0 };


	//runThread Lambda
	auto runThread = [&]()
	{
		for (;;)
		{
			int begin = nextPixel.fetch_add(chunk, std::memory_order_relaxed);
			if (begin >= scrSize) break;
			int end = std::min<int>(begin + chunk, scrSize);

			for (int i = begin; i < end; i++)
			{
				int x = i % scrWidth;
				int y = i / scrWidth;

				//Pixel Vector Calculatiion
				cyVec3f pixelPos = cyVec3f((-(wrldImgWidth / 2.0f) + (wrldImgWidth / camWidthRes) * (x + (1.0f / 2.0f))),    //x
										    ((wrldImgHeight / 2.0f) - (wrldImgHeight / camHeightRes) * (y + (1.0f / 2.0f))), //y
										  (l));																			 //z

				trace_ray(scene, i, cam2Wrld, pixelPos);
			}
		}
	};

	for (int t = 0; t < num_threads; t++)
		threads.emplace_back(runThread);
	for (auto& th : threads) th.join();

	scene->renderImage.ComputeZBufferImage();
	scene->renderImage.SaveZImage("projectOneZ.png");
	scene->renderImage.SaveImage("projectOne.png");
}

void StopRender()
{
	return;
}
