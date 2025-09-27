//-------------------------------------------------------------------------------
///
/// \file       renderer.h 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    1.0
/// \date       September 19, 2025
///
/// \brief Project source for CS 6620 - University of Utah.
///
/// Copyright (c) 2025 Cem Yuksel. All Rights Reserved.
///
/// This code is provided for educational use only. Redistribution, sharing, or 
/// sublicensing of this code or its derivatives is strictly prohibited.
///
//-------------------------------------------------------------------------------

#ifndef _RENDERER_H_INCLUDED_
#define _RENDERER_H_INCLUDED_

//-------------------------------------------------------------------------------

#include "scene.h"
#include "lodepng.h"

//-------------------------------------------------------------------------------

class RenderImage
{
private:
	std::vector<Color24> img;
	std::vector<float>   zbuffer;
	std::vector<uint8_t> zbufferImg;
	int                  width=0, height=0;
	std::atomic<int>     numRenderedPixels=0;
public:
	void Init(int w, int h)
	{
		width = w;
		height = h;
		int size = width * height;
		img.resize(size);
		zbuffer.resize(size);
		for ( int i=0; i<size; ++i ) zbuffer[i] = BIGFLOAT;
		zbufferImg.resize(size);
		ResetNumRenderedPixels();
	}

	int      GetWidth  () const { return width; }
	int      GetHeight () const { return height; }
	Color24* GetPixels ()       { return img.data(); }
	float*   GetZBuffer()       { return zbuffer.data(); }
	uint8_t* GetZBufferImage()  { return zbufferImg.data(); }

	void ResetNumRenderedPixels ()        { numRenderedPixels=0; }
	int  GetNumRenderedPixels   () const  { return numRenderedPixels; }
	bool IsRenderDone           () const  { return numRenderedPixels >= width*height; }
	void IncrementNumRenderPixel( int n ) { numRenderedPixels+=n; }

	void ComputeZBufferImage()
	{
		int size = width * height;
		float zmin=BIGFLOAT, zmax=0;
		for ( int i=0; i<size; i++ ) {
			if ( zbuffer[i] == BIGFLOAT ) continue;
			if ( zmin > zbuffer[i] ) zmin = zbuffer[i];
			if ( zmax < zbuffer[i] ) zmax = zbuffer[i];
		}
		for ( int i=0; i<size; i++ ) {
			if ( zbuffer[i] == BIGFLOAT ) zbufferImg[i] = 0;
			else {
				float f = (zmax-zbuffer[i])/(zmax-zmin);
				int c = int(f * 255);
				zbufferImg[i] = c < 0 ? 0 : ( c > 255 ? 255 : c );
			}
		}
	}

	bool SaveImage ( char const *filename ) const { return lodepng::encode(filename,&img[0].r,     width,height,LCT_RGB, 8) == 0; }
	bool SaveZImage( char const *filename ) const { return lodepng::encode(filename,&zbufferImg[0],width,height,LCT_GREY,8) == 0; }
};

//-------------------------------------------------------------------------------

class Renderer
{
protected:
	Scene       scene;
	Camera      camera;
	RenderImage renderImage;
	std::string sceneFile;
	bool isRendering = false;

public:
	Scene &             GetScene      ()       { return scene; }
	Scene const &       GetScene      () const { return scene; }
	Camera &            GetCamera     ()       { return camera; }
	Camera const &      GetCamera     () const { return camera; }
	RenderImage &       GetRenderImage()       { return renderImage; }
	RenderImage const & GetRenderImage() const { return renderImage; }

	virtual bool LoadScene( char const *sceneFilename );
	std::string const & SceneFileName() const { return sceneFile; }

	virtual void BeginRender() {}	// Generates one or more rendering threads and begins rendering. Returns immediately.
	virtual void StopRender () {}	// Stops the current rendering process. It should wait till rendering threads stop.
	bool IsRendering() const { return isRendering; }

	virtual bool TraceRay( Ray const &ray, HitInfo &hInfo, int hitSide=HIT_FRONT_AND_BACK ) const { return false; }
};

//-------------------------------------------------------------------------------

void ShowViewport( Renderer *renderer, bool beginRendering=false );

//-------------------------------------------------------------------------------

#endif