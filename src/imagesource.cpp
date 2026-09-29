#include "imagesource.h"

#include <QByteArray>
#include <QFile>
#include <QImage>
#include <QImageReader>

#include <algorithm>
#include <cmath>

namespace {

const double XyzD50ToSrgb[9] = {
     3.1338561, -1.6168667, -0.4906146,
    -0.9787684,  1.9161415,  0.0334540,
     0.0719453, -0.2289914,  1.4052427
};

void cfaChannels(int cfa, int channels[4])
{
    static const int layouts[4][4] = {
        { 0, 1, 1, 2 },   // RGGB
        { 1, 0, 2, 1 },   // GRBG
        { 1, 2, 0, 1 },   // GBRG
        { 2, 1, 1, 0 }    // BGGR
    };
    for (int i = 0; i < 4; ++i)
        channels[i] = layouts[cfa][i];
}

struct Finish
{
    float gain[3];
    float matrix[9];

    explicit Finish(const RawMeta &meta)
    {
        double g[3] = { 1.0, 1.0, 1.0 };
        if (meta.hasNeutral) {
            for (int i = 0; i < 3; ++i)
                g[i] = meta.neutral[1] / meta.neutral[i];
        }
        const double lowest = std::min(g[0], std::min(g[1], g[2]));
        for (int i = 0; i < 3; ++i)
            gain[i] = float(g[i] / lowest);

        // The camera's own transform for this shot comes first, since it is
        // what the camera's JPEG was made with; then the DNG path through the
        // forward matrix; without either, the camera's colours as they are.
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                double value = r == c ? 1.0 : 0.0;
                if (meta.hasCapture) {
                    value = meta.capture[r * 3 + c];
                } else if (meta.hasForward) {
                    value = 0.0;
                    for (int k = 0; k < 3; ++k)
                        value += XyzD50ToSrgb[r * 3 + k] * meta.forward[k * 3 + c];
                }
                matrix[r * 3 + c] = float(value);
            }
        }
    }

    inline void apply(const float in[3], float *out) const
    {
        const float r = std::min(in[0] * gain[0], 1.0f);
        const float g = std::min(in[1] * gain[1], 1.0f);
        const float b = std::min(in[2] * gain[2], 1.0f);
        out[0] = matrix[0] * r + matrix[1] * g + matrix[2] * b;
        out[1] = matrix[3] * r + matrix[4] * g + matrix[5] * b;
        out[2] = matrix[6] * r + matrix[7] * g + matrix[8] * b;
    }
};

class Mosaic
{
public:
    Mosaic(const QByteArray &data, const RawMeta &meta)
        : m_base(reinterpret_cast<const uchar *>(data.constData()))
        , m_meta(meta)
    {
        for (int i = 0; i < 4; ++i) {
            const double range = meta.whiteLevel - meta.black[i];
            m_black[i] = float(meta.black[i]);
            m_scale[i] = range > 0.0 ? float(1.0 / range) : 0.0f;
        }
    }

    inline float at(int x, int y) const
    {
        const int raw = sample(x, y);
        const int idx = ((y & 1) << 1) | (x & 1);
        const float v = (raw - m_black[idx]) * m_scale[idx];
        return v < 0.0f ? 0.0f : v;
    }

private:
    // Android's packings: RAW10 keeps the high eight bits of four pixels in
    // four bytes and their low two bits in a fifth; RAW12 keeps two pixels
    // in three bytes, the shared byte holding both low nibbles.
    inline int sample(int x, int y) const
    {
        const uchar *row = m_base + qint64(y) * m_meta.rowStride;
        switch (m_meta.bits) {
        case 8:
            return row[x];
        case 10: {
            const uchar *p = row + (x >> 2) * 5;
            const int i = x & 3;
            return (p[i] << 2) | ((p[4] >> (i * 2)) & 0x3);
        }
        case 12: {
            const uchar *p = row + (x >> 1) * 3;
            return (x & 1) ? ((p[1] << 4) | (p[2] >> 4)) : ((p[0] << 4) | (p[2] & 0x0f));
        }
        default: {
            const uchar *p = row + qint64(x) * std::max(2, m_meta.pixelStride);
            return p[0] | (p[1] << 8);
        }
        }
    }

    const uchar *m_base;
    const RawMeta &m_meta;
    float m_black[4];
    float m_scale[4];
};

inline int reflect(int i, int size)
{
    if (i < 0)
        i = -i;
    if (i >= size)
        i = 2 * size - 2 - i;
    return i < 0 ? 0 : (i >= size ? size - 1 : i);
}

