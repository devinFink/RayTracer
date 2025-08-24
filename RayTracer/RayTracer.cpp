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


void BeginRender(RenderScene* scene)
{
	scene->renderImage.ResetNumRenderedPixels();
	const int scrHeight = scene->renderImage.GetHeight();
	const int scrWidth = scene->renderImage.GetWidth();
	const float camWidthRes = scene->camera.imgWidth;
	const float camHeightRes = scene->camera.imgHeight;

	int l = 1;

	float wrldImgHeight = 2.0f * l * tan((DEG2RAD(scene->camera.fov)) / 2.0f);
	float wrldImgWidth = wrldImgHeight * (camWidthRes / camHeightRes);

	const int scrSize = scrHeight * scrWidth;

	// Camera To World Transformation Matrix Creation ----------
	cyVec3f cam2WrldZ = scene->camera.dir;
	cyVec3f cam2WrldY = scene->camera.up;
	cyVec3f cam2WrldX = cam2WrldZ.Cross(cam2WrldY);

	cyMatrix4f cam2Wrld = cyMatrix4f(cyVec4f(cam2WrldX, 0), cyVec4f(cam2WrldY, 0), cyVec4f(cam2WrldZ, 0), cyVec4f(scene->camera.pos, 1));
	cam2Wrld = cam2Wrld.GetTranspose();

	// ----------------------------------------------------------


	Color24* pixels = scene->renderImage.GetPixels();

	for(int pixel = 0; pixel < scrSize; pixel++)
	{
		int i = pixel % scrWidth;
		int j = pixel / scrWidth;


		cyVec3f pixelPos = cyVec3f((-(wrldImgWidth / 2.0f) + (wrldImgWidth / camWidthRes) * (i + (1.0f / 2.0f))),    //x
									((wrldImgHeight / 2.0f) - (wrldImgHeight / camHeightRes) * (j + (1.0f / 2.0f))), //y
								  (l));																             //z


		cyVec3f rayDir = pixelPos - scene->camera.pos;
		Ray ray = Ray(scene->camera.pos, rayDir);
		ray.dir = cyVec3f(cam2Wrld * cyVec4f(ray.dir, 1));
		ray.p = cyVec3f(cam2Wrld * cyVec4f(ray.p, 1));

		ray.Normalize();


		if(TraverseTree(ray, &scene->rootNode))
		{
			pixels[pixel] = Color24(255, 255, 255);
		}
		else
		{
			pixels[pixel] = Color24(0, 0, 0);
		}

		scene->renderImage.IncrementNumRenderPixel(1);
	}
}

bool TraverseTree(const Ray &ray, Node* node)
{
	if (!node) return false;

	HitInfo hitInfo;
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
		if (TraverseTree(transformedRay, node->GetChild(i)))
			hit = true;
	}

	return hit;
}

void StopRender()
{
	return;
}
