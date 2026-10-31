#include "dng.h"

#include <QFile>
#include <QMap>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

enum Tag {
    NewSubfileType = 254, ImageWidth = 256, ImageLength = 257, BitsPerSample = 258,
    Compression = 259, Photometric = 262, StripOffsets = 273, Orientation = 274,
    SamplesPerPixel = 277, StripByteCounts = 279, TileWidth = 322, SubIFDs = 330,
    CfaRepeatPatternDim = 33421, CfaPattern = 33422,
    DngVersion = 50706, BlackLevelRepeatDim = 50713, BlackLevel = 50714, WhiteLevel = 50717,
    ColorMatrix1 = 50721, ColorMatrix2 = 50722, AsShotNeutral = 50728,
    CalibrationIlluminant1 = 50778, CalibrationIlluminant2 = 50779,
    ForwardMatrix1 = 50964, ForwardMatrix2 = 50965, OpcodeList2 = 51009
};

const int CfaPhotometric = 32803;

struct Entry
{
    int type = 0;
    quint32 count = 0;
    QByteArray bytes;       // the value, in the file's byte order
};

class Tiff
{
public:
    explicit Tiff(QFile &file) : m_file(file) { }

    bool open()
    {
        const QByteArray head = readAt(0, 8);
        if (head.size() < 8)
            return false;
        if (head.startsWith("II"))
            m_big = false;
        else if (head.startsWith("MM"))
            m_big = true;
        else
            return false;
        return u16(head.constData() + 2) == 42;
    }

    bool bigEndian() const { return m_big; }
    quint32 firstIfd() const { return u32(readAt(4, 4).constData()); }

    // One directory's tags, each value read in full whether it sits in the
    // entry or elsewhere in the file.
    QMap<int, Entry> directory(quint32 offset) const
    {
        QMap<int, Entry> tags;
        const QByteArray countBytes = readAt(offset, 2);
        if (countBytes.size() < 2)
            return tags;
        const int count = u16(countBytes.constData());
        const QByteArray table = readAt(offset + 2, count * 12);
        if (table.size() < count * 12)
            return tags;
        for (int i = 0; i < count; ++i) {
            const char *e = table.constData() + i * 12;
            Entry entry;
            entry.type = u16(e + 2);
            entry.count = u32(e + 4);
            const qint64 size = qint64(typeSize(entry.type)) * entry.count;
            if (size <= 0 || size > 64 * 1024 * 1024)
                continue;
            entry.bytes = size <= 4 ? QByteArray(e + 8, int(size)) : readAt(u32(e + 8), size);
            if (entry.bytes.size() == size)
                tags.insert(u16(e), entry);
        }
        return tags;
    }

    // Every value of a numeric tag as doubles, rationals divided out.
    QVector<double> numbers(const QMap<int, Entry> &tags, int tag) const
    {
        QVector<double> out;
        if (!tags.contains(tag))
            return out;
        const Entry &e = tags.value(tag);
        const char *p = e.bytes.constData();
        for (quint32 i = 0; i < e.count; ++i) {
            switch (e.type) {
            case 1: case 7: out << double(quint8(p[i])); break;
            case 6: out << double(qint8(p[i])); break;
            case 3: out << double(u16(p + i * 2)); break;
            case 8: out << double(qint16(u16(p + i * 2))); break;
            case 4: case 13: out << double(u32(p + i * 4)); break;
            case 9: out << double(qint32(u32(p + i * 4))); break;
            case 5: case 10: {
                const quint32 n = u32(p + i * 8);
                const quint32 d = u32(p + i * 8 + 4);
                const double num = e.type == 10 ? double(qint32(n)) : double(n);
                const double den = e.type == 10 ? double(qint32(d)) : double(d);
                out << (den != 0.0 ? num / den : 0.0);
                break;
            }
            case 11: {
                const quint32 bits = u32(p + i * 4);
                float f;
                std::memcpy(&f, &bits, 4);
                out << double(f);
                break;
            }
            case 12: {
                quint64 bits = 0;
                for (int b = 0; b < 8; ++b) {
                    const quint64 byte = quint8(p[i * 8 + b]);
                    bits |= m_big ? byte << (56 - 8 * b) : byte << (8 * b);
                }
                double d;
                std::memcpy(&d, &bits, 8);
                out << d;
                break;
            }
            default:
                return QVector<double>();
            }
        }
        return out;
    }

    double number(const QMap<int, Entry> &tags, int tag, double fallback) const
    {
        const QVector<double> v = numbers(tags, tag);
        return v.isEmpty() ? fallback : v.first();
    }

private:
    static int typeSize(int type)
    {
        switch (type) {
        case 1: case 2: case 6: case 7: return 1;
        case 3: case 8: return 2;
        case 4: case 9: case 11: case 13: return 4;
        case 5: case 10: case 12: return 8;
        default: return 0;
        }
    }

