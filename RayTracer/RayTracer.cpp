#define _USE_MATH_DEFINES
#include "raytracer.h"
#include <cmath>
#include <iostream>
#include <scene.h>
#include "viewport.h"

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

	Color24* pixels = scene->renderImage.GetPixels();

	for(int pixel = 0; pixel < scrSize; pixel++)
	{
		int i = pixel % scrWidth;
		int j = pixel / scrWidth;

		//Pixel Vector Calculatiion
		cyVec3f pixelPos = cyVec3f((-(wrldImgWidth / 2.0f) + (wrldImgWidth / camWidthRes) * (i + (1.0f / 2.0f))),    //x
									((wrldImgHeight / 2.0f) - (wrldImgHeight / camHeightRes) * (j + (1.0f / 2.0f))), //y
								  (l));																             //z

		//Ray Generation
		cyVec3f rayDir = pixelPos - scene->camera.pos;
		Ray ray = Ray(scene->camera.pos, rayDir);
		ray.dir = cyVec3f(cam2Wrld * cyVec4f(ray.dir, 1));
		ray.p = cyVec3f(cam2Wrld * cyVec4f(ray.p, 1));

		HitInfo hit;
		hit.Init();

		if (TraverseTree(ray, &scene->rootNode, hit))
		{
			pixels[pixel] = Color24(255, 255, 255);
		}
		else
		{
			pixels[pixel] = Color24(0, 0, 0);
		}

		scene->renderImage.GetZBuffer()[pixel] = hit.z;
		scene->renderImage.IncrementNumRenderPixel(1);
	}

	scene->renderImage.ComputeZBufferImage();
	scene->renderImage.SaveZImage("projectOneZ.png");
	scene->renderImage.SaveImage("projectOne.png");
}

void StopRender()
{
	return;
}
