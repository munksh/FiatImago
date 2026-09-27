#ifndef EXPORTER_H
#define EXPORTER_H

#include <QByteArray>
#include <QImage>
#include <QString>

namespace Exporter {

// The camera's own EXIF block, with Orientation set to 1 because the pixels
// are already upright.
QByteArray cameraData(const QString &jpegPath);

QString uniquePath(const QString &baseName);

bool writeJpeg(const QImage &image, const QString &path, int quality,
               const QByteArray &cameraData, QString *error);

}

#endif
