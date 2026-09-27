#include "rawmeta.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>

namespace {

QJsonObject readObject(const QString &path)
{
    if (path.isEmpty())
        return QJsonObject();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QJsonObject();
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.isObject() ? doc.object() : QJsonObject();
}

// Camera2 rationals may arrive as numbers, "n/d" strings, [n, d] pairs or
// {numerator, denominator} objects depending on how they were printed.
bool toNumber(const QJsonValue &value, double &out)
{
    if (value.isDouble()) {
        out = value.toDouble();
        return true;
    }
    if (value.isString()) {
        const QString s = value.toString().trimmed();
        const int slash = s.indexOf(QLatin1Char('/'));
        bool okN = false;
        bool okD = false;
        if (slash > 0) {
            const double n = s.left(slash).toDouble(&okN);
            const double d = s.mid(slash + 1).toDouble(&okD);
            if (okN && okD && d != 0.0) {
                out = n / d;
                return true;
            }
            return false;
        }
        out = s.toDouble(&okN);
        return okN;
    }
    if (value.isArray()) {
        const QJsonArray a = value.toArray();
        if (a.size() == 2 && a.at(0).isDouble() && a.at(1).isDouble() && a.at(1).toDouble() != 0.0) {
            out = a.at(0).toDouble() / a.at(1).toDouble();
            return true;
        }
        return false;
    }
    if (value.isObject()) {
        const QJsonObject o = value.toObject();
        const QJsonValue n = o.contains(QStringLiteral("numerator")) ? o.value(QStringLiteral("numerator")) : o.value(QStringLiteral("n"));
        const QJsonValue d = o.contains(QStringLiteral("denominator")) ? o.value(QStringLiteral("denominator")) : o.value(QStringLiteral("d"));
        if (n.isDouble() && d.isDouble() && d.toDouble() != 0.0) {
            out = n.toDouble() / d.toDouble();
            return true;
        }
    }
    return false;
}

QVector<double> flatten(const QJsonValue &value)
{
    QVector<double> out;
    double d = 0.0;
    if (toNumber(value, d)) {
        out << d;
        return out;
    }
    if (value.isArray()) {
        const QJsonArray a = value.toArray();
        for (const QJsonValue &v : a)
            out += flatten(v);
    }
    return out;
}

QVector<double> numbers(const QJsonValue &value, int expected)
{
    QVector<double> v = flatten(value);
    if (v.size() == expected * 2) {
        QVector<double> paired;
        for (int i = 0; i < expected; ++i) {
            if (v.at(2 * i + 1) == 0.0)
                return QVector<double>();
            paired << v.at(2 * i) / v.at(2 * i + 1);
        }
        return paired;
    }
    return v.size() == expected ? v : QVector<double>();
}

class Sources
{
public:
    Sources(const QJsonObject &a, const QJsonObject &b) : m_a(a), m_b(b) { }

    QJsonValue value(const QString &key) const
    {
        const QJsonValue v = m_a.value(key);
        return (v.isUndefined() || v.isNull()) ? m_b.value(key) : v;
    }

    int integer(const QString &key, int fallback) const
    {
        double d = 0.0;
        return toNumber(value(key), d) ? int(d) : fallback;
    }

    bool isEmpty() const { return m_a.isEmpty() && m_b.isEmpty(); }

private:
    QJsonObject m_a;
    QJsonObject m_b;
};

int cfaFromName(const QString &name)
{
    const QString n = name.toUpper();
    if (n.contains(QLatin1String("RGGB"))) return 0;
    if (n.contains(QLatin1String("GRBG"))) return 1;
    if (n.contains(QLatin1String("GBRG"))) return 2;
    if (n.contains(QLatin1String("BGGR"))) return 3;
    return -1;
}

bool allZero(const QVector<double> &v)
{
    for (double d : v) {
        if (d != 0.0)
            return false;
    }
    return true;
}

}

bool RawMeta::isValid() const
{
    return width > 0 && height > 0 && rowStride >= width * pixelStride
            && pixelStride >= 2 && cfa >= 0 && cfa <= 3 && whiteLevel > 0.0;
}

bool readRawMeta(const QString &primaryJson, const QString &secondaryJson,
                 RawMeta &meta, QString *error)
{
    const Sources s(readObject(primaryJson), readObject(secondaryJson));
    if (s.isEmpty()) {
        if (error)
            *error = QStringLiteral("The RAW sidecar could not be read.");
        return false;
    }

    meta.width = s.integer(QStringLiteral("width"), 0);
    meta.height = s.integer(QStringLiteral("height"), 0);
    meta.pixelStride = s.integer(QStringLiteral("pixel_stride"), 2);
    meta.rowStride = s.integer(QStringLiteral("row_stride"), meta.width * meta.pixelStride);

    meta.cfa = s.integer(QStringLiteral("cfa_value"), -1);
    if (meta.cfa < 0 || meta.cfa > 3)
        meta.cfa = cfaFromName(s.value(QStringLiteral("cfa")).toString());

    const int dynamicWhite = s.integer(QStringLiteral("dynamic_white_level"), -1);
    meta.whiteLevel = dynamicWhite > 0 ? dynamicWhite : s.integer(QStringLiteral("white_level"), 0);

    QVector<double> black = numbers(s.value(QStringLiteral("dynamic_black_level")), 4);
    if (black.isEmpty() || allZero(black))
        black = numbers(s.value(QStringLiteral("black_level_pattern")), 4);
    for (int i = 0; i < black.size() && i < 4; ++i)
        meta.black[i] = black.at(i);

    const QVector<double> neutral = numbers(s.value(QStringLiteral("neutral_color_point")), 3);
    if (neutral.size() == 3 && neutral.at(0) > 0.0 && neutral.at(1) > 0.0 && neutral.at(2) > 0.0) {
        for (int i = 0; i < 3; ++i)
            meta.neutral[i] = neutral.at(i);
        meta.hasNeutral = true;
    }

    QVector<double> forward = numbers(s.value(QStringLiteral("forward_matrix2")), 9);
    if (forward.isEmpty() || allZero(forward))
        forward = numbers(s.value(QStringLiteral("forward_matrix1")), 9);
    if (forward.size() == 9 && !allZero(forward)) {
        for (int i = 0; i < 9; ++i)
            meta.forward[i] = forward.at(i);
        meta.hasForward = true;
    }

    const QStringList orientationKeys = { QStringLiteral("orientation"),
                                          QStringLiteral("rotation"),
                                          QStringLiteral("jpeg_orientation") };
    for (const QString &key : orientationKeys) {
        const int degrees = s.integer(key, -1);
        if (degrees == 0 || degrees == 90 || degrees == 180 || degrees == 270) {
            meta.orientation = degrees;
            break;
        }
    }

    if (!meta.isValid()) {
        if (error)
            *error = QStringLiteral("The RAW sidecar is missing size, layout or white level.");
        return false;
    }
    return true;
}
