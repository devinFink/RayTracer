#define _USE_MATH_DEFINES
#include <cmath>
#include "raytracer.h"
#include "objects.h"
#include "shadowInfo.h"
#include "photonmap.h"
#include "denoiser.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>

#define DEG2RAD(degrees) ((degrees) * M_PI / 180.0)

void RayTracer::CreateCam2Wrld()
{
	cyVec3f cam2WrldZ = -camera.dir;
	cyVec3f cam2WrldY = camera.up;
	cyVec3f cam2WrldX = cam2WrldY.Cross(cam2WrldZ);

	this->cam2Wrld = cyMatrix4f(cam2WrldX, cam2WrldY, cam2WrldZ, camera.pos);
}

void RayTracer::BeginRender()
{
	renderImage.ResetNumRenderedPixels();
	CreateCam2Wrld();

	PhotonMap* pMap = new PhotonMap;
	pMap->Resize(numPhotons * scene.lights.size());

	GeneratePhotons(pMap);

	this->map = pMap;

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
}

void RayTracer::StopRender()
{
	while (!renderImage.IsRenderDone())
	{
		continue;
	}

	//Save Raw image for comparison
	renderImage.SaveImage("outputs/rawImage.png");

	// Denoise
	Denoiser denoiser(camera.imgWidth, camera.imgHeight);

	// Create output buffer
	std::vector<Color> denoisedPixels(camera.imgWidth * camera.imgHeight);

	// Convert Color24 to Color (float) for denoising
	Color* inputPixels = new Color[camera.imgWidth * camera.imgHeight];
	for (int i = 0; i < camera.imgWidth * camera.imgHeight; i++) {
		Color24 pixel = renderImage.GetPixels()[i];
		inputPixels[i] = Color(pixel);
	}

	// Denoise
	denoiser.Denoise(inputPixels, denoisedPixels.data());


	// Convert back to Color24
	for (int i = 0; i < camera.imgWidth * camera.imgHeight; i++) {
		renderImage.GetPixels()[i] = Color24(denoisedPixels[i]);
	}

	delete[] inputPixels;

	// Save images
	renderImage.ComputeZBufferImage();
	renderImage.SaveZImage("testZ.png");
	renderImage.SaveImage("outputs/singleBounceDenoise.png");
}

void RayTracer::RunThread(std::atomic<int>& nextTile, int totalTiles, int tilesX, int tilesY)
{
	float l = camera.focaldist;
	const int scrHeight = renderImage.GetHeight();
	const int scrWidth = renderImage.GetWidth();
	const float camWidthRes = camera.imgWidth;
	const float camHeightRes = camera.imgHeight;
	float wrldImgHeight = 2.0f * l * tan((DEG2RAD(camera.fov)) / 2.0f);
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

				int index = y * scrWidth + x;
				Color sumColor(0, 0, 0);
				Color sumColorSquared(0, 0, 0);

				//Precompute halton sequences up to a decent number
				HaltonSeq<128> haltonX(2);
				HaltonSeq<128> haltonY(3);
				HaltonSeq<128> haltonDiscX(5);
				HaltonSeq<128> haltonDiscY(7);
				RNG rng(index);
				float randomOffset = rng.RandomFloat();
				float randomOffsetY = rng.RandomFloat();
				int totalSamples = 0;


				//Adaptive Sampling loop
				for(int i = 0; i < maxSamples; i++)
				{
					float haltonValueX = haltonX[i] + randomOffset;
					if(haltonValueX > 1.0f)
						haltonValueX -= 1.0f;

					float haltonValueY = haltonY[i] + randomOffset;
					if(haltonValueY > 1.0f)
						haltonValueY -= 1.0f;

					//Find pixel location in world space
					float pixX = -(wrldImgWidth / 2.0f) + ((wrldImgWidth * (x + (1.0f / 2.0f)  + haltonValueX) / camWidthRes));
					float pixY = (wrldImgHeight / 2.0f) - ((wrldImgHeight * (y + (1.0f / 2.0f) + haltonValueY) / camHeightRes));
					cyVec3f pixelPos(pixX, pixY, -l);

					//Depth of Field
					float discX = haltonDiscX[i] + randomOffset;
					float discY = haltonDiscY[i] + randomOffsetY;
					if(discX > 1.0f)
						discX -= 1.0f;
					if (discY > 1.0f)
						discY -= 1.0f;

					//Sample camera disc
					float r = sqrt(discX);
					float angle = 2.0f * M_PI * discY;
					float lensU = r * camera.dof * cos(angle);
					float lensV = r * camera.dof * sin(angle);
					Vec3f cameraOffset = Vec3f(lensU, lensV, 0.0f);
					Vec3f worldCamera = cyVec3f(cam2Wrld * cyVec4f(cameraOffset, 0));
					Vec3f worldPixel = cyVec3f(cam2Wrld * cyVec4f(pixelPos, 0));
					Vec3f offsetCamera = camera.pos + worldCamera;
					cyVec3f rayDir = worldPixel - worldCamera;


					cyVec2f scrPos = cyVec2f((float)x, (float)y);

					//Ray Generation 
					Ray ray = Ray(offsetCamera, rayDir);

					Color tempColor = SendRay(i, ray, scrPos, rng);
					sumColor += tempColor;
					sumColorSquared += tempColor * tempColor;

					if (i >= minSamples)
					{
						//Adaptive Sampling Calculation
						float n = (float)(i + 1);
						Color mean = sumColor / n;
						Color meanSq = sumColor * sumColor;
						Color variance = (sumColorSquared - meanSq / n) / (n - 1.0f);
						variance.ClampMin(0.0f);
						Color stdDev = Sqrt(variance);
						float t = tValues[(int)n - 1]; // from table
						Color phi = t * (stdDev / sqrtf(n));
						float threshold = 0.01f;

						if (phi.r <= threshold && phi.g <= threshold && phi.b <= threshold)
						{
							totalSamples = i + 1;
							break;
						}
					}

					if (i == maxSamples)
					{
						totalSamples = i + 1;
					}
				}

				Color finalColor = sumColor / (float)totalSamples;

				if (camera.sRGB)
				{
					finalColor = finalColor.Linear2sRGB();
				}

				renderImage.GetPixels()[index] = Color24(finalColor);
				renderImage.IncrementNumRenderPixel(1);
				renderImage.GetZBuffer()[index] = 0;
				renderImage.GetSampleCount()[index] = totalSamples;

				if (renderImage.IsRenderDone())
				{
					StopRender();
				}
			}
		}
	}
}

