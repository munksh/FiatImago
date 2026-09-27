#ifndef RAWMETA_H
#define RAWMETA_H

#include <QString>

struct RawMeta
{
    int width = 0;
    int height = 0;
    int rowStride = 0;
    int pixelStride = 2;
    int cfa = -1;
    double whiteLevel = 0.0;
    double black[4] = { 0.0, 0.0, 0.0, 0.0 };
    double neutral[3] = { 1.0, 1.0, 1.0 };
    bool hasNeutral = false;
    double forward[9] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
    bool hasForward = false;
    int orientation = -1;

    bool isValid() const;
};

// RAWfish writes a sidecar per raw16 and a second one per capture. Keys are
// read from the primary file first and the secondary fills whatever is
// missing, so either may carry a field.
bool readRawMeta(const QString &primaryJson, const QString &secondaryJson,
                 RawMeta &meta, QString *error);

#endif