void demosaic(const std::vector<float> &plane, int w, int h, const int channels[4],
              const Finish &finish, ImageBuffer &out)
{
    out.resize(w, h);
    Imaging::parallelFor(h, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            float *dst = out.row(y);
            for (int x = 0; x < w; ++x) {
                auto P = [&](int dx, int dy) {
                    return plane[size_t(reflect(y + dy, h)) * w + reflect(x + dx, w)];
                };
                const int here = channels[((y & 1) << 1) | (x & 1)];
                const float c0 = P(0, 0);
                const float cross = P(-1, 0) + P(1, 0) + P(0, -1) + P(0, 1);
                const float diag = P(-1, -1) + P(1, -1) + P(-1, 1) + P(1, 1);
                const float farH = P(-2, 0) + P(2, 0);
                const float farV = P(0, -2) + P(0, 2);
                float rgb[3];

                if (here == 1) {
                    const bool redRow = channels[((y & 1) << 1) | ((x + 1) & 1)] == 0;
                    const float horizontal = (5.0f * c0 + 4.0f * (P(-1, 0) + P(1, 0)) - farH - diag + 0.5f * farV) / 8.0f;
                    const float vertical = (5.0f * c0 + 4.0f * (P(0, -1) + P(0, 1)) - farV - diag + 0.5f * farH) / 8.0f;
                    rgb[1] = c0;
                    rgb[0] = redRow ? horizontal : vertical;
                    rgb[2] = redRow ? vertical : horizontal;
                } else {
                    const float green = (4.0f * c0 + 2.0f * cross - farH - farV) / 8.0f;
                    const float opposite = (6.0f * c0 + 2.0f * diag - 1.5f * (farH + farV)) / 8.0f;
                    rgb[1] = green;
                    rgb[here] = c0;
                    rgb[2 - here] = opposite;
                }
                for (int c = 0; c < 3; ++c)
                    rgb[c] = rgb[c] < 0.0f ? 0.0f : rgb[c];
                finish.apply(rgb, dst + x * 3);
            }
        }
    });
}

QImage smallJpeg(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (size.isValid())
        reader.setScaledSize(size.scaled(96, 96, Qt::KeepAspectRatio));
    return reader.read().convertToFormat(QImage::Format_RGB32);
}

void mapTurn(int turns, double u, double v, double &su, double &sv)
{
    switch (turns) {
    case 1: su = v; sv = 1.0 - u; break;
    case 2: su = 1.0 - u; sv = 1.0 - v; break;
    case 3: su = 1.0 - v; sv = u; break;
    default: su = u; sv = v; break;
    }
}

float lumaAt(const ImageBuffer &b, double u, double v)
{
    const int x = std::min(b.width - 1, std::max(0, int(u * b.width)));
    const int y = std::min(b.height - 1, std::max(0, int(v * b.height)));
    const float *p = b.row(y) + x * 3;
    return 0.2126f * p[0] + 0.7152f * p[1] + 0.0722f * p[2];
}

float lumaAt(const QImage &img, double u, double v)
{
    const int x = std::min(img.width() - 1, std::max(0, int(u * img.width())));
    const int y = std::min(img.height() - 1, std::max(0, int(v * img.height())));
    const QRgb c = img.pixel(x, y);
    return 0.2126f * Imaging::srgbToLinear(qRed(c)) + 0.7152f * Imaging::srgbToLinear(qGreen(c))
            + 0.0722f * Imaging::srgbToLinear(qBlue(c));
}

}

bool ImageSource::loadJpeg(const QString &path, int maxLongEdge, ImageBuffer &out, QString *error)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (maxLongEdge > 0 && size.isValid() && std::max(size.width(), size.height()) > maxLongEdge)
        reader.setScaledSize(size.scaled(maxLongEdge, maxLongEdge, Qt::KeepAspectRatio));

    QImage image = reader.read();
    if (image.isNull()) {
        if (error)
            *error = reader.errorString();
        return false;
    }
    image = image.convertToFormat(QImage::Format_RGB32);

    out.resize(image.width(), image.height());
    Imaging::parallelFor(image.height(), [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
            float *dst = out.row(y);
            for (int x = 0; x < image.width(); ++x) {
                dst[x * 3] = Imaging::srgbToLinear(qRed(line[x]));
                dst[x * 3 + 1] = Imaging::srgbToLinear(qGreen(line[x]));
                dst[x * 3 + 2] = Imaging::srgbToLinear(qBlue(line[x]));
            }
        }
    });
    return true;
}

