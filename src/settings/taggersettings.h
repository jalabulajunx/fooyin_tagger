#pragma once

#include <tagger/tagger_common.h>
#include <QMetaType>

namespace TaggerSettings {
Q_NAMESPACE

enum Setting : uint32_t
{
    // Int settings (type code 2)
    DefaultSource       = 2 << 28 | 1,
    DurationTolerance   = 2 << 28 | 2,
    WindowWidth         = 2 << 28 | 3,
    WindowHeight        = 2 << 28 | 4,

    // Int settings (continued) - confidence stored as percentage 0-100
    ConfidenceThreshold = 2 << 28 | 5,

    // Bool settings (type code 1)
    WriteTitle          = 1 << 28 | 1,
    WriteArtist         = 1 << 28 | 2,
    WriteAlbum          = 1 << 28 | 3,
    WriteLyrics         = 1 << 28 | 4,
    WriteYear           = 1 << 28 | 5,
    WriteTrackNumber    = 1 << 28 | 6,
};

Q_ENUM_NS(Setting)
}

Q_DECLARE_METATYPE(TaggerSettings::Setting)
