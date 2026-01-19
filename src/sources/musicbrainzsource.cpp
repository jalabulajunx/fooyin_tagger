#include "musicbrainzsource.h"
#include "core/httpclient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

MusicBrainzSource::MusicBrainzSource(HttpClient* client, QObject* parent)
    : MetadataSource(client, parent)
{
    // Set rate limit for MusicBrainz (1 request per second)
    m_httpClient->setRateLimit(1000);
}

bool MusicBrainzSource::isValidUrl(const QString& url) const
{
    return !extractMbidFromUrl(url).isEmpty();
}

QString MusicBrainzSource::extractMbidFromUrl(const QString& url, QString& entityType) const
{
    // Match URLs like:
    // https://musicbrainz.org/release/12345678-1234-1234-1234-123456789012
    // https://musicbrainz.org/release-group/12345678-1234-1234-1234-123456789012
    // musicbrainz.org/release/12345678-1234-1234-1234-123456789012
    static QRegularExpression mbidRe(
        R"(musicbrainz\.org/(release|release-group)/([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}))",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatch match = mbidRe.match(url);
    if(match.hasMatch()) {
        entityType = match.captured(1).toLower();
        return match.captured(2);
    }

    // Also accept raw MBID (assume release by default)
    static QRegularExpression rawMbidRe(R"(^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$)",
                                        QRegularExpression::CaseInsensitiveOption);
    if(rawMbidRe.match(url).hasMatch()) {
        entityType = "release";
        return url;
    }

    return QString();
}

QString MusicBrainzSource::extractMbidFromUrl(const QString& url) const
{
    QString entityType;
    return extractMbidFromUrl(url, entityType);
}

void MusicBrainzSource::fetchFromUrl(const QString& url)
{
    QString entityType;
    QString mbid = extractMbidFromUrl(url, entityType);
    if(mbid.isEmpty()) {
        emit fetchFailed(tr("Invalid MusicBrainz URL or ID"));
        return;
    }

    if(entityType == "release") {
        fetchRelease(mbid);
    }
    else if(entityType == "release-group") {
        fetchReleaseGroup(mbid);
    }
    else {
        emit fetchFailed(tr("Unsupported MusicBrainz entity type: %1").arg(entityType));
    }
}

void MusicBrainzSource::searchAlbum(const QString& artist, const QString& album)
{
    if(artist.isEmpty() && album.isEmpty()) {
        emit fetchFailed(tr("Please provide artist and/or album name"));
        return;
    }

    cancel();

    emit fetchStarted();
    m_currentRequestType = RequestType::Search;

    QUrl searchUrl = buildSearchUrl(artist, album);
    m_currentReply = m_httpClient->get(searchUrl);

    if(m_currentReply) {
        connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
            onSearchReply(m_currentReply);
        });
    }
}

void MusicBrainzSource::fetchRelease(const QString& mbid)
{
    cancel();

    emit fetchStarted();
    m_currentRequestType = RequestType::Release;

    QUrl releaseUrl = buildReleaseUrl(mbid);
    m_currentReply = m_httpClient->get(releaseUrl);

    if(m_currentReply) {
        connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
            onReleaseReply(m_currentReply);
        });
    }
}

void MusicBrainzSource::fetchReleaseGroup(const QString& mbid)
{
    cancel();

    emit fetchStarted();
    m_currentRequestType = RequestType::ReleaseGroup;

    QUrl releaseGroupUrl = buildReleaseGroupUrl(mbid);
    m_currentReply = m_httpClient->get(releaseGroupUrl);

    if(m_currentReply) {
        connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
            onReleaseGroupReply(m_currentReply);
        });
    }
}

