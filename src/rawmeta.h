#ifndef RAWMETA_H
#define RAWMETA_H

#include <QString>

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
    int orientation = -1;

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
