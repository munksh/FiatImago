#ifndef IMAGESOURCE_H
#define IMAGESOURCE_H

#include <QString>

#include "imaging.h"
#include "rawmeta.h"

namespace ImageSource {

// Linear sRGB, upright. maxLongEdge 0 reads at full size.
bool loadJpeg(const QString &path, int maxLongEdge, ImageBuffer &out, QString *error);

// Linear sRGB in sensor orientation. Half size bins each 2x2 cell into one
// pixel; full size demosaics (Malvar-He-Cutler).
bool loadRaw(const QString &rawPath, const QString &json, const QString &altJson,
             bool halfSize, ImageBuffer &out, RawMeta *meta, QString *error);

// Quarter turns clockwise that make the RAW match its sibling JPEG, and the
// gain that brings its brightness to the JPEG's.
int matchRotation(const ImageBuffer &raw, const QString &jpegPath);
float matchBrightness(const ImageBuffer &raw, const QString &jpegPath);

}

#endif
