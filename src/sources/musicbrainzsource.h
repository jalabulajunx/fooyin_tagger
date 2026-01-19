#pragma once

#include "metadatasource.h"

class QNetworkReply;
class QJsonDocument;

class MusicBrainzSource : public MetadataSource
{
    Q_OBJECT

public:
    explicit MusicBrainzSource(HttpClient* client, QObject* parent = nullptr);

    [[nodiscard]] QString name() const override { return QStringLiteral("MusicBrainz"); }
    [[nodiscard]] Tagger::SourceType type() const override { return Tagger::SourceType::MusicBrainz; }
    [[nodiscard]] bool supportsUrlInput() const override { return true; }
    [[nodiscard]] bool supportsSearch() const override { return true; }

    void fetchFromUrl(const QString& url) override;
    void searchAlbum(const QString& artist, const QString& album) override;
    void fetchRelease(const QString& mbid);
    void fetchReleaseGroup(const QString& mbid);
    void cancel() override;

    [[nodiscard]] bool isValidUrl(const QString& url) const override;

private slots:
    void onSearchReply(QNetworkReply* reply);
    void onReleaseReply(QNetworkReply* reply);
    void onReleaseGroupReply(QNetworkReply* reply);

private:
    static constexpr const char* API_BASE = "https://musicbrainz.org/ws/2";

    QUrl buildSearchUrl(const QString& artist, const QString& album) const;
    QUrl buildReleaseUrl(const QString& mbid) const;
    QUrl buildReleaseGroupUrl(const QString& mbid) const;

    QList<Tagger::AlbumMetadata> parseSearchResults(const QJsonDocument& json);
    Tagger::AlbumMetadata parseRelease(const QJsonDocument& json);
    Tagger::AlbumMetadata parseReleaseGroup(const QJsonDocument& json);

    QString extractMbidFromUrl(const QString& url) const;
    QString extractMbidFromUrl(const QString& url, QString& entityType) const;

    enum class RequestType { None, Search, Release, ReleaseGroup };

    QNetworkReply* m_currentReply{nullptr};
    RequestType m_currentRequestType{RequestType::None};
};
