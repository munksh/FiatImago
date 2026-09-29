#ifndef RECIPE_H
#define RECIPE_H

#include <QString>
#include <QStringList>
#include <QVariantMap>

struct Recipe
{
    double exposure = 0.0;
    double contrast = 0.0;
    double highlights = 0.0;
    double shadows = 0.0;
    double whites = 0.0;
    double blacks = 0.0;

    double temperature = 0.0;
    double tint = 0.0;
    double saturation = 0.0;
    double vibrance = 0.0;

    // The colour mixer: per hue band, a hue shift in degrees and a
    // saturation and lightness change in percent.
    static const int Bands = 8;
    double mixHue[Bands] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    double mixSat[Bands] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    double mixLum[Bands] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    static const char *bandName(int band);
    static const double *bandCentres();

    int rotation = 0;
    bool flip = false;
    double straighten = 0.0;
    double perspectiveV = 0.0;
    double perspectiveH = 0.0;
    double cropX = 0.0;
    double cropY = 0.0;
    double cropW = 1.0;
    double cropH = 1.0;
    QString aspect = QStringLiteral("original");

    double sharpenAmount = 0.0;
    double sharpenRadius = 1.0;

    double vignetteAmount = 0.0;
    double vignetteMidpoint = 50.0;
    double vignetteFeather = 50.0;
    double blur = 0.0;

    // Toning: a colour for the shadows and one for the highlights, each a hue
    // in degrees and a strength; balance moves the split between them.
    double toneShadowHue = 0.0;
    double toneShadowAmount = 0.0;
    double toneHighlightHue = 0.0;
    double toneHighlightAmount = 0.0;
    double toneBalance = 0.0;

    // The triangle pattern Sailfish draws on its own backgrounds; size is
    // the number of pattern tiles across the frame.
    double patternAmount = 0.0;
    double patternSize = 80.0;

    QString look;
    double lookStrength = 100.0;

    QVariantMap toMap() const;
    static Recipe fromMap(const QVariantMap &map);
    bool isDefault() const;
    static QStringList geometryKeys();
    static QStringList lookKeys();

    // Sets every look key to the preset's value scaled by strength (0 to
    // 150 %) from the default; keys the preset leaves out go to their
    // default. Geometry is kept.
    Recipe withLook(const QString &id, const QVariantMap &values, double strength) const;
};

#endif