Color RayTracer::SendRay(int index, Ray ray, cyVec2f scrPos, RNG rng)
{
	HitInfo hit;
	hit.Init();
	hit.node = &scene.rootNode;

	if (TraceRay(ray, hit, HIT_FRONT))
	{
		ShadowInfo info = ShadowInfo(scene.lights, scene.environment, rng, this);
		info.SetPixelSample(index);
		info.SetHit(ray, hit);

		if (!hit.light)
		{
			return Color(hit.node->GetMaterial()->Shade(info));
		}
		else {
			for (const auto& light : scene.lights)
			{
				if (light->IsRenderable() && light->IntersectRay(ray, hit, HIT_FRONT_AND_BACK))
				{
					return light->Radiance(info);
				}
			}
		}
	}
	else
	{
		float u = scrPos.x / (float)camera.imgWidth;
		float v = scrPos.y / (float)camera.imgHeight;
		return scene.background.Eval(Vec3f(u, v, 0.0));
	}
}


void RayTracer::GeneratePhotons(PhotonMap* pMap) {
	RNG rng;
	Ray ray;
	Color c;

	for (auto& light : scene.lights) {
		if (light->IsPhotonSource()) {
			while (pMap->RemainingSpace() > 0) {
				light->RandomPhoton(rng, ray, c);
				HitInfo info;
				TracePhoton(ray, info, c, pMap, true);
			}
		}
	}

	pMap->ScalePhotonPowers(1.0f / (float)numPhotons);
	pMap->PrepareForIrradianceEstimation();
}

bool RayTracer::TracePhoton(const Ray &ray, HitInfo& hInfo, Color& c, PhotonMap* pMap, bool first){
	RNG rng(pMap->NumPhotons());
	if (TraverseTree(ray, &scene.rootNode, hInfo, HIT_FRONT)) {

		if (hInfo.node) {
			if(!first && hInfo.node->GetMaterial()->IsPhotonSurface())
				pMap->AddPhoton(hInfo.p, ray.dir, c);

			SamplerInfo sInfo(rng);
			sInfo.SetHit(ray, hInfo);
			Vec3f newDir;
			DirSampler::Info si;

			if (hInfo.node->GetMaterial()->GenerateSample(sInfo, newDir, si))
			{
				c *= si.mult / si.prob;
				Ray photonRay(hInfo.p, newDir);
				TracePhoton(photonRay, hInfo, c, pMap, false);
			}


			return true;
		}
	}

	return false;
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

	if(node == &scene.rootNode)
	{
		for(const auto& light : scene.lights)
		{
			if (light->IsRenderable())
			{
				HitInfo localHit;
				localHit.Init();
				if (light->IntersectRay(ray, localHit, HIT_FRONT_AND_BACK))
				{
					if (localHit.z < hitInfo.z)
					{
						hitInfo = localHit;
						hit = true;
						hitInfo.light = true;
					}
				}
			}
		}
	}

	return hit;
}
