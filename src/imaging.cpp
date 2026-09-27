#include "imaging.h"

#include <algorithm>
#include <cmath>
#include <thread>

namespace {

const int EncodeSize = 16384;

const std::vector<float> &toLinearTable()
{
    static const std::vector<float> table = [] {
        std::vector<float> t(256);
        for (int i = 0; i < 256; ++i) {
            const double c = i / 255.0;
            t[i] = float(c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4));
        }
        return t;
    }();
    return table;
}

const std::vector<float> &toSrgbTable()
{
    static const std::vector<float> table = [] {
        std::vector<float> t(EncodeSize + 1);
        for (int i = 0; i <= EncodeSize; ++i) {
            const double l = double(i) / EncodeSize;
            t[i] = float(l <= 0.0031308 ? l * 12.92 : 1.055 * std::pow(l, 1.0 / 2.4) - 0.055);
        }
        return t;
    }();
    return table;
}

inline int clampIndex(int i, int size)
{
    return i < 0 ? 0 : (i >= size ? size - 1 : i);
}

std::vector<int> boxRadii(double sigma)
{
    const int n = 3;
    const double ideal = std::sqrt(12.0 * sigma * sigma / n + 1.0);
    int lower = int(std::floor(ideal));
    if (lower % 2 == 0)
        --lower;
    const int upper = lower + 2;
    const double m = (12.0 * sigma * sigma - n * lower * lower - 4.0 * n * lower - 3.0 * n)
            / (-4.0 * lower - 4.0);
    const int smaller = int(std::round(m));
    std::vector<int> radii;
    for (int i = 0; i < n; ++i)
        radii.push_back(((i < smaller ? lower : upper) - 1) / 2);
    return radii;
}

void boxHorizontal(const float *src, float *dst, int w, int h, int r)
{
    const double norm = 1.0 / (2 * r + 1);
    Imaging::parallelFor(h, [=](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            const float *in = src + static_cast<size_t>(y) * w * 3;
            float *out = dst + static_cast<size_t>(y) * w * 3;
            for (int c = 0; c < 3; ++c) {
                double acc = 0.0;
                for (int k = -r; k <= r; ++k)
                    acc += in[clampIndex(k, w) * 3 + c];
                for (int x = 0; x < w; ++x) {
                    out[x * 3 + c] = float(acc * norm);
                    acc += in[clampIndex(x + r + 1, w) * 3 + c] - in[clampIndex(x - r, w) * 3 + c];
                }
            }
        }
    });
}

void boxVertical(const float *src, float *dst, int w, int h, int r)
{
    const double norm = 1.0 / (2 * r + 1);
    const size_t stride = static_cast<size_t>(w) * 3;
    Imaging::parallelFor(w, [=](int begin, int end) {
        for (int x = begin; x < end; ++x) {
            for (int c = 0; c < 3; ++c) {
                const float *in = src + x * 3 + c;
                float *out = dst + x * 3 + c;
                double acc = 0.0;
                for (int k = -r; k <= r; ++k)
                    acc += in[clampIndex(k, h) * stride];
                for (int y = 0; y < h; ++y) {
                    out[y * stride] = float(acc * norm);
                    acc += in[clampIndex(y + r + 1, h) * stride] - in[clampIndex(y - r, h) * stride];
                }
            }
        }
    });
}

}

void Imaging::parallelFor(int count, const std::function<void(int, int)> &work)
{
    unsigned threads = std::thread::hardware_concurrency();
    if (threads == 0)
        threads = 4;
    threads = std::min(threads, 8u);
    if (count < 64 || threads == 1) {
        work(0, count);
        return;
    }
    const int chunk = (count + int(threads) - 1) / int(threads);
    std::vector<std::thread> pool;
    for (unsigned i = 0; i < threads; ++i) {
        const int begin = int(i) * chunk;
        const int end = std::min(count, begin + chunk);
        if (begin >= end)
            break;
        pool.emplace_back(work, begin, end);
    }
    for (std::thread &t : pool)
        t.join();
}

float Imaging::srgbToLinear(int value8)
{
    return toLinearTable()[value8 < 0 ? 0 : (value8 > 255 ? 255 : value8)];
}

float Imaging::linearToSrgb(float linear)
{
    if (!(linear > 0.0f))
        return 0.0f;
    if (linear >= 1.0f)
        return 1.0f;
    const std::vector<float> &table = toSrgbTable();
    const float f = linear * EncodeSize;
    const int i = int(f);
    const float t = f - i;
    return table[i] + (table[i + 1] - table[i]) * t;
}

ImageBuffer Imaging::rotated(const ImageBuffer &source, int quarterTurnsClockwise)
{
    const int turns = ((quarterTurnsClockwise % 4) + 4) % 4;
    if (turns == 0 || source.isNull())
        return source;

    const int w = source.width;
    const int h = source.height;
    ImageBuffer result;
    if (turns % 2)
        result.resize(h, w);
    else
        result.resize(w, h);

    parallelFor(result.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            float *out = result.row(y);
            for (int x = 0; x < result.width; ++x) {
                int sx, sy;
                if (turns == 1) {
                    sx = y;
                    sy = h - 1 - x;
                } else if (turns == 2) {
                    sx = w - 1 - x;
                    sy = h - 1 - y;
                } else {
                    sx = w - 1 - y;
                    sy = x;
                }
                const float *in = source.row(sy) + sx * 3;
                out[x * 3] = in[0];
                out[x * 3 + 1] = in[1];
                out[x * 3 + 2] = in[2];
            }
        }
    });
    return result;
}

void Imaging::scale(ImageBuffer &image, float factor)
{
    if (factor == 1.0f || image.isNull())
        return;
    const int w = image.width;
    parallelFor(image.height, [&image, factor, w](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            float *p = image.row(y);
            for (int i = 0; i < w * 3; ++i)
                p[i] *= factor;
        }
    });
}

void Imaging::gaussianBlur(ImageBuffer &image, double sigma)
{
    if (sigma < 0.3 || image.isNull())
        return;
    std::vector<float> temp(image.rgb.size());
    for (int r : boxRadii(sigma)) {
        if (r < 1)
            continue;
        boxHorizontal(image.rgb.data(), temp.data(), image.width, image.height, r);
        boxVertical(temp.data(), image.rgb.data(), image.width, image.height, r);
    }
}
