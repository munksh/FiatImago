#include "pipeline.h"

#include <algorithm>
#include <cmath>

namespace {

const double Pi = 3.14159265358979323846;

inline double smoothstep(double e0, double e1, double x)
{
    const double t = std::max(0.0, std::min(1.0, (x - e0) / (e1 - e0)));
    return t * t * (3.0 - 2.0 * t);
}

struct Geometry
{
    double ws, hs, wo, ho;
    int rotation;
    bool flip;
    double cosT, sinT, zoom;
    double cx, cy, cw, ch;

    Geometry(int sourceWidth, int sourceHeight, const Recipe &r, bool applyCrop)
        : ws(sourceWidth), hs(sourceHeight)
        , rotation(r.rotation), flip(r.flip)
    {
        wo = (rotation % 2) ? hs : ws;
        ho = (rotation % 2) ? ws : hs;
        const double theta = r.straighten * Pi / 180.0;
        cosT = std::cos(theta);
        sinT = std::sin(theta);
        zoom = std::abs(cosT) + std::max(wo / ho, ho / wo) * std::abs(sinT);
        if (applyCrop) {
            cx = r.cropX;
            cy = r.cropY;
            cw = r.cropW;
            ch = r.cropH;
        } else {
            cx = 0.0;
            cy = 0.0;
            cw = 1.0;
            ch = 1.0;
        }
    }

    inline void map(double u, double v, double &sx, double &sy) const
    {
        const double dx = (cx + u * cw) * wo - wo / 2.0;
        const double dy = (cy + v * ch) * ho - ho / 2.0;
        double qx = (cosT * dx + sinT * dy) / zoom + wo / 2.0;
        const double qy = (-sinT * dx + cosT * dy) / zoom + ho / 2.0;
        if (flip)
            qx = wo - qx;
        switch (rotation) {
        case 1: sx = qy; sy = hs - qx; break;
        case 2: sx = ws - qx; sy = hs - qy; break;
        case 3: sx = ws - qy; sy = qx; break;
        default: sx = qx; sy = qy; break;
        }
    }
};

inline void sample(const ImageBuffer &b, double x, double y, float out[3])
{
    x -= 0.5;
    y -= 0.5;
    x = std::max(0.0, std::min(double(b.width - 1), x));
    y = std::max(0.0, std::min(double(b.height - 1), y));
    const int x0 = int(x);
    const int y0 = int(y);
    const int x1 = std::min(x0 + 1, b.width - 1);
    const int y1 = std::min(y0 + 1, b.height - 1);
    const float fx = float(x - x0);
    const float fy = float(y - y0);
    const float *a = b.row(y0) + x0 * 3;
    const float *c = b.row(y0) + x1 * 3;
    const float *d = b.row(y1) + x0 * 3;
    const float *e = b.row(y1) + x1 * 3;
    for (int k = 0; k < 3; ++k) {
        const float top = a[k] + (c[k] - a[k]) * fx;
        const float bottom = d[k] + (e[k] - d[k]) * fx;
        out[k] = top + (bottom - top) * fy;
    }
}

class ToneCurve
{
public:
    ToneCurve(const Recipe &r, bool raw)
        : m_active(raw || r.contrast != 0.0 || r.highlights != 0.0 || r.shadows != 0.0
                   || r.whites != 0.0 || r.blacks != 0.0)
    {
        if (!m_active)
            return;
        m_table.resize(Size + 1);
        for (int i = 0; i <= Size; ++i) {
            const double q = double(i) / Size;
            const double y = q * q * MaxY;
            double p = std::pow(y, 1.0 / 2.2);
            p = perceptual(p, r, raw);
            m_table[i] = float(std::pow(std::max(0.0, p), 2.2));
        }
    }

    bool active() const { return m_active; }

