#include "denoiser.h"

Denoiser::Denoiser(int width, int height) : width(width), height(height)
{
    std::cout << "Creating OIDN CPU device..." << std::endl;

    device = oidn::newDevice(oidn::DeviceType::CPU);  // Force CPU

    oidn::Error error = device.getError();
    if ((int)error != 0) {
        std::cerr << "OIDN Error: " << (int)error << std::endl;
    }

    device.commit();

    error = device.getError();
    if ((int)error != 0) {
        std::cerr << "OIDN Error: " << (int)error << std::endl;
    }
}

void Denoiser::Denoise(Color* inputColor, Color* outputColor)
{
    std::vector<float> colorBuffer(width * height * 3);

    // Convert Color to float RGB
    for (int i = 0; i < width * height; i++) {
        colorBuffer[i * 3 + 0] = inputColor[i].r;
        colorBuffer[i * 3 + 1] = inputColor[i].g;
        colorBuffer[i * 3 + 2] = inputColor[i].b;
    }

    std::vector<float> outputBuffer(width * height * 3);


    //Ray Tracing Filter
    oidn::FilterRef filter = device.newFilter("RT");

    filter.setImage("color", colorBuffer.data(), oidn::Format::Float3, width, height);
    filter.setImage("output", outputBuffer.data(), oidn::Format::Float3, width, height);

    filter.set("ldr", true);
    filter.set("srgb", true);
    filter.commit();

    filter.execute();std::cout << "First pixel before denoise: "
        << colorBuffer[0] << ", " << colorBuffer[1] << ", " << colorBuffer[2] << std::endl;

    filter.execute();

    std::cout << "First pixel after denoise: "
        << outputBuffer[0] << ", " << outputBuffer[1] << ", " << outputBuffer[2] << std::endl;

    oidn::Error error = device.getError();
    if ((int)error != 0) {
        std::cerr << "OIDN Error: " << (int)error << std::endl;
    }

    for (int i = 0; i < width * height; i++) {
        outputColor[i].r = outputBuffer[i * 3 + 0];
        outputColor[i].g = outputBuffer[i * 3 + 1];
        outputColor[i].b = outputBuffer[i * 3 + 2];
    }
}



