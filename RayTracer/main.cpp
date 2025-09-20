// RayTracer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <objects.h>

#include "viewport.h"
#include "xmlload.h"

RenderScene scene;

int main()
{
    LoadScene(scene, "scene_5.xml");
    Node* treeRoot = &scene.rootNode;
    ShowViewport(&scene);
}
