#pragma once

#include <vector>
#include <iostream>
#include "scene.h"
#include "OpenImageDenoise/oidn.hpp"

class Denoiser
{
public:
    Denoiser(int width, int height);
    ~Denoiser() = default;

    void Denoise(Color* inputColor, Color* outputColor);

    void Denoise(Color* inputColor, Color* outputColor,
        Color* albedo, Vec3f* normals);

    oidn::Error GetError() { return device.getError(); }

private:
    int width, height;
    oidn::DeviceRef device;
};