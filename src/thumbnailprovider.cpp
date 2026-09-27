#include "thumbnailprovider.h"

#include "imagesource.h"
#include "pipeline.h"

#include <QImageReader>
#include <QUrl>

#include <algorithm>

ThumbnailProvider::ThumbnailProvider()
    : QQuickImageProvider(QQuickImageProvider::Image, QQmlImageProviderBase::ForceAsynchronousImageLoading)
{
}

QImage ThumbnailProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString decoded = QUrl::fromPercentEncoding(id.toUtf8());
    const int shortEdge = requestedSize.width() > 0 ? requestedSize.width() : 360;
    QImage image;

    if (decoded.startsWith(QLatin1String("raw\n"))) {
        const QStringList parts = decoded.split(QLatin1Char('\n'));
        ImageBuffer buffer;
        if (parts.size() >= 4
                && ImageSource::loadRaw(parts.at(1), parts.at(2), parts.at(3), true, buffer, nullptr, nullptr)) {
            const Recipe neutral;
            const QSizeF frame = Pipeline::frameSize(buffer.width, buffer.height, neutral, false);
            const double scale = double(shortEdge) / std::min(frame.width(), frame.height());
            image = Pipeline::render(buffer, neutral, true, false,
                                     QSize(int(frame.width() * scale), int(frame.height() * scale)));
        }
    } else {
        QImageReader reader(decoded);
        reader.setAutoTransform(true);
        const QSize full = reader.size();
        if (full.isValid() && std::min(full.width(), full.height()) > shortEdge) {
            const double scale = double(shortEdge) / std::min(full.width(), full.height());
            reader.setScaledSize(QSize(int(full.width() * scale), int(full.height() * scale)));
        }
        image = reader.read();
    }

    if (size)
        *size = image.size();
    return image;
}
