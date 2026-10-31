#ifndef RAWMETA_H
#define RAWMETA_H

#include <QString>
#include <QVector>

// A DNG GainMap for one CFA position: a rows x cols grid of gains over the
// area [top, bottom) x [left, right), placed by origin and spacing in
// fractions of that area.
struct ShadingMap
{
    int rows = 0;
    int cols = 0;
    int top = 0;
    int left = 0;
    int bottom = 0;
    int right = 0;
    double originV = 0.0;
    double originH = 0.0;
    double spacingV = 0.0;
    double spacingH = 0.0;
    QVector<float> gains;

    bool isValid() const { return rows > 0 && cols > 0 && gains.size() == rows * cols && bottom > top && right > left; }
};

struct RawMeta
{
    int width = 0;
    int height = 0;
    int rowStride = 0;
    int pixelStride = 0;
    int bits = 0;
    int cfa = -1;
    double whiteLevel = 0.0;
    double black[4] = { 0.0, 0.0, 0.0, 0.0 };
    double neutral[3] = { 1.0, 1.0, 1.0 };
    bool hasNeutral = false;
    double forward[9] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
    bool hasForward = false;
    double capture[9] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
    bool hasCapture = false;
    int orientation = -1;

    // Where the samples start in the file, and their byte order: a DNG keeps
    // its pixels after the tags, in the file's own byte order.
    qint64 dataOffset = 0;
    bool bigEndian = false;
    // Lens shading per CFA position (index ((y & 1) << 1) | (x & 1)).
    ShadingMap shading[4];
    bool hasShading = false;

    bool isValid() const;
    int bytesPerRow() const;
};

// RAWfish writes a sidecar per raw file and a second one per capture. Keys
// are read from the primary file first and the secondary fills whatever is
// missing, so either may carry a field. The packing (RAW8, RAW10, RAW12 or
// RAW16) comes from a "format" key when there is one, else from the file's
// extension, else from the row layout.
bool readRawMeta(const QString &rawPath, const QString &primaryJson,
                 const QString &secondaryJson, RawMeta &meta, QString *error);

#endif