void MusicBrainzSource::cancel()
{
    if(m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    m_currentRequestType = RequestType::None;
}

QUrl MusicBrainzSource::buildSearchUrl(const QString& artist, const QString& album) const
{
    QUrl url(QString("%1/release").arg(API_BASE));
    QUrlQuery query;

    // Build Lucene query
    QStringList queryParts;
    if(!artist.isEmpty()) {
        queryParts.append(QString("artist:\"%1\"").arg(artist));
    }
    if(!album.isEmpty()) {
        queryParts.append(QString("release:\"%1\"").arg(album));
    }

    query.addQueryItem("query", queryParts.join(" AND "));
    query.addQueryItem("fmt", "json");
    query.addQueryItem("limit", "25");

    url.setQuery(query);

    qDebug() << "MusicBrainz search URL:" << url.toString();
    return url;
}

QUrl MusicBrainzSource::buildReleaseUrl(const QString& mbid) const
{
    QUrl url(QString("%1/release/%2").arg(API_BASE, mbid));
    QUrlQuery query;

    // Include recordings and artist credits
    query.addQueryItem("inc", "recordings+artist-credits+labels+release-groups");
    query.addQueryItem("fmt", "json");

    url.setQuery(query);

    qDebug() << "MusicBrainz release URL:" << url.toString();
    return url;
}

QUrl MusicBrainzSource::buildReleaseGroupUrl(const QString& mbid) const
{
    QUrl url(QString("%1/release-group/%2").arg(API_BASE, mbid));
    QUrlQuery query;

    // Include releases, recordings, and artist credits
    query.addQueryItem("inc", "releases+recordings+artist-credits+labels");
    query.addQueryItem("fmt", "json");

    url.setQuery(query);

    qDebug() << "MusicBrainz release group URL:" << url.toString();
    return url;
}

void MusicBrainzSource::onSearchReply(QNetworkReply* reply)
{
    if(reply != m_currentReply) {
        return;
    }

    m_currentReply = nullptr;
    m_currentRequestType = RequestType::None;

    if(reply->error() != QNetworkReply::NoError) {
        emit fetchFailed(tr("Search failed: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument json = QJsonDocument::fromJson(data, &parseError);

    if(parseError.error != QJsonParseError::NoError) {
        emit fetchFailed(tr("Failed to parse search results: %1").arg(parseError.errorString()));
        return;
    }

    QList<Tagger::AlbumMetadata> results = parseSearchResults(json);

    if(results.isEmpty()) {
        emit fetchFailed(tr("No releases found"));
        return;
    }

    emit searchResults(results);
}

void MusicBrainzSource::onReleaseGroupReply(QNetworkReply* reply)
{
    if(reply != m_currentReply) {
        return;
    }

    m_currentReply = nullptr;
    m_currentRequestType = RequestType::None;

    if(reply->error() != QNetworkReply::NoError) {
        emit fetchFailed(tr("Failed to fetch release group: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    emit fetchProgress(50);

    QJsonParseError parseError;
    QJsonDocument json = QJsonDocument::fromJson(data, &parseError);

    if(parseError.error != QJsonParseError::NoError) {
        emit fetchFailed(tr("Failed to parse release group data: %1").arg(parseError.errorString()));
        return;
    }

    Tagger::AlbumMetadata metadata = parseReleaseGroup(json);

    if(metadata.tracks.isEmpty()) {
        emit fetchFailed(tr("Release group has no tracks"));
        return;
    }

    emit fetchProgress(100);
    emit fetchCompleted(metadata);
}

void MusicBrainzSource::onReleaseReply(QNetworkReply* reply)
{
    if(reply != m_currentReply) {
        return;
    }

    m_currentReply = nullptr;
    m_currentRequestType = RequestType::None;

    if(reply->error() != QNetworkReply::NoError) {
        emit fetchFailed(tr("Failed to fetch release: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    emit fetchProgress(50);

    QJsonParseError parseError;
    QJsonDocument json = QJsonDocument::fromJson(data, &parseError);

    if(parseError.error != QJsonParseError::NoError) {
        emit fetchFailed(tr("Failed to parse release data: %1").arg(parseError.errorString()));
        return;
    }

    Tagger::AlbumMetadata metadata = parseRelease(json);

    if(metadata.tracks.isEmpty()) {
        emit fetchFailed(tr("Release has no tracks"));
        return;
    }

    emit fetchProgress(100);
    emit fetchCompleted(metadata);
}

QList<Tagger::AlbumMetadata> MusicBrainzSource::parseSearchResults(const QJsonDocument& json)
{
    QList<Tagger::AlbumMetadata> results;

    QJsonObject root = json.object();
    QJsonArray releases = root["releases"].toArray();

    for(const QJsonValue& releaseVal : releases) {
        QJsonObject release = releaseVal.toObject();

        Tagger::AlbumMetadata metadata;
        metadata.source = Tagger::SourceType::MusicBrainz;
        metadata.releaseId = release["id"].toString();
        metadata.album = release["title"].toString();
        metadata.country = release["country"].toString();

        // Parse date
        QString date = release["date"].toString();
        if(!date.isEmpty()) {
            metadata.year = date.left(4).toInt();
        }

        // Parse artist credit
        QJsonArray artistCredit = release["artist-credit"].toArray();
        QStringList artists;
        for(const QJsonValue& creditVal : artistCredit) {
            QJsonObject credit = creditVal.toObject();
            QJsonObject artist = credit["artist"].toObject();
            artists.append(artist["name"].toString());
        }
        metadata.albumArtist = artists.join(", ");

        metadata.sourceUrl = QString("https://musicbrainz.org/release/%1").arg(metadata.releaseId);

        results.append(metadata);
    }

    qDebug() << "Parsed" << results.size() << "search results";
    return results;
}

Tagger::AlbumMetadata MusicBrainzSource::parseReleaseGroup(const QJsonDocument& json)
{
    Tagger::AlbumMetadata metadata;
    metadata.source = Tagger::SourceType::MusicBrainz;

    QJsonObject root = json.object();

    metadata.releaseId = root["id"].toString();
    metadata.album = root["title"].toString();
    metadata.sourceUrl = QString("https://musicbrainz.org/release-group/%1").arg(metadata.releaseId);

    // Parse date
    QString date = root["first-release-date"].toString();
    if(!date.isEmpty()) {
        metadata.year = date.left(4).toInt();
    }

    // Parse album artist
    QJsonArray artistCredit = root["artist-credit"].toArray();
    QStringList artists;
    for(const QJsonValue& creditVal : artistCredit) {
        QJsonObject credit = creditVal.toObject();
        QJsonObject artist = credit["artist"].toObject();
        artists.append(artist["name"].toString());
    }
    metadata.albumArtist = artists.join(", ");

    // Get first release with tracks
    QJsonArray releases = root["releases"].toArray();
    for(const QJsonValue& releaseVal : releases) {
        QJsonObject release = releaseVal.toObject();
        // For simplicity, pick the first release with media and tracks
        if(release.contains("media")) {
            QJsonArray media = release["media"].toArray();
            bool hasTracks = false;
            for(const QJsonValue& mediaVal : media) {
                QJsonObject disc = mediaVal.toObject();
                if(disc.contains("tracks") && disc["tracks"].toArray().size() > 0) {
                    hasTracks = true;
                    break;
                }
            }
            if(hasTracks) {
                // If we have a full release object with media/tracks, parse it
                if(release.contains("media")) {
                    int totalDiscs = media.size();
                    int globalTrackNum = 0;

                    for(int discNum = 0; discNum < media.size(); ++discNum) {
                        QJsonObject disc = media[discNum].toObject();
                        QJsonArray tracks = disc["tracks"].toArray();
                        int totalTracksOnDisc = tracks.size();

                        for(int trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
                            QJsonObject trackObj = tracks[trackIdx].toObject();

                            Tagger::TrackMetadata track;
                            track.album = metadata.album;
                            track.albumArtist = metadata.albumArtist;
                            track.year = metadata.year;

                            // Track title (prefer track title over recording title)
                            track.title = trackObj["title"].toString();

                            // Track position
                            track.trackNumber = trackObj["position"].toInt();
                            track.totalTracks = totalTracksOnDisc;
                            track.discNumber = discNum + 1;
                            track.totalDiscs = totalDiscs;

                            // Duration (in milliseconds from API, convert to seconds)
                            int lengthMs = trackObj["length"].toInt();
                            track.durationSeconds = lengthMs / 1000;

                            // Recording info
                            QJsonObject recording = trackObj["recording"].toObject();
                            track.mbid = recording["id"].toString();

                            // Track artist credit
                            QJsonArray trackArtistCredit = trackObj["artist-credit"].toArray();
                            if(trackArtistCredit.isEmpty()) {
                                trackArtistCredit = recording["artist-credit"].toArray();
                            }

                            QStringList trackArtists;
                            for(const QJsonValue& creditVal : trackArtistCredit) {
                                QJsonObject credit = creditVal.toObject();
                                QJsonObject artist = credit["artist"].toObject();
                                trackArtists.append(artist["name"].toString());
                            }
                            track.artist = trackArtists.join(", ");

                            if(track.artist.isEmpty()) {
                                track.artist = metadata.albumArtist;
                            }

                            metadata.tracks.append(track);
                            globalTrackNum++;
                        }
                    }
                }
                break;
            }
        }
    }

    qDebug() << "Parsed release group:" << metadata.album
             << "by" << metadata.albumArtist
             << "with" << metadata.tracks.size() << "tracks";

    return metadata;
}

Tagger::AlbumMetadata MusicBrainzSource::parseRelease(const QJsonDocument& json)
{
    Tagger::AlbumMetadata metadata;
    metadata.source = Tagger::SourceType::MusicBrainz;

    QJsonObject root = json.object();

    metadata.releaseId = root["id"].toString();
    metadata.album = root["title"].toString();
    metadata.country = root["country"].toString();
    metadata.sourceUrl = QString("https://musicbrainz.org/release/%1").arg(metadata.releaseId);

    // Parse date
    QString date = root["date"].toString();
    if(!date.isEmpty()) {
        metadata.year = date.left(4).toInt();
    }

    // Parse album artist
    QJsonArray artistCredit = root["artist-credit"].toArray();
    QStringList artists;
    for(const QJsonValue& creditVal : artistCredit) {
        QJsonObject credit = creditVal.toObject();
        QJsonObject artist = credit["artist"].toObject();
        artists.append(artist["name"].toString());
    }
    metadata.albumArtist = artists.join(", ");

    // Parse media (discs) and tracks
    QJsonArray media = root["media"].toArray();
    int totalDiscs = media.size();
    int globalTrackNum = 0;

    for(int discNum = 0; discNum < media.size(); ++discNum) {
        QJsonObject disc = media[discNum].toObject();
        QJsonArray tracks = disc["tracks"].toArray();
        int totalTracksOnDisc = tracks.size();

        for(int trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
            QJsonObject trackObj = tracks[trackIdx].toObject();

            Tagger::TrackMetadata track;
            track.album = metadata.album;
            track.albumArtist = metadata.albumArtist;
            track.year = metadata.year;

            // Track title (prefer track title over recording title)
            track.title = trackObj["title"].toString();

            // Track position
            track.trackNumber = trackObj["position"].toInt();
            track.totalTracks = totalTracksOnDisc;
            track.discNumber = discNum + 1;
            track.totalDiscs = totalDiscs;

            // Duration (in milliseconds from API, convert to seconds)
            int lengthMs = trackObj["length"].toInt();
            track.durationSeconds = lengthMs / 1000;

            // Recording info
            QJsonObject recording = trackObj["recording"].toObject();
            track.mbid = recording["id"].toString();

            // Track artist credit
            QJsonArray trackArtistCredit = trackObj["artist-credit"].toArray();
            if(trackArtistCredit.isEmpty()) {
                trackArtistCredit = recording["artist-credit"].toArray();
            }

            QStringList trackArtists;
            for(const QJsonValue& creditVal : trackArtistCredit) {
                QJsonObject credit = creditVal.toObject();
                QJsonObject artist = credit["artist"].toObject();
                trackArtists.append(artist["name"].toString());
            }
            track.artist = trackArtists.join(", ");

            if(track.artist.isEmpty()) {
                track.artist = metadata.albumArtist;
            }

            metadata.tracks.append(track);
            globalTrackNum++;
        }
    }

    qDebug() << "Parsed release:" << metadata.album
             << "by" << metadata.albumArtist
             << "with" << metadata.tracks.size() << "tracks";

    return metadata;
}
