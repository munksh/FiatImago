#include "recipe.h"

#include <QByteArray>

#include <algorithm>

namespace {

double bounded(const QVariantMap &map, const char *key, double fallback, double low, double high)
{
    const QVariant v = map.value(QLatin1String(key));
    bool ok = false;
    const double d = v.toDouble(&ok);
    if (!v.isValid() || !ok)
        return fallback;
    return std::max(low, std::min(high, d));
}

}

const int Recipe::Bands;

const char *Recipe::bandName(int band)
{
    static const char *names[Bands] = { "Red", "Orange", "Yellow", "Green", "Aqua", "Blue", "Purple", "Magenta" };
    return names[band];
}

const double *Recipe::bandCentres()
{
    static const double centres[Bands] = { 0.0, 30.0, 60.0, 120.0, 180.0, 220.0, 275.0, 320.0 };
    return centres;
}

QVariantMap Recipe::toMap() const
{
    QVariantMap m;
    m.insert(QStringLiteral("exposure"), exposure);
    m.insert(QStringLiteral("contrast"), contrast);
    m.insert(QStringLiteral("highlights"), highlights);
    m.insert(QStringLiteral("shadows"), shadows);
    m.insert(QStringLiteral("whites"), whites);
    m.insert(QStringLiteral("blacks"), blacks);
    m.insert(QStringLiteral("temperature"), temperature);
    m.insert(QStringLiteral("tint"), tint);
    m.insert(QStringLiteral("saturation"), saturation);
    m.insert(QStringLiteral("vibrance"), vibrance);
    for (int b = 0; b < Bands; ++b) {
        const QString name = QLatin1String(bandName(b));
        m.insert(QStringLiteral("hue") + name, mixHue[b]);
        m.insert(QStringLiteral("sat") + name, mixSat[b]);
        m.insert(QStringLiteral("lum") + name, mixLum[b]);
    }
    m.insert(QStringLiteral("rotation"), rotation);
    m.insert(QStringLiteral("flip"), flip);
    m.insert(QStringLiteral("straighten"), straighten);
    m.insert(QStringLiteral("perspectiveV"), perspectiveV);
    m.insert(QStringLiteral("perspectiveH"), perspectiveH);
    m.insert(QStringLiteral("cropX"), cropX);
    m.insert(QStringLiteral("cropY"), cropY);
    m.insert(QStringLiteral("cropW"), cropW);
    m.insert(QStringLiteral("cropH"), cropH);
    m.insert(QStringLiteral("aspect"), aspect);
    m.insert(QStringLiteral("sharpenAmount"), sharpenAmount);
    m.insert(QStringLiteral("sharpenRadius"), sharpenRadius);
    m.insert(QStringLiteral("vignetteAmount"), vignetteAmount);
    m.insert(QStringLiteral("vignetteMidpoint"), vignetteMidpoint);
    m.insert(QStringLiteral("vignetteFeather"), vignetteFeather);
    m.insert(QStringLiteral("blur"), blur);
    m.insert(QStringLiteral("toneShadowHue"), toneShadowHue);
    m.insert(QStringLiteral("toneShadowAmount"), toneShadowAmount);
    m.insert(QStringLiteral("toneHighlightHue"), toneHighlightHue);
    m.insert(QStringLiteral("toneHighlightAmount"), toneHighlightAmount);
    m.insert(QStringLiteral("toneBalance"), toneBalance);
    m.insert(QStringLiteral("patternAmount"), patternAmount);
    m.insert(QStringLiteral("patternSize"), patternSize);
    m.insert(QStringLiteral("look"), look);
    m.insert(QStringLiteral("lookStrength"), lookStrength);
    return m;
}

Recipe Recipe::fromMap(const QVariantMap &m)
{
    Recipe r;
    r.exposure = bounded(m, "exposure", r.exposure, -4.0, 4.0);
    r.contrast = bounded(m, "contrast", r.contrast, -100.0, 100.0);
    r.highlights = bounded(m, "highlights", r.highlights, -100.0, 100.0);
    r.shadows = bounded(m, "shadows", r.shadows, -100.0, 100.0);
    r.whites = bounded(m, "whites", r.whites, -100.0, 100.0);
    r.blacks = bounded(m, "blacks", r.blacks, -100.0, 100.0);
    r.temperature = bounded(m, "temperature", r.temperature, -100.0, 100.0);
    r.tint = bounded(m, "tint", r.tint, -100.0, 100.0);
    r.saturation = bounded(m, "saturation", r.saturation, -100.0, 100.0);
    r.vibrance = bounded(m, "vibrance", r.vibrance, -100.0, 100.0);
    for (int b = 0; b < Bands; ++b) {
        const QByteArray name(bandName(b));
        r.mixHue[b] = bounded(m, QByteArray("hue" + name).constData(), 0.0, -90.0, 90.0);
        r.mixSat[b] = bounded(m, QByteArray("sat" + name).constData(), 0.0, -100.0, 100.0);
        r.mixLum[b] = bounded(m, QByteArray("lum" + name).constData(), 0.0, -100.0, 100.0);
    }
    r.rotation = ((int(bounded(m, "rotation", 0.0, -1000.0, 1000.0)) % 4) + 4) % 4;
    r.flip = m.value(QStringLiteral("flip"), false).toBool();
    r.straighten = bounded(m, "straighten", r.straighten, -45.0, 45.0);
    r.perspectiveV = bounded(m, "perspectiveV", r.perspectiveV, -100.0, 100.0);
    r.perspectiveH = bounded(m, "perspectiveH", r.perspectiveH, -100.0, 100.0);
    r.cropX = bounded(m, "cropX", r.cropX, 0.0, 0.95);
    r.cropY = bounded(m, "cropY", r.cropY, 0.0, 0.95);
    r.cropW = bounded(m, "cropW", r.cropW, 0.05, 1.0 - r.cropX);
    r.cropH = bounded(m, "cropH", r.cropH, 0.05, 1.0 - r.cropY);
    const QString aspect = m.value(QStringLiteral("aspect")).toString();
    if (!aspect.isEmpty())
        r.aspect = aspect;
    r.sharpenAmount = bounded(m, "sharpenAmount", r.sharpenAmount, 0.0, 100.0);
    r.sharpenRadius = bounded(m, "sharpenRadius", r.sharpenRadius, 0.5, 3.0);
    r.vignetteAmount = bounded(m, "vignetteAmount", r.vignetteAmount, -100.0, 100.0);
    r.vignetteMidpoint = bounded(m, "vignetteMidpoint", r.vignetteMidpoint, 0.0, 100.0);
    r.vignetteFeather = bounded(m, "vignetteFeather", r.vignetteFeather, 0.0, 100.0);
    r.blur = bounded(m, "blur", r.blur, 0.0, 100.0);
    r.toneShadowHue = bounded(m, "toneShadowHue", r.toneShadowHue, 0.0, 360.0);
    r.toneShadowAmount = bounded(m, "toneShadowAmount", r.toneShadowAmount, 0.0, 100.0);
    r.toneHighlightHue = bounded(m, "toneHighlightHue", r.toneHighlightHue, 0.0, 360.0);
    r.toneHighlightAmount = bounded(m, "toneHighlightAmount", r.toneHighlightAmount, 0.0, 100.0);
    r.toneBalance = bounded(m, "toneBalance", r.toneBalance, -100.0, 100.0);
    r.patternAmount = bounded(m, "patternAmount", r.patternAmount, 0.0, 100.0);
    r.patternSize = bounded(m, "patternSize", r.patternSize, 30.0, 150.0);
    r.look = m.value(QStringLiteral("look")).toString();
    r.lookStrength = bounded(m, "lookStrength", r.lookStrength, 0.0, 150.0);
    return r;
}

bool Recipe::isDefault() const
{
    return toMap() == Recipe().toMap();
}

QStringList Recipe::geometryKeys()
{
    return QStringList() << QStringLiteral("rotation") << QStringLiteral("flip")
                         << QStringLiteral("straighten") << QStringLiteral("perspectiveV")
                         << QStringLiteral("perspectiveH") << QStringLiteral("cropX")
                         << QStringLiteral("cropY") << QStringLiteral("cropW")
                         << QStringLiteral("cropH") << QStringLiteral("aspect");
}

QStringList Recipe::lookKeys()
{
    QStringList keys = Recipe().toMap().keys();
    for (const QString &k : geometryKeys())
        keys.removeAll(k);
    keys.removeAll(QStringLiteral("look"));
    keys.removeAll(QStringLiteral("lookStrength"));
    return keys;
}

Recipe Recipe::withLook(const QString &id, const QVariantMap &values, double strength) const
{
    const QVariantMap defaults = Recipe().toMap();
    const double f = std::max(0.0, std::min(150.0, strength)) / 100.0;
    QVariantMap m = toMap();
    for (const QString &k : lookKeys()) {
        const double d = defaults.value(k).toDouble();
        const double target = values.contains(k) ? values.value(k).toDouble() : d;
        m.insert(k, d + (target - d) * f);
    }
    m.insert(QStringLiteral("look"), id == QLatin1String("builtin:none") ? QString() : id);
    m.insert(QStringLiteral("lookStrength"), strength);
    return fromMap(m);
}