    inline float operator()(float y) const
    {
        if (y <= 0.0f)
            return m_table[0];
        const float q = std::sqrt(std::min(y, float(MaxY)) / float(MaxY)) * Size;
        const int i = std::min(int(q), Size - 1);
        const float t = q - i;
        return m_table[i] + (m_table[i + 1] - m_table[i]) * t;
    }

private:
    static double perceptual(double p, const Recipe &r, bool raw)
    {
        const double c = r.contrast / 100.0;
        const double h = r.highlights / 100.0;
        const double s = r.shadows / 100.0;
        const double w = r.whites / 100.0;
        const double b = r.blacks / 100.0;
        p += b * 0.08 * (1.0 - smoothstep(0.0, 0.35, p));
        p += w * 0.12 * smoothstep(0.55, 1.0, p);
        p += s * 0.22 * (1.0 - smoothstep(0.0, 0.55, p)) * smoothstep(0.0, 0.08, p);
        p += h * 0.22 * smoothstep(0.45, 1.0, p);
        const double factor = (1.0 + 0.6 * c) * (raw ? 1.15 : 1.0);
        p = 0.46 + (p - 0.46) * factor;
        if (raw && p > 0.8)
            p = 0.8 + 0.2 * std::tanh((p - 0.8) / 0.2);
        return p < 0.0 ? 0.0 : p;
    }

    static const int Size = 4096;
    static constexpr double MaxY = 16.0;
    bool m_active;
    std::vector<float> m_table;
};

constexpr double ToneCurve::MaxY;

}

QSizeF Pipeline::frameSize(int sourceWidth, int sourceHeight, const Recipe &recipe, bool applyCrop)
{
    const Geometry g(sourceWidth, sourceHeight, recipe, applyCrop);
    return QSizeF(g.cw * g.wo, g.ch * g.ho);
}

QSize Pipeline::fitted(const QSizeF &frame, int maxLongEdge)
{
    if (frame.isEmpty())
        return QSize();
    const double longest = std::max(frame.width(), frame.height());
    const double scale = (maxLongEdge > 0 && longest > maxLongEdge) ? maxLongEdge / longest : 1.0;
    return QSize(std::max(1, int(std::lround(frame.width() * scale))),
                 std::max(1, int(std::lround(frame.height() * scale))));
}

