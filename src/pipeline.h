#ifndef PIPELINE_H
#define PIPELINE_H

#include <QImage>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QVariantList>

#include "imaging.h"
#include "recipe.h"

// Preview, inspection and export all run through render(): the same code at
// different sizes. Radii are fractions of the frame width, never pixels, so a
// small preview and a full-size export look alike.
namespace Pipeline {

QSizeF frameSize(int sourceWidth, int sourceHeight, const Recipe &recipe, bool applyCrop);
QSize fitted(const QSizeF &frame, int maxLongEdge);

// window is the part of the frame to render, in frame fractions.
// clipWarning paints pixels that reach white in any channel red; for the
// preview only, never for an export.
QImage render(const ImageBuffer &source, const Recipe &recipe, bool raw, bool applyCrop,
              const QSize &size, const QRectF &window = QRectF(0.0, 0.0, 1.0, 1.0),
              bool clipWarning = false);

QVariantList histogram(const QImage &image);

}

#endif
