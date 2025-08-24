#pragma once
#include <scene.h>
bool TraverseTree(const Ray& ray, Node* node, HitInfo& hit);
cyMatrix4f CreateCam2Wrld(RenderScene* scene);