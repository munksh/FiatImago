#include "recipe.h"

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
