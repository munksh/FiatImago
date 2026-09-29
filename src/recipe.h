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

    QVariantMap toMap() const;
    static Recipe fromMap(const QVariantMap &map);
    bool isDefault() const;
    static QStringList geometryKeys();
};

#endif