    QByteArray readAt(qint64 offset, qint64 size) const
    {
        if (!m_file.seek(offset))
            return QByteArray();
        return m_file.read(size);
    }

    quint16 u16(const char *p) const
    {
        return m_big ? qFromBigEndian<quint16>(reinterpret_cast<const uchar *>(p))
                     : qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(p));
    }

    quint32 u32(const char *p) const
    {
        return m_big ? qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(p))
                     : qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(p));
    }

    QFile &m_file;
    bool m_big = false;
};

// The opcode list is big-endian whatever the file's byte order.
quint32 be32(const char *p) { return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(p)); }

double be64f(const char *p)
{
    const quint64 bits = qFromBigEndian<quint64>(reinterpret_cast<const uchar *>(p));
    double d;
    std::memcpy(&d, &bits, 8);
    return d;
}

float be32f(const char *p)
{
    const quint32 bits = be32(p);
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

// GainMap opcodes (id 9) for single CFA positions, as RAWfish and Android
// write them: one map per position, a row and column pitch of 2.
void readGainMaps(const QByteArray &list, RawMeta &meta)
{
    if (list.size() < 4)
        return;
    const char *p = list.constData();
    const quint32 count = be32(p);
    int pos = 4;
    for (quint32 i = 0; i < count && pos + 16 <= list.size(); ++i) {
        const quint32 id = be32(p + pos);
        const quint32 size = be32(p + pos + 12);
        const int start = pos + 16;
        pos = start + int(size);
        if (pos > list.size())
            return;
        if (id != 9 || size < 76)
            continue;
        const char *q = p + start;
        ShadingMap map;
        map.top = int(be32(q));
        map.left = int(be32(q + 4));
        map.bottom = int(be32(q + 8));
        map.right = int(be32(q + 12));
        const quint32 rowPitch = be32(q + 24);
        const quint32 colPitch = be32(q + 28);
        map.rows = int(be32(q + 32));
        map.cols = int(be32(q + 36));
        map.spacingV = be64f(q + 40);
        map.spacingH = be64f(q + 48);
        map.originV = be64f(q + 56);
        map.originH = be64f(q + 64);
        const quint32 mapPlanes = be32(q + 72);
        if (rowPitch != 2 || colPitch != 2 || mapPlanes != 1 || map.rows <= 0 || map.cols <= 0
                || quint32(76 + 4 * map.rows * map.cols) > size)
            continue;
        for (int k = 0; k < map.rows * map.cols; ++k)
            map.gains << be32f(q + 76 + 4 * k);
        if (!map.isValid())
            continue;
        meta.shading[((map.top & 1) << 1) | (map.left & 1)] = map;
    }

    // RAWfish's maps sit near 0.45 at the centre and rise towards the edges.
    // Applied as written, white would land at 0.45 and highlights, clipping
    // and recovery would all work on the wrong level; scaled so the least
    // gain is 1, the centre keeps the sensor's own white and only the edges
    // are lifted.
    float lowest = 0.0f;
    int found = 0;
    for (const ShadingMap &m : meta.shading) {
        if (!m.isValid())
            continue;
        ++found;
        const float least = *std::min_element(m.gains.constBegin(), m.gains.constEnd());
        lowest = found == 1 ? least : std::min(lowest, least);
    }
    if (found != 4 || lowest <= 0.0f)
        return;
    for (ShadingMap &m : meta.shading) {
        for (float &g : m.gains)
            g /= lowest;
    }
    meta.hasShading = true;
}

double illuminantKelvin(int code)
{
    switch (code) {
    case 17: return 2856.0;   // Standard light A
    case 3: return 2850.0;    // Tungsten
    case 24: return 3200.0;   // ISO studio tungsten
    case 15: return 3450.0;   // White fluorescent
    case 2: case 14: return 4150.0;
    case 18: return 4874.0;   // Standard light B
    case 13: return 5000.0;
    case 23: return 5003.0;   // D50
    case 1: case 4: case 9: return 5500.0;
    case 20: return 5503.0;   // D55
    case 12: return 6430.0;
    case 10: return 6500.0;
    case 21: return 6504.0;   // D65
    case 19: return 6774.0;   // Standard light C
    case 11: return 7500.0;
    case 22: return 7504.0;   // D75
    default: return 0.0;
    }
}

bool solve3(const double m[9], const double b[3], double x[3])
{
    const double det = m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6])
            + m[2] * (m[3] * m[7] - m[4] * m[6]);
    if (std::abs(det) < 1e-12)
        return false;
    const double inv[9] = {
        (m[4] * m[8] - m[5] * m[7]) / det, (m[2] * m[7] - m[1] * m[8]) / det, (m[1] * m[5] - m[2] * m[4]) / det,
        (m[5] * m[6] - m[3] * m[8]) / det, (m[0] * m[8] - m[2] * m[6]) / det, (m[2] * m[3] - m[0] * m[5]) / det,
        (m[3] * m[7] - m[4] * m[6]) / det, (m[1] * m[6] - m[0] * m[7]) / det, (m[0] * m[4] - m[1] * m[3]) / det
    };
    for (int r = 0; r < 3; ++r)
        x[r] = inv[r * 3] * b[0] + inv[r * 3 + 1] * b[1] + inv[r * 3 + 2] * b[2];
    return true;
}

