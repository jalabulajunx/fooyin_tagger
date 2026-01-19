#pragma once

#include <QMetaType>
#include <QString>
#include <QStringList>

namespace Tagger {

enum class SourceType
{
    Wikipedia,
    MusicBrainz,
    Discogs,  // Future
    GNUDB     // Future
};

enum class FetchStatus
{
    Idle,
    Fetching,
    Success,
    Error
};

struct TrackMetadata
{
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString lyricist;
    QString composer;
    QString musicDirector;
    int trackNumber{0};
    int totalTracks{0};
    int discNumber{1};
    int totalDiscs{1};
    int year{0};
    int durationSeconds{0};
    QString isrc;
    QString mbid;  // MusicBrainz ID
};

struct AlbumMetadata
{
    QString album;
    QString albumArtist;
    QString musicDirector;
    int year{0};
    QString country;
    QString releaseId;  // MusicBrainz release ID
    QString sourceUrl;
    SourceType source{SourceType::Wikipedia};
    QList<TrackMetadata> tracks;
};

} // namespace Tagger

Q_DECLARE_METATYPE(Tagger::SourceType)
Q_DECLARE_METATYPE(Tagger::FetchStatus)
Q_DECLARE_METATYPE(Tagger::TrackMetadata)
Q_DECLARE_METATYPE(Tagger::AlbumMetadata)
