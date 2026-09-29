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

// Output point -> source point: crop, zoom, straighten, perspective, flip and
// the quarter turns, in that order. The zoom is the smallest that keeps the
// frame's corners inside the photo, so no empty corners ever show. A
// perspective transform keeps straight lines straight, which is why checking
// the four corners is enough.
struct Geometry
{
    double ws, hs, wo, ho;
    int rotation;
    bool flip;
    double cosT, sinT;
    double pv, ph;
    double zoom;
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
        pv = r.perspectiveV / 100.0 * MaxPerspective;
        ph = r.perspectiveH / 100.0 * MaxPerspective;
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
        zoom = coverZoom();
    }

    inline void unwarp(double dx, double dy, double &qx, double &qy) const
    {
        const double xn = (cosT * dx + sinT * dy) / wo;
        const double yn = (-sinT * dx + cosT * dy) / ho;
        const double d = 1.0 - pv * yn + ph * xn;
        qx = xn / d * wo + wo / 2.0;
        qy = yn / d * ho + ho / 2.0;
    }

    inline void map(double u, double v, double &sx, double &sy) const
    {
        const double dx = ((cx + u * cw) * wo - wo / 2.0) / zoom;
        const double dy = ((cy + v * ch) * ho - ho / 2.0) / zoom;
        double qx, qy;
        unwarp(dx, dy, qx, qy);
        if (flip)
            qx = wo - qx;
        switch (rotation) {
        case 1: sx = qy; sy = hs - qx; break;
        case 2: sx = ws - qx; sy = hs - qy; break;
        case 3: sx = ws - qy; sy = qx; break;
        default: sx = qx; sy = qy; break;
        }
    }

private:
    bool covers(double z) const
    {
        const double slack = 1e-6 * std::max(wo, ho);
        for (int corner = 0; corner < 4; ++corner) {
            const double dx = ((corner & 1) ? wo : -wo) / 2.0 / z;
            const double dy = ((corner & 2) ? ho : -ho) / 2.0 / z;
            double qx, qy;
            unwarp(dx, dy, qx, qy);
            if (qx < -slack || qx > wo + slack || qy < -slack || qy > ho + slack)
                return false;
        }
        return true;
    }

    double coverZoom() const
    {
        if (covers(1.0))
            return 1.0;
        double low = 1.0;
        double high = 2.0;
        while (!covers(high) && high < 64.0)
            high *= 2.0;
        for (int i = 0; i < 40; ++i) {
            const double mid = (low + high) / 2.0;
            if (covers(mid))
                high = mid;
            else
                low = mid;
        }
        return high;
    }

    static constexpr double MaxPerspective = 0.6;
};

constexpr double Geometry::MaxPerspective;

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

// Power 2.2 there and back through tables, so the mixer's perceptual
// space costs no pow() per pixel.
class Gamma
{
public:
    Gamma()
    {
        m_encode.resize(Size + 1);
        m_decode.resize(Size + 1);
        for (int i = 0; i <= Size; ++i) {
            m_encode[i] = float(std::pow(double(i) / Size * MaxIn, 1.0 / 2.2));
            m_decode[i] = float(std::pow(double(i) / Size * MaxOut, 2.2));
        }
    }
    inline float encode(float x) const { return lookup(m_encode, x / MaxIn); }
    inline float decode(float v) const { return lookup(m_decode, v / MaxOut); }

private:
    static inline float lookup(const std::vector<float> &t, float f)
    {
        if (!(f > 0.0f))
            return t[0];
        if (f >= 1.0f)
            return t[Size];
        const float x = f * Size;
        const int i = int(x);
        return t[i] + (t[i + 1] - t[i]) * (x - i);
    }
    static const int Size = 8192;
    static constexpr float MaxIn = 4.0f;
    static constexpr float MaxOut = 1.9f;
    std::vector<float> m_encode;
    std::vector<float> m_decode;
};

constexpr float Gamma::MaxIn;
constexpr float Gamma::MaxOut;