// How far towards the first calibration light the shot's light is, as the
// DNG specification weighs it: by inverse colour temperature, the
// temperature found from the white balance through the colour matrices.
double lightWeight(const double cm1[9], const double cm2[9], double t1, double t2, const double neutral[3])
{
    double weight = 0.5;
    for (int round = 0; round < 20; ++round) {
        double cm[9];
        for (int i = 0; i < 9; ++i)
            cm[i] = weight * cm1[i] + (1.0 - weight) * cm2[i];
        double xyz[3];
        if (!solve3(cm, neutral, xyz))
            return weight;
        const double sum = xyz[0] + xyz[1] + xyz[2];
        if (sum <= 0.0)
            return weight;
        const double x = xyz[0] / sum;
        const double y = xyz[1] / sum;
        // McCamy's approximation is plenty to pick between two lights.
        const double n = (x - 0.3320) / (0.1858 - y);
        const double kelvin = std::max(1000.0, ((449.0 * n + 3525.0) * n + 6823.3) * n + 5520.33);
        double next;
        if (kelvin <= std::min(t1, t2))
            next = t1 < t2 ? 1.0 : 0.0;
        else if (kelvin >= std::max(t1, t2))
            next = t1 > t2 ? 1.0 : 0.0;
        else
            next = (1.0 / kelvin - 1.0 / t2) / (1.0 / t1 - 1.0 / t2);
        if (std::abs(next - weight) < 1e-6)
            return next;
        weight = next;
    }
    return weight;
}

int orientationDegrees(int tiff)
{
    switch (tiff) {
    case 1: return 0;
    case 6: return 90;
    case 3: return 180;
    case 8: return 270;
    default: return -1;     // mirrored: leave it to the photographer
    }
}

bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

}

