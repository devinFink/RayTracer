// RayTracer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "xmlload.h"
#include "objects.h"
#include "raytracer.h"

int main()
{
	Renderer* theRenderer = new RayTracer();
	theRenderer->LoadScene("project_1_scene.xml");
    ShowViewport(theRenderer);
}
