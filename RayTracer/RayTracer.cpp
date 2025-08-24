#include "raytracer.h"
#include <scene.h>
#include "viewport.h"


Color24 ray_color(const Ray& r) {
	return Color24(0, 0, 0);
}


void BeginRender(RenderScene* scene)
{
	scene->renderImage.ResetNumRenderedPixels();
	const int scrHeight = scene->renderImage.GetHeight();
	const int scrWidth = scene->renderImage.GetWidth();
	const int camWidthRes = scene->camera.imgWidth;
	const int camHeightRes = scene->camera.imgHeight;

	int l = 1;

	float wrldImgHeight = 2 * l * tan(scene->camera.fov / 2);
	float wrldImgWidth = wrldImgHeight * (camHeightRes / camWidthRes);

	int scrSize = scrHeight * scrWidth;

	// Camera To World Transformation Matrix Creation ----------
	cyVec3f cam2WrldZ = cyVec3f(0, 0, scene->camera.dir.y);
	cyVec3f cam2WrldY = cyVec3f(0, scene->camera.up.z, 0);
	cyVec3f cam2WrldX = cam2WrldZ.Cross(cam2WrldY);

	cyMatrix4f cam2Wrld = cyMatrix4f(cam2WrldX, cam2WrldY, cam2WrldZ, scene->camera.pos);
	cam2Wrld.Transpose();
	// ----------------------------------------------------------


	Color24* pixels = scene->renderImage.GetPixels();

	for(int pixel = 0; pixel < scrSize; pixel++)
	{
		int i = pixel % scrWidth;
		int j = pixel / scrWidth;

		cyVec3f pixelPos = cyVec3f((-(wrldImgWidth / 2) + (wrldImgWidth / camWidthRes) * (i + (1 / 2))),    //x
									((wrldImgHeight / 2) - (wrldImgHeight / camHeightRes) * (j + (1 / 2))), //y
								  (-l));																    //z

		scene->renderImage.IncrementNumRenderPixel(1);
	}
}

void StopRender()
{
	return;
}
