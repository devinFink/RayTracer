#pragma once
#include "scene.h"

extern Node* treeRoot;

bool TraverseTree(const Ray& ray, Node* node, HitInfo& hit, int hitSide = HIT_FRONT);
cyMatrix4f CreateCam2Wrld(RenderScene* scene);