// Moves hue, saturation and lightness per colour band in HSV on gamma-encoded
// values. A pixel belongs to the two bands its hue lies between, blended
// smoothly, so neighbouring colours never tear. Greys have no hue and are
// left alone.
class Mixer
{
public:
    explicit Mixer(const Recipe &r)
        : m_active(false)
    {
        for (int b = 0; b < Recipe::Bands; ++b) {
            m_hue[b] = float(r.mixHue[b]);
            m_sat[b] = float(r.mixSat[b] / 100.0);
            m_lum[b] = float(r.mixLum[b] / 100.0 * 0.5);
            if (r.mixHue[b] != 0.0 || r.mixSat[b] != 0.0 || r.mixLum[b] != 0.0)
                m_active = true;
            m_centre[b] = float(Recipe::bandCentres()[b]);
        }
    }

    bool active() const { return m_active; }

    inline void apply(float px[3]) const
    {
        const float r = m_gamma.encode(px[0]);
        const float g = m_gamma.encode(px[1]);
        const float b = m_gamma.encode(px[2]);
        const float mx = std::max(r, std::max(g, b));
        const float mn = std::min(r, std::min(g, b));
        const float c = mx - mn;
        if (c < 1e-5f || mx < 1e-5f)
            return;
        float h;
        if (mx == r)
            h = 60.0f * std::fmod((g - b) / c + 6.0f, 6.0f);
        else if (mx == g)
            h = 60.0f * ((b - r) / c + 2.0f);
        else
            h = 60.0f * ((r - g) / c + 4.0f);
        const float s = c / mx;

        int i = Recipe::Bands - 1;
        for (int k = 0; k < Recipe::Bands - 1; ++k) {
            if (h >= m_centre[k] && h < m_centre[k + 1]) {
                i = k;
                break;
            }
        }
        const int j = (i + 1) % Recipe::Bands;
        const float from = m_centre[i];
        const float to = j == 0 ? 360.0f : m_centre[j];
        const float hh = (i == Recipe::Bands - 1 && h < from) ? h + 360.0f : h;
        float t = (hh - from) / (to - from);
        t = t * t * (3.0f - 2.0f * t);
        const float wi = 1.0f - t;

        const float grey = std::min(1.0f, s * 2.0f);
        float h2 = h + (wi * m_hue[i] + t * m_hue[j]) * grey;
        h2 = std::fmod(h2 + 360.0f, 360.0f);
        const float s2 = std::max(0.0f, std::min(1.0f, s * (1.0f + wi * m_sat[i] + t * m_sat[j])));
        const float v2 = mx;

        const float c2 = v2 * s2;
        const float x = c2 * (1.0f - std::fabs(std::fmod(h2 / 60.0f, 2.0f) - 1.0f));
        const float m = v2 - c2;
        float o[3];
        switch (int(h2 / 60.0f) % 6) {
        case 0: o[0] = c2; o[1] = x; o[2] = 0; break;
        case 1: o[0] = x; o[1] = c2; o[2] = 0; break;
        case 2: o[0] = 0; o[1] = c2; o[2] = x; break;
        case 3: o[0] = 0; o[1] = x; o[2] = c2; break;
        case 4: o[0] = x; o[1] = 0; o[2] = c2; break;
        default: o[0] = c2; o[1] = 0; o[2] = x; break;
        }
        // HSV keeps the brightest channel, so a colour losing saturation
        // would turn pale. Keep its lightness instead; only the lightness
        // slider moves it.
        const float before = 0.2126f * r + 0.7152f * g + 0.0722f * b;
        const float after = 0.2126f * (o[0] + m) + 0.7152f * (o[1] + m) + 0.0722f * (o[2] + m);
        const float target = before * (1.0f + (wi * m_lum[i] + t * m_lum[j]) * grey);
        const float k = after > 1e-5f ? std::max(0.0f, target) / after : 1.0f;
        for (int n = 0; n < 3; ++n)
            px[n] = m_gamma.decode((o[n] + m) * k);
    }

private:
    bool m_active;
    float m_hue[Recipe::Bands];
    float m_sat[Recipe::Bands];
    float m_lum[Recipe::Bands];
    float m_centre[Recipe::Bands];
    Gamma m_gamma;
};

