#pragma once

#include <tagger/tagger_common.h>
#include <QObject>

class HttpClient;

class MetadataSource : public QObject
{
    Q_OBJECT

public:
    explicit MetadataSource(HttpClient* client, QObject* parent = nullptr);
    ~MetadataSource() override = default;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual Tagger::SourceType type() const = 0;
    [[nodiscard]] virtual bool supportsUrlInput() const = 0;
    [[nodiscard]] virtual bool supportsSearch() const = 0;

    // Async operations
    virtual void fetchFromUrl(const QString& url) = 0;
    virtual void searchAlbum(const QString& artist, const QString& album) = 0;
    virtual void cancel() = 0;

    [[nodiscard]] virtual bool isValidUrl(const QString& url) const { Q_UNUSED(url); return false; }

signals:
    void fetchStarted();
    void fetchProgress(int percent);
    void fetchCompleted(const Tagger::AlbumMetadata& metadata);
    void fetchFailed(const QString& error);
    void searchResults(const QList<Tagger::AlbumMetadata>& results);

protected:
    HttpClient* m_httpClient;
};
