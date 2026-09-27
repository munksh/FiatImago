#ifndef IMAGING_H
#define IMAGING_H

#include <cstddef>
#include <functional>
#include <vector>

struct ImageBuffer
{
    int width = 0;
    int height = 0;
    std::vector<float> rgb;

    bool isNull() const { return width <= 0 || height <= 0; }

    void resize(int w, int h)
    {
        width = w;
        height = h;
        rgb.assign(static_cast<size_t>(w) * static_cast<size_t>(h) * 3, 0.0f);
    }

    void clear()
    {
        width = 0;
        height = 0;
        std::vector<float>().swap(rgb);
    }

    float *row(int y) { return rgb.data() + static_cast<size_t>(y) * width * 3; }
    const float *row(int y) const { return rgb.data() + static_cast<size_t>(y) * width * 3; }
};

namespace Imaging {

void parallelFor(int count, const std::function<void(int, int)> &work);

float srgbToLinear(int value8);
float linearToSrgb(float linear);

ImageBuffer rotated(const ImageBuffer &source, int quarterTurnsClockwise);
void scale(ImageBuffer &image, float factor);
void gaussianBlur(ImageBuffer &image, double sigma);

}

#endif
