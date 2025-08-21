#include "raytracer.h"
#include <scene.h>
#include "viewport.h"

void BeginRender(RenderScene* scene)
{
	scene->renderImage.ResetNumRenderedPixels();
	int scrHeight = scene->renderImage.GetHeight();
	int scrWidth = scene->renderImage.GetWidth();
	int scrSize = scrHeight * scrWidth;

	Color24* initialPixel = scene->renderImage.GetPixels();

	for(int pixel = 0; pixel < scrSize; pixel++)
	{
		int x = pixel % scrWidth;     // column
		int y = pixel / scrWidth;     // row


        /*
		float u = float(x) / float(scrWidth - 1);
		float v = float(y) / float(scrHeight - 1);

        // Corner colors (R,G,B) in [0,1]
        float topLeft[3] = { 0.0f, 0.0f, 1.0f }; // blue
        float topRight[3] = { 1.0f, 0.0f, 1.0f }; // magenta
        float bottomLeft[3] = { 0.0f, 1.0f, 1.0f }; // cyan
        float bottomRight[3] = { 1.0f, 1.0f, 1.0f }; // white

        // bilinear interpolation
        float top[3] = {
            (1 - u) * topLeft[0] + u * topRight[0],
            (1 - u) * topLeft[1] + u * topRight[1],
            (1 - u) * topLeft[2] + u * topRight[2]
        };

        float bottom[3] = {
            (1 - u) * bottomLeft[0] + u * bottomRight[0],
            (1 - u) * bottomLeft[1] + u * bottomRight[1],
            (1 - u) * bottomLeft[2] + u * bottomRight[2]
        };

        float color[3] = {
            (1 - v) * top[0] + v * bottom[0],
            (1 - v) * top[1] + v * bottom[1],
            (1 - v) * top[2] + v * bottom[2]
        };
        */

		initialPixel[pixel] = Color24(color[0] * 255, color[1] * 255, color[2] * 255);
		scene->renderImage.IncrementNumRenderPixel(1);
	}
}

void StopRender()
{
	return;
}