// Split toning in linear light: each side's colour is scaled to unit
// luminance, so tinting changes colour but not brightness.
class Toning
{
public:
    explicit Toning(const Recipe &r)
        : m_active(r.toneShadowAmount > 0.0 || r.toneHighlightAmount > 0.0)
        , m_shadowAmount(float(r.toneShadowAmount / 100.0 * 0.8))
        , m_highlightAmount(float(r.toneHighlightAmount / 100.0 * 0.8))
        , m_pivot(float(0.5 - 0.35 * r.toneBalance / 100.0))
    {
        tint(r.toneShadowHue, m_shadow);
        tint(r.toneHighlightHue, m_highlight);
    }

    bool active() const { return m_active; }

    inline void apply(float px[3]) const
    {
        const float lum = 0.2126f * px[0] + 0.7152f * px[1] + 0.0722f * px[2];
        const float p = std::sqrt(std::max(0.0f, std::min(1.0f, lum)));
        const float t = float(smoothstep(m_pivot - 0.35f, m_pivot + 0.35f, p));
        const float ws = (1.0f - t) * m_shadowAmount;
        const float wh = t * m_highlightAmount;
        for (int k = 0; k < 3; ++k)
            px[k] *= 1.0f + ws * (m_shadow[k] - 1.0f) + wh * (m_highlight[k] - 1.0f);
    }

private:
    static void tint(double hue, float out[3])
    {
        const double h = std::fmod(hue, 360.0) / 60.0;
        const double x = 1.0 - std::fabs(std::fmod(h, 2.0) - 1.0);
        double rgb[3];
        switch (int(h) % 6) {
        case 0: rgb[0] = 1; rgb[1] = x; rgb[2] = 0; break;
        case 1: rgb[0] = x; rgb[1] = 1; rgb[2] = 0; break;
        case 2: rgb[0] = 0; rgb[1] = 1; rgb[2] = x; break;
        case 3: rgb[0] = 0; rgb[1] = x; rgb[2] = 1; break;
        case 4: rgb[0] = x; rgb[1] = 0; rgb[2] = 1; break;
        default: rgb[0] = 1; rgb[1] = 0; rgb[2] = x; break;
        }
        for (int k = 0; k < 3; ++k)
            rgb[k] = 0.4 + 0.6 * rgb[k];
        const double lum = 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2];
        for (int k = 0; k < 3; ++k)
            out[k] = float(rgb[k] / lum);
    }

    bool m_active;
    float m_shadowAmount;
    float m_highlightAmount;
    float m_pivot;
    float m_shadow[3];
    float m_highlight[3];
};

// Sailfish's background pattern, measured from a screenshot of its Settings:
// an 8 by 8 tile holding two triangles in staggered rows, brightest at the
// tip. Values are relative to the brightest point.
const float PatternTile[8][8] = {
    {  1.00f, -0.21f, -0.21f, -0.21f, -0.21f, -0.21f, -0.21f, -0.21f },
    {  0.50f,  0.50f, -0.21f, -0.21f, -0.21f, -0.21f, -0.21f,  0.50f },
    {  0.14f,  0.14f,  0.14f, -0.21f, -0.21f, -0.21f,  0.14f,  0.14f },
    { -0.07f, -0.07f, -0.07f, -0.07f, -0.21f, -0.07f, -0.07f, -0.07f },
    { -0.21f, -0.21f, -0.21f, -0.21f,  1.00f, -0.21f, -0.21f, -0.21f },
    { -0.21f, -0.21f, -0.21f,  0.50f,  0.50f,  0.50f, -0.21f, -0.21f },
    { -0.21f, -0.21f,  0.14f,  0.14f,  0.14f,  0.14f,  0.14f, -0.21f },
    { -0.21f, -0.07f, -0.07f, -0.07f, -0.07f, -0.07f, -0.07f, -0.07f }
};