bool ImageSource::loadRaw(const QString &rawPath, const QString &json, const QString &altJson,
                          bool halfSize, ImageBuffer &out, RawMeta *metaOut, QString *error)
{
    RawMeta meta;
    if (!readRawMeta(rawPath, json, altJson, meta, error))
        return false;

    QFile file(rawPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("The RAW file could not be opened.");
        return false;
    }
    const QByteArray data = file.readAll();
    const qint64 needed = qint64(meta.height - 1) * meta.rowStride + meta.bytesPerRow();
    if (data.size() < needed) {
        if (error)
            *error = QStringLiteral("The RAW file is shorter than its sidecar says.");
        return false;
    }

    int channels[4];
    cfaChannels(meta.cfa, channels);
    const Mosaic mosaic(data, meta);
    const Finish finish(meta);

    if (halfSize) {
        const int w = meta.width / 2;
        const int h = meta.height / 2;
        out.resize(w, h);
        Imaging::parallelFor(h, [&](int begin, int end) {
            for (int y = begin; y < end; ++y) {
                float *dst = out.row(y);
                for (int x = 0; x < w; ++x) {
                    float sum[3] = { 0.0f, 0.0f, 0.0f };
                    int count[3] = { 0, 0, 0 };
                    for (int dy = 0; dy < 2; ++dy) {
                        for (int dx = 0; dx < 2; ++dx) {
                            const int c = channels[(dy << 1) | dx];
                            sum[c] += mosaic.at(2 * x + dx, 2 * y + dy);
                            ++count[c];
                        }
                    }
                    float rgb[3];
                    for (int c = 0; c < 3; ++c)
                        rgb[c] = count[c] ? sum[c] / count[c] : 0.0f;
                    finish.apply(rgb, dst + x * 3);
                }
            }
        });
    } else {
        const int w = meta.width;
        const int h = meta.height;
        std::vector<float> plane(size_t(w) * size_t(h));
        Imaging::parallelFor(h, [&](int begin, int end) {
            for (int y = begin; y < end; ++y) {
                for (int x = 0; x < w; ++x)
                    plane[size_t(y) * w + x] = mosaic.at(x, y);
            }
        });
        demosaic(plane, w, h, channels, finish, out);
    }

    if (metaOut)
        *metaOut = meta;
    return true;
}

int ImageSource::matchRotation(const ImageBuffer &raw, const QString &jpegPath)
{
    if (raw.isNull() || jpegPath.isEmpty())
        return 0;
    const QImage jpeg = smallJpeg(jpegPath);
    if (jpeg.isNull())
        return 0;

    const double jpegAspect = double(jpeg.width()) / jpeg.height();
    const int grid = 24;
    std::vector<float> reference(grid * grid);
    double refMean = 0.0;
    for (int j = 0; j < grid; ++j) {
        for (int i = 0; i < grid; ++i) {
            const float v = std::sqrt(lumaAt(jpeg, (i + 0.5) / grid, (j + 0.5) / grid));
            reference[j * grid + i] = v;
            refMean += v;
        }
    }
    refMean /= grid * grid;

    int best = 0;
    double bestScore = -2.0;
    for (int turns = 0; turns < 4; ++turns) {
        const double aspect = (turns % 2) ? double(raw.height) / raw.width : double(raw.width) / raw.height;
        if (std::abs(aspect / jpegAspect - 1.0) > 0.08)
            continue;
        std::vector<float> candidate(grid * grid);
        double mean = 0.0;
        for (int j = 0; j < grid; ++j) {
            for (int i = 0; i < grid; ++i) {
                double su, sv;
                mapTurn(turns, (i + 0.5) / grid, (j + 0.5) / grid, su, sv);
                const float v = std::sqrt(std::max(0.0f, lumaAt(raw, su, sv)));
                candidate[j * grid + i] = v;
                mean += v;
            }
        }
        mean /= grid * grid;
        double num = 0.0, da = 0.0, db = 0.0;
        for (int k = 0; k < grid * grid; ++k) {
            const double a = reference[k] - refMean;
            const double b = candidate[k] - mean;
            num += a * b;
            da += a * a;
            db += b * b;
        }
        const double score = (da > 0.0 && db > 0.0) ? num / std::sqrt(da * db) : -1.0;
        if (score > bestScore) {
            bestScore = score;
            best = turns;
        }
    }
    return best;
}

float ImageSource::matchBrightness(const ImageBuffer &raw, const QString &jpegPath)
{
    if (raw.isNull() || jpegPath.isEmpty())
        return 1.0f;
    const QImage jpeg = smallJpeg(jpegPath);
    if (jpeg.isNull())
        return 1.0f;

    const int grid = 32;
    double logJpeg = 0.0;
    double logRaw = 0.0;
    for (int j = 0; j < grid; ++j) {
        for (int i = 0; i < grid; ++i) {
            const double u = (i + 0.5) / grid;
            const double v = (j + 0.5) / grid;
            logJpeg += std::log(lumaAt(jpeg, u, v) + 1e-3);
            logRaw += std::log(std::max(0.0f, lumaAt(raw, u, v)) + 1e-3);
        }
    }
    const double gain = std::exp((logJpeg - logRaw) / (grid * grid));
    return float(std::min(8.0, std::max(0.125, gain)));
}