bool readDngMeta(const QString &path, RawMeta &meta, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return fail(error, QStringLiteral("The DNG file could not be opened."));
    Tiff tiff(file);
    if (!tiff.open())
        return fail(error, QStringLiteral("The file is not a DNG."));

    const QMap<int, Entry> ifd0 = tiff.directory(tiff.firstIfd());
    if (!ifd0.contains(DngVersion))
        return fail(error, QStringLiteral("The file is not a DNG."));

    // The CFA image is the first directory in RAWfish's files; other
    // writers put a preview there and the RAW in a sub-directory.
    QMap<int, Entry> image = ifd0;
    if (int(tiff.number(ifd0, Photometric, 0)) != CfaPhotometric) {
        image.clear();
        for (double offset : tiff.numbers(ifd0, SubIFDs)) {
            const QMap<int, Entry> sub = tiff.directory(quint32(offset));
            if (int(tiff.number(sub, NewSubfileType, 1)) == 0
                    && int(tiff.number(sub, Photometric, 0)) == CfaPhotometric) {
                image = sub;
                break;
            }
        }
        if (image.isEmpty())
            return fail(error, QStringLiteral("The DNG holds no CFA image."));
    }

    if (int(tiff.number(image, Compression, 1)) != 1 || image.contains(TileWidth))
        return fail(error, QStringLiteral("Only uncompressed DNG files can be read."));
    if (int(tiff.number(image, BitsPerSample, 0)) != 16 || int(tiff.number(image, SamplesPerPixel, 1)) != 1)
        return fail(error, QStringLiteral("Only 16-bit DNG files can be read."));

    meta.width = int(tiff.number(image, ImageWidth, 0));
    meta.height = int(tiff.number(image, ImageLength, 0));
    meta.bits = 16;
    meta.pixelStride = 2;
    meta.rowStride = meta.width * 2;
    meta.bigEndian = tiff.bigEndian();

    // The pixels in one piece: strips are accepted when they follow each
    // other, as every writer that uses several of them lays them out.
    const QVector<double> offsets = tiff.numbers(image, StripOffsets);
    const QVector<double> counts = tiff.numbers(image, StripByteCounts);
    if (offsets.isEmpty() || offsets.size() != counts.size())
        return fail(error, QStringLiteral("The DNG has no pixel data."));
    for (int i = 1; i < offsets.size(); ++i) {
        if (offsets.at(i) != offsets.at(i - 1) + counts.at(i - 1))
            return fail(error, QStringLiteral("The DNG's pixel data is scattered."));
    }
    meta.dataOffset = qint64(offsets.first());

    const QVector<double> dim = tiff.numbers(image, CfaRepeatPatternDim);
    const QVector<double> pattern = tiff.numbers(image, CfaPattern);
    if (dim.size() != 2 || dim.at(0) != 2 || dim.at(1) != 2 || pattern.size() != 4)
        return fail(error, QStringLiteral("Only 2x2 Bayer DNG files can be read."));
    static const int layouts[4][4] = { { 0, 1, 1, 2 }, { 1, 0, 2, 1 }, { 1, 2, 0, 1 }, { 2, 1, 1, 0 } };
    for (int c = 0; c < 4; ++c) {
        if (int(pattern.at(0)) == layouts[c][0] && int(pattern.at(1)) == layouts[c][1]
                && int(pattern.at(2)) == layouts[c][2] && int(pattern.at(3)) == layouts[c][3])
            meta.cfa = c;
    }

    // Levels may sit in the image's directory or, by some writers, in the
    // first one.
    const QMap<int, Entry> &levels = image.contains(WhiteLevel) ? image : ifd0;
    meta.whiteLevel = tiff.number(levels, WhiteLevel, 65535.0);
    const QVector<double> black = tiff.numbers(levels, BlackLevel);
    const QVector<double> blackDim = tiff.numbers(levels, BlackLevelRepeatDim);
    if (black.size() == 4 && blackDim.size() == 2 && blackDim.at(0) == 2 && blackDim.at(1) == 2) {
        for (int i = 0; i < 4; ++i)
            meta.black[i] = black.at(i);
    } else if (!black.isEmpty()) {
        for (int i = 0; i < 4; ++i)
            meta.black[i] = black.first();
    }

    const QVector<double> neutral = tiff.numbers(ifd0, AsShotNeutral);
    if (neutral.size() == 3 && neutral.at(0) > 0.0 && neutral.at(1) > 0.0 && neutral.at(2) > 0.0) {
        for (int i = 0; i < 3; ++i)
            meta.neutral[i] = neutral.at(i);
        meta.hasNeutral = true;
    }

    // The forward matrix for the shot's light, between the two the camera
    // was calibrated under.
    const QVector<double> fm1 = tiff.numbers(ifd0, ForwardMatrix1);
    const QVector<double> fm2 = tiff.numbers(ifd0, ForwardMatrix2);
    const QVector<double> cm1 = tiff.numbers(ifd0, ColorMatrix1);
    const QVector<double> cm2 = tiff.numbers(ifd0, ColorMatrix2);
    if (fm1.size() == 9 || fm2.size() == 9) {
        double weight = fm1.size() == 9 ? 1.0 : 0.0;
        if (fm1.size() == 9 && fm2.size() == 9 && cm1.size() == 9 && cm2.size() == 9 && meta.hasNeutral) {
            const double t1 = illuminantKelvin(int(tiff.number(ifd0, CalibrationIlluminant1, 0)));
            const double t2 = illuminantKelvin(int(tiff.number(ifd0, CalibrationIlluminant2, 0)));
            if (t1 > 0.0 && t2 > 0.0 && t1 != t2)
                weight = lightWeight(cm1.constData(), cm2.constData(), t1, t2, meta.neutral);
        }
        for (int i = 0; i < 9; ++i) {
            const double a = fm1.size() == 9 ? fm1.at(i) : 0.0;
            const double b = fm2.size() == 9 ? fm2.at(i) : 0.0;
            meta.forward[i] = weight * a + (1.0 - weight) * b;
        }
        meta.hasForward = true;
    }

    meta.orientation = orientationDegrees(int(tiff.number(ifd0, Orientation, 1)));

    const QMap<int, Entry> &opcodes = image.contains(OpcodeList2) ? image : ifd0;
    if (opcodes.contains(OpcodeList2))
        readGainMaps(opcodes.value(OpcodeList2).bytes, meta);

    if (!meta.isValid())
        return fail(error, QStringLiteral("The DNG is missing size, layout or white level."));
    if (meta.dataOffset + qint64(meta.height) * meta.rowStride > file.size())
        return fail(error, QStringLiteral("The DNG file is shorter than its tags say."));
    return true;
}