QImage Pipeline::render(const ImageBuffer &source, const Recipe &r, bool raw, bool applyCrop,
                        const QSize &size, const QRectF &window)
{
    if (source.isNull() || size.isEmpty() || window.isEmpty())
        return QImage();

    const Geometry geometry(source.width, source.height, r, applyCrop);
    const double frameWidth = size.width() / window.width();
    const double blurSigma = r.blur > 0.0 ? r.blur / 100.0 * 0.012 * frameWidth : 0.0;
    const double sharpenSigma = std::max(0.4, r.sharpenRadius / 1000.0 * frameWidth);
    const float sharpenAmount = float(r.sharpenAmount / 100.0 * 1.5);

    const bool whole = window.x() <= 0.0 && window.y() <= 0.0
            && window.right() >= 1.0 && window.bottom() >= 1.0;
    int pad = 0;
    if (!whole) {
        const double reach = std::max(blurSigma, sharpenAmount > 0.0f ? sharpenSigma : 0.0);
        pad = int(std::ceil(3.0 * reach)) + 1;
    }
    const int w = size.width() + 2 * pad;
    const int h = size.height() + 2 * pad;
    const double du = window.width() / size.width();
    const double dv = window.height() / size.height();
    const double u0 = window.x() - pad * du;
    const double v0 = window.y() - pad * dv;

    const double exposure = std::pow(2.0, r.exposure);
    const double t = r.temperature / 100.0;
    const double m = r.tint / 100.0;
    const float gain[3] = { float(exposure * std::pow(2.0, 0.5 * t)),
                            float(exposure * std::pow(2.0, -0.35 * m)),
                            float(exposure * std::pow(2.0, -0.5 * t)) };
    const ToneCurve tone(r, raw);
    const float saturation = float(1.0 + r.saturation / 100.0);
    const float vibrance = float(r.vibrance / 100.0);
    const bool colour = saturation != 1.0f || vibrance != 0.0f;
    const float vignette = float(r.vignetteAmount / 100.0);
    const double vignetteStart = 0.25 + 0.6 * r.vignetteMidpoint / 100.0;
    const double vignetteWidth = 0.05 + 0.9 * r.vignetteFeather / 100.0;

    ImageBuffer image;
    image.resize(w, h);
    Imaging::parallelFor(h, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            float *out = image.row(y);
            const double v = v0 + (y + 0.5) * dv;
            for (int x = 0; x < w; ++x) {
                const double u = u0 + (x + 0.5) * du;
                double sx, sy;
                geometry.map(u, v, sx, sy);
                float px[3];
                sample(source, sx, sy, px);

                px[0] *= gain[0];
                px[1] *= gain[1];
                px[2] *= gain[2];

                if (tone.active()) {
                    const float lum = 0.2126f * px[0] + 0.7152f * px[1] + 0.0722f * px[2];
                    const float target = tone(lum);
                    const float s = lum > 1e-7f ? std::min(target / lum, 16.0f) : 0.0f;
                    const float rest = target - lum * s;
                    for (int k = 0; k < 3; ++k)
                        px[k] = px[k] * s + (rest > 0.0f ? rest : 0.0f);
                }

                if (colour) {
                    const float lum = 0.2126f * px[0] + 0.7152f * px[1] + 0.0722f * px[2];
                    const float hi = std::max(px[0], std::max(px[1], px[2]));
                    const float lo = std::min(px[0], std::min(px[1], px[2]));
                    const float chroma = hi > 1e-6f ? (hi - lo) / hi : 0.0f;
                    const float factor = std::max(0.0f, saturation * (1.0f + vibrance * (1.0f - chroma)));
                    for (int k = 0; k < 3; ++k)
                        px[k] = lum + (px[k] - lum) * factor;
                }

                if (vignette != 0.0f) {
                    const double ex = (u - 0.5) * 2.0;
                    const double ey = (v - 0.5) * 2.0;
                    const double radius = std::sqrt(ex * ex + ey * ey) / std::sqrt(2.0);
                    const double weight = smoothstep(vignetteStart, vignetteStart + vignetteWidth, radius);
                    const float f = float(std::pow(2.0, 2.0 * vignette * weight));
                    for (int k = 0; k < 3; ++k)
                        px[k] *= f;
                }

                out[x * 3] = px[0] > 0.0f ? px[0] : 0.0f;
                out[x * 3 + 1] = px[1] > 0.0f ? px[1] : 0.0f;
                out[x * 3 + 2] = px[2] > 0.0f ? px[2] : 0.0f;
            }
        }
    });

    if (blurSigma > 0.3)
        Imaging::gaussianBlur(image, blurSigma);

    Imaging::parallelFor(h, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            float *p = image.row(y);
            for (int i = 0; i < w * 3; ++i)
                p[i] = Imaging::linearToSrgb(p[i]);
        }
    });

    if (sharpenAmount > 0.0f) {
        ImageBuffer soft = image;
        Imaging::gaussianBlur(soft, sharpenSigma);
        Imaging::parallelFor(h, [&](int begin, int end) {
            for (int y = begin; y < end; ++y) {
                float *p = image.row(y);
                const float *s = soft.row(y);
                for (int i = 0; i < w * 3; ++i)
                    p[i] += sharpenAmount * (p[i] - s[i]);
            }
        });
    }

    QImage result(size, QImage::Format_RGB32);
    Imaging::parallelFor(size.height(), [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            const float *p = image.row(y + pad) + pad * 3;
            QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
            for (int x = 0; x < size.width(); ++x) {
                int c[3];
                for (int k = 0; k < 3; ++k) {
                    const int v = int(p[x * 3 + k] * 255.0f + 0.5f);
                    c[k] = v < 0 ? 0 : (v > 255 ? 255 : v);
                }
                line[x] = qRgb(c[0], c[1], c[2]);
            }
        }
    });
    return result;
}

QVariantList Pipeline::histogram(const QImage &image)
{
    const int bins = 64;
    std::vector<int> counts(3 * bins, 0);
    if (image.isNull())
        return QVariantList();
    const int step = image.width() * image.height() > 1000000 ? 2 : 1;
    for (int y = 0; y < image.height(); y += step) {
        const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); x += step) {
            ++counts[qRed(line[x]) >> 2];
            ++counts[bins + (qGreen(line[x]) >> 2)];
            ++counts[2 * bins + (qBlue(line[x]) >> 2)];
        }
    }
    int peak = 1;
    for (int c = 0; c < 3; ++c) {
        for (int i = 1; i < bins - 1; ++i)
            peak = std::max(peak, counts[c * bins + i]);
    }
    QVariantList channels;
    for (int c = 0; c < 3; ++c) {
        QVariantList values;
        for (int i = 0; i < bins; ++i)
            values << std::min(1.0, double(counts[c * bins + i]) / peak);
        channels << QVariant(values);
    }
    return channels;
}
