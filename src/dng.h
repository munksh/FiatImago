#ifndef DNG_H
#define DNG_H

#include <QString>

#include "rawmeta.h"

// Reads what developing needs from an uncompressed DNG: the CFA image's
// size, layout and offset, black and white levels, white balance, colour
// (the forward matrix for the shot's light), orientation and the lens
// shading GainMap. Compressed and tiled DNGs are refused with a message.
bool readDngMeta(const QString &path, RawMeta &meta, QString *error);

#endif
