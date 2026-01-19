#pragma once

#include "metadatasource.h"

class QNetworkReply;

class WikipediaSource : public MetadataSource
{
    Q_OBJECT

public:
    explicit WikipediaSource(HttpClient* client, QObject* parent = nullptr);

    [[nodiscard]] QString name() const override { return QStringLiteral("Wikipedia"); }
    [[nodiscard]] Tagger::SourceType type() const override { return Tagger::SourceType::Wikipedia; }
    [[nodiscard]] bool supportsUrlInput() const override { return true; }
    [[nodiscard]] bool supportsSearch() const override { return false; }

    void fetchFromUrl(const QString& url) override;
    void searchAlbum(const QString& artist, const QString& album) override;
    void cancel() override;

    [[nodiscard]] bool isValidUrl(const QString& url) const override;

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    Tagger::AlbumMetadata parseWikipediaPage(const QByteArray& html, const QString& sourceUrl);
    QList<Tagger::TrackMetadata> parseSoundtrackTable(const QString& tableHtml);
    QString extractSoundtrackSection(const QString& html);
    QString extractPageTitle(const QString& html);
    QString cleanWikiText(const QString& text);
    int parseDuration(const QString& durationStr);

    struct TableColumns
    {
        int numberIndex{-1};
        int songIndex{-1};
        int singerIndex{-1};
        int lyricsIndex{-1};
        int musicIndex{-1};
        int durationIndex{-1};
    };
    TableColumns detectColumns(const QStringList& headers);

    QNetworkReply* m_currentReply{nullptr};
    QString m_pendingUrl;
};