inline float patternAt(double tx, double ty)
{
    tx -= 0.5;
    ty -= 0.5;
    const double fx = std::floor(tx);
    const double fy = std::floor(ty);
    const float ax = float(tx - fx);
    const float ay = float(ty - fy);
    const int x0 = ((int(fx) % 8) + 8) % 8;
    const int y0 = ((int(fy) % 8) + 8) % 8;
    const int x1 = (x0 + 1) % 8;
    const int y1 = (y0 + 1) % 8;
    const float top = PatternTile[y0][x0] + (PatternTile[y0][x1] - PatternTile[y0][x0]) * ax;
    const float bottom = PatternTile[y1][x0] + (PatternTile[y1][x1] - PatternTile[y1][x0]) * ax;
    return top + (bottom - top) * ay;
}

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
                        const QSize &size, const QRectF &window, bool clipWarning)
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
    const float gain[3] = { float(exposure * std::pow(2.0, 1.0 * t)),
                            float(exposure * std::pow(2.0, -0.6 * m)),
                            float(exposure * std::pow(2.0, -1.0 * t)) };
    const ToneCurve tone(r, raw);
    const Mixer mixer(r);
    const Toning toning(r);
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

                if (mixer.active())
                    mixer.apply(px);

                if (colour) {
                    const float lum = 0.2126f * px[0] + 0.7152f * px[1] + 0.0722f * px[2];
                    const float hi = std::max(px[0], std::max(px[1], px[2]));
                    const float lo = std::min(px[0], std::min(px[1], px[2]));
                    const float chroma = hi > 1e-6f ? (hi - lo) / hi : 0.0f;
                    const float factor = std::max(0.0f, saturation * (1.0f + vibrance * (1.0f - chroma)));
                    for (int k = 0; k < 3; ++k)
                        px[k] = lum + (px[k] - lum) * factor;
                }

                if (toning.active())
                    toning.apply(px);

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

    std::vector<unsigned char> clipped;
    if (clipWarning) {
        clipped.assign(static_cast<size_t>(w) * h, 0);
        Imaging::parallelFor(h, [&](int begin, int end) {
            for (int y = begin; y < end; ++y) {
                const float *p = image.row(y);
                unsigned char *m = clipped.data() + static_cast<size_t>(y) * w;
                for (int x = 0; x < w; ++x)
                    m[x] = (p[x * 3] >= 1.0f || p[x * 3 + 1] >= 1.0f || p[x * 3 + 2] >= 1.0f) ? 1 : 0;
            }
        });
    }

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

    if (r.patternAmount > 0.0) {
        const float amp = float(r.patternAmount / 100.0 * 0.16);
        const Geometry frame(source.width, source.height, r, applyCrop);
        const double aspect = (frame.ch * frame.ho) / (frame.cw * frame.wo);
        const double texels = r.patternSize * 8.0;
        Imaging::parallelFor(h, [&](int begin, int end) {
            for (int y = begin; y < end; ++y) {
                float *p = image.row(y);
                const double v = v0 + (y + 0.5) * dv;
                for (int x = 0; x < w; ++x) {
                    const double u = u0 + (x + 0.5) * du;
                    const float add = amp * patternAt(u * texels, v * texels * aspect);
                    p[x * 3] += add;
                    p[x * 3 + 1] += add;
                    p[x * 3 + 2] += add;
                }
            }
        });
    }

    QImage result(size, QImage::Format_RGB32);
    Imaging::parallelFor(size.height(), [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            const float *p = image.row(y + pad) + pad * 3;
            const unsigned char *m = clipWarning
                    ? clipped.data() + static_cast<size_t>(y + pad) * w + pad : nullptr;
            QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
            for (int x = 0; x < size.width(); ++x) {
                if (m && m[x]) {
                    line[x] = qRgb(235, 40, 35);
                    continue;
                }
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
