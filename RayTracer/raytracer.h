#pragma once
#include "scene.h"

extern Node* treeRoot;

bool TraverseTree(const Ray& ray, Node* node, HitInfo& hit, int hitSide = HIT_FRONT);
bool TraverseTreeShadow(const Ray& ray, Node* node, float t_max);
cyMatrix4f CreateCam2Wrld(RenderScene* scene);
