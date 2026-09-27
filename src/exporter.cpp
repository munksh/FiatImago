#include "exporter.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageWriter>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

int read16(const QByteArray &d, int at, bool little)
{
    const uchar a = uchar(d.at(at));
    const uchar b = uchar(d.at(at + 1));
    return little ? (a | (b << 8)) : ((a << 8) | b);
}

quint32 read32(const QByteArray &d, int at, bool little)
{
    const quint32 a = uchar(d.at(at)), b = uchar(d.at(at + 1));
    const quint32 c = uchar(d.at(at + 2)), e = uchar(d.at(at + 3));
    return little ? (a | (b << 8) | (c << 16) | (e << 24)) : ((a << 24) | (b << 16) | (c << 8) | e);
}

void write16(QByteArray &d, int at, int value, bool little)
{
    d[at] = char(little ? (value & 0xff) : ((value >> 8) & 0xff));
    d[at + 1] = char(little ? ((value >> 8) & 0xff) : (value & 0xff));
}

QByteArray uprightOrientation(QByteArray segment)
{
    const int tiff = 10;
    if (segment.size() < tiff + 8)
        return segment;
    const bool little = segment.mid(tiff, 2) == "II";
    if (!little && segment.mid(tiff, 2) != "MM")
        return segment;
    const quint32 ifd = read32(segment, tiff + 4, little);
    const int dir = tiff + int(ifd);
    if (dir + 2 > segment.size())
        return segment;
    const int entries = read16(segment, dir, little);
    for (int i = 0; i < entries; ++i) {
        const int e = dir + 2 + 12 * i;
        if (e + 12 > segment.size())
            break;
        if (read16(segment, e, little) == 0x0112) {
            write16(segment, e + 8, 1, little);
            break;
        }
    }
    return segment;
}

}

QByteArray Exporter::cameraData(const QString &jpegPath)
{
    QFile file(jpegPath);
    if (jpegPath.isEmpty() || !file.open(QIODevice::ReadOnly))
        return QByteArray();
    const QByteArray d = file.read(256 * 1024);
    if (d.size() < 4 || uchar(d.at(0)) != 0xFF || uchar(d.at(1)) != 0xD8)
        return QByteArray();

    int pos = 2;
    while (pos + 4 <= d.size()) {
        if (uchar(d.at(pos)) != 0xFF)
            break;
        const int marker = uchar(d.at(pos + 1));
        if (marker == 0xD8 || (marker >= 0xD0 && marker <= 0xD7)) {
            pos += 2;
            continue;
        }
        if (marker == 0xDA || marker == 0xD9)
            break;
        const int length = (uchar(d.at(pos + 2)) << 8) | uchar(d.at(pos + 3));
        if (marker == 0xE1 && pos + 2 + length <= d.size()
                && d.mid(pos + 4, 6) == QByteArray("Exif\0\0", 6))
            return uprightOrientation(d.mid(pos, 2 + length));
        pos += 2 + length;
    }
    return QByteArray();
}

QString Exporter::uniquePath(const QString &baseName)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
            + QStringLiteral("/fiat imago");
    QDir().mkpath(dir);
    QString path = dir + QLatin1Char('/') + baseName + QStringLiteral("_imago.jpg");
    for (int n = 2; QFileInfo::exists(path); ++n)
        path = dir + QLatin1Char('/') + baseName + QStringLiteral("_imago-") + QString::number(n) + QStringLiteral(".jpg");
    return path;
}

bool Exporter::writeJpeg(const QImage &image, const QString &path, int quality,
                         const QByteArray &cameraData, QString *error)
{
    QByteArray encoded;
    {
        QBuffer buffer(&encoded);
        buffer.open(QIODevice::WriteOnly);
        QImageWriter writer(&buffer, "jpg");
        writer.setQuality(quality);
        if (!writer.write(image)) {
            if (error)
                *error = writer.errorString();
            return false;
        }
    }

    QByteArray output;
    if (cameraData.isEmpty() || encoded.size() < 4) {
        output = encoded;
    } else {
        int rest = 2;
        if (uchar(encoded.at(2)) == 0xFF && uchar(encoded.at(3)) == 0xE0 && encoded.size() > 6) {
            const int length = (uchar(encoded.at(4)) << 8) | uchar(encoded.at(5));
            rest = 4 + length;
        }
        output = encoded.left(2) + cameraData + encoded.mid(rest);
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(output) != output.size() || !file.commit()) {
        if (error)
            *error = QStringLiteral("The export could not be written to %1.").arg(path);
        return false;
    }
    return true;
}
