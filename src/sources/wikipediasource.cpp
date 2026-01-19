#include "wikipediasource.h"
#include "core/httpclient.h"

#include <QNetworkReply>
#include <QRegularExpression>
#include <QUrl>
#include <QDebug>

WikipediaSource::WikipediaSource(HttpClient* client, QObject* parent)
    : MetadataSource(client, parent)
{
    connect(m_httpClient, &HttpClient::requestCompleted, this, &WikipediaSource::onNetworkReply);
}

bool WikipediaSource::isValidUrl(const QString& url) const
{
    QUrl parsed(url);
    if(!parsed.isValid()) {
        return false;
    }

    QString host = parsed.host().toLower();
    return host.endsWith("wikipedia.org") && parsed.path().startsWith("/wiki/");
}

void WikipediaSource::fetchFromUrl(const QString& url)
{
    if(!isValidUrl(url)) {
        emit fetchFailed(tr("Invalid Wikipedia URL"));
        return;
    }

    cancel();

    m_pendingUrl = url;
    emit fetchStarted();

    m_currentReply = m_httpClient->get(QUrl(url));
}

void WikipediaSource::searchAlbum(const QString& artist, const QString& album)
{
    Q_UNUSED(artist)
    Q_UNUSED(album)
    emit fetchFailed(tr("Wikipedia does not support search. Please provide a URL."));
}

void WikipediaSource::cancel()
{
    if(m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    m_pendingUrl.clear();
}

void WikipediaSource::onNetworkReply(QNetworkReply* reply)
{
    if(reply != m_currentReply) {
        return;
    }

    m_currentReply = nullptr;

    if(reply->error() != QNetworkReply::NoError) {
        emit fetchFailed(tr("Network error: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray html = reply->readAll();
    reply->deleteLater();

    if(html.isEmpty()) {
        emit fetchFailed(tr("Empty response from Wikipedia"));
        return;
    }

    emit fetchProgress(50);

    Tagger::AlbumMetadata metadata = parseWikipediaPage(html, m_pendingUrl);

    if(metadata.tracks.isEmpty()) {
        emit fetchFailed(tr("No soundtrack table found on the page"));
        return;
    }

    emit fetchProgress(100);
    emit fetchCompleted(metadata);
}

Tagger::AlbumMetadata WikipediaSource::parseWikipediaPage(const QByteArray& html, const QString& sourceUrl)
{
    Tagger::AlbumMetadata metadata;
    metadata.source = Tagger::SourceType::Wikipedia;
    metadata.sourceUrl = sourceUrl;

    QString htmlStr = QString::fromUtf8(html);

    // Extract page title as album name
    metadata.album = extractPageTitle(htmlStr);

    // Find and parse soundtrack section
    QString soundtrackSection = extractSoundtrackSection(htmlStr);
    if(!soundtrackSection.isEmpty()) {
        metadata.tracks = parseSoundtrackTable(soundtrackSection);

        // Try to extract album artist from tracks
        if(!metadata.tracks.isEmpty() && metadata.albumArtist.isEmpty()) {
            // Use music director if available
            for(const auto& track : metadata.tracks) {
                if(!track.musicDirector.isEmpty()) {
                    metadata.musicDirector = track.musicDirector;
                    break;
                }
            }
        }
    }

    // Set album name on all tracks
    for(auto& track : metadata.tracks) {
        track.album = metadata.album;
    }

    qDebug() << "Parsed Wikipedia page:" << metadata.album
             << "with" << metadata.tracks.size() << "tracks";

    return metadata;
}

QString WikipediaSource::extractPageTitle(const QString& html)
{
    // Look for <title> tag
    static QRegularExpression titleRe(R"(<title>([^<]+)</title>)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = titleRe.match(html);
    if(match.hasMatch()) {
        QString title = match.captured(1);
        // Remove " - Wikipedia" suffix
        title.remove(QRegularExpression(R"(\s*[-–]\s*Wikipedia.*$)", QRegularExpression::CaseInsensitiveOption));
        // Remove "(film)" suffix
        title.remove(QRegularExpression(R"(\s*\(film\)\s*$)", QRegularExpression::CaseInsensitiveOption));
        return cleanWikiText(title.trimmed());
    }
    return QString();
}

QString WikipediaSource::extractSoundtrackSection(const QString& html)
{
    // Look for Soundtrack, Music, or Track listing section
    // Look for any h2/h3 tag with id containing:
    // - Soundtrack
    // - Music
    // - Track (with possible variations like Track_listing)
    static QRegularExpression sectionRe(
        R"RE(<h[23][^>]*id="(?:Soundtrack|Music|Track[^"]*)"[^>]*>.*?</h[23]>)RE",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );

    QRegularExpressionMatch sectionMatch = sectionRe.match(html);
    if(!sectionMatch.hasMatch()) {
        qDebug() << "No Soundtrack/Music/Track listing section found";
        return QString();
    }

    int sectionStart = sectionMatch.capturedEnd();

    // Find the next section (next h2 or h3)
    static QRegularExpression nextSectionRe(R"(<h[23][^>]*>)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch nextMatch = nextSectionRe.match(html, sectionStart);

    int sectionEnd = nextMatch.hasMatch() ? nextMatch.capturedStart() : html.length();

    qDebug() << "Section found from" << sectionStart << "to" << sectionEnd << "(length:" << sectionEnd - sectionStart << "chars)";

    QString section = html.mid(sectionStart, sectionEnd - sectionStart);

    // Debug: Print section contents if needed
    // qDebug() << "Section content snippet:" << section.left(200);

    return section;
}

QList<Tagger::TrackMetadata> WikipediaSource::parseSoundtrackTable(const QString& tableHtml)
{
    QList<Tagger::TrackMetadata> tracks;

    // Find table element - wikitable or tracklist
    static QRegularExpression tableRe(
        R"(<table[^>]*class="[^"]*(?:wikitable|tracklist)[^"]*"[^>]*>(.*?)</table>)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );

    QRegularExpressionMatch tableMatch = tableRe.match(tableHtml);
    if(!tableMatch.hasMatch()) {
        qDebug() << "No wikitable/tracklist found in section";
        return tracks;
    }

    QString tableContent = tableMatch.captured(1);

    // Parse header row to detect columns
    static QRegularExpression headerRowRe(
        R"(<tr[^>]*>(.*?)</tr>)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );

    static QRegularExpression headerCellRe(
        R"(<t[hd][^>]*>(.*?)</t[hd]>)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );

    QRegularExpressionMatchIterator rowIt = headerRowRe.globalMatch(tableContent);

    TableColumns columns;
    bool foundHeader = false;

    while(rowIt.hasNext()) {
        QRegularExpressionMatch rowMatch = rowIt.next();
        QString rowContent = rowMatch.captured(1);

        // Check if this is a header row (contains <th> tags)
        if(rowContent.contains("<th", Qt::CaseInsensitive) && !foundHeader) {
            QStringList headers;
            QRegularExpressionMatchIterator cellIt = headerCellRe.globalMatch(rowContent);
            while(cellIt.hasNext()) {
                QRegularExpressionMatch cellMatch = cellIt.next();
                headers.append(cleanWikiText(cellMatch.captured(1)));
            }
            columns = detectColumns(headers);
            foundHeader = true;
            continue;
        }

        // Data row
        if(foundHeader && rowContent.contains("<td", Qt::CaseInsensitive)) {
            QStringList cells;
            QRegularExpressionMatchIterator cellIt = headerCellRe.globalMatch(rowContent);
            while(cellIt.hasNext()) {
                QRegularExpressionMatch cellMatch = cellIt.next();
                cells.append(cleanWikiText(cellMatch.captured(1)));
            }

            if(cells.isEmpty()) {
                continue;
            }

            Tagger::TrackMetadata track;
            track.trackNumber = tracks.size() + 1;

            if(columns.numberIndex >= 0 && columns.numberIndex < cells.size()) {
                bool ok;
                int num = cells[columns.numberIndex].toInt(&ok);
                if(ok) {
                    track.trackNumber = num;
                }
            }

            if(columns.songIndex >= 0 && columns.songIndex < cells.size()) {
                track.title = cells[columns.songIndex];
            }
            else if(!cells.isEmpty()) {
                // Fallback: assume first non-number column is title
                track.title = cells[0];
            }

            if(columns.singerIndex >= 0 && columns.singerIndex < cells.size()) {
                track.artist = cells[columns.singerIndex];
            }

            if(columns.lyricsIndex >= 0 && columns.lyricsIndex < cells.size()) {
                track.lyricist = cells[columns.lyricsIndex];
            }

            if(columns.musicIndex >= 0 && columns.musicIndex < cells.size()) {
                track.musicDirector = cells[columns.musicIndex];
                if(track.composer.isEmpty()) {
                    track.composer = track.musicDirector;
                }
            }

            if(columns.durationIndex >= 0 && columns.durationIndex < cells.size()) {
                track.durationSeconds = parseDuration(cells[columns.durationIndex]);
            }

            if(!track.title.isEmpty()) {
                tracks.append(track);
            }
        }
    }

    // Set total tracks
    for(auto& track : tracks) {
        track.totalTracks = tracks.size();
    }

    qDebug() << "Parsed" << tracks.size() << "tracks from table";
    return tracks;
}

WikipediaSource::TableColumns WikipediaSource::detectColumns(const QStringList& headers)
{
    TableColumns cols;

    for(int i = 0; i < headers.size(); ++i) {
        QString h = headers[i].toLower();

        if(h.contains("no") || h.contains("#") || h == "sr") {
            cols.numberIndex = i;
        }
        else if(h.contains("song") || h.contains("title") || h.contains("track")) {
            cols.songIndex = i;
        }
        else if(h.contains("singer") || h.contains("artist") || h.contains("vocals") || h.contains("performed")) {
            cols.singerIndex = i;
        }
        else if(h.contains("lyric") || h.contains("written")) {
            cols.lyricsIndex = i;
        }
        else if(h.contains("music") || h.contains("composer") || h.contains("composed")) {
            cols.musicIndex = i;
        }
        else if(h.contains("duration") || h.contains("length") || h.contains("time")) {
            cols.durationIndex = i;
        }
    }

    qDebug() << "Detected columns - Song:" << cols.songIndex
             << "Singer:" << cols.singerIndex
             << "Lyrics:" << cols.lyricsIndex
             << "Music:" << cols.musicIndex
             << "Duration:" << cols.durationIndex;

    return cols;
}

QString WikipediaSource::cleanWikiText(const QString& text)
{
    QString cleaned = text;

    // Remove HTML tags
    static QRegularExpression htmlTagRe(R"(<[^>]+>)");
    cleaned.remove(htmlTagRe);

    // Remove Wikipedia citation markers [1], [2], etc.
    static QRegularExpression citationRe(R"(\[\d+\])");
    cleaned.remove(citationRe);

    // Remove [edit] links
    static QRegularExpression editRe(R"(\[edit\])", QRegularExpression::CaseInsensitiveOption);
    cleaned.remove(editRe);

    // Decode HTML entities
    cleaned.replace("&amp;", "&");
    cleaned.replace("&lt;", "<");
    cleaned.replace("&gt;", ">");
    cleaned.replace("&quot;", "\"");
    cleaned.replace("&#39;", "'");
    cleaned.replace("&nbsp;", " ");

    // Remove quotes around titles
    cleaned.remove(QChar('"'));
    cleaned.remove(QChar(0x201C)); // "
    cleaned.remove(QChar(0x201D)); // "

    // Normalize whitespace
    cleaned = cleaned.simplified();

    return cleaned;
}

int WikipediaSource::parseDuration(const QString& durationStr)
{
    // Parse formats like "3:45", "3.45", "3m 45s", "225"
    QString str = durationStr.trimmed();

    // Format: M:SS or MM:SS
    static QRegularExpression colonRe(R"((\d+):(\d+))");
    QRegularExpressionMatch match = colonRe.match(str);
    if(match.hasMatch()) {
        int minutes = match.captured(1).toInt();
        int seconds = match.captured(2).toInt();
        return minutes * 60 + seconds;
    }

    // Format: M.SS
    static QRegularExpression dotRe(R"((\d+)\.(\d+))");
    match = dotRe.match(str);
    if(match.hasMatch()) {
        int minutes = match.captured(1).toInt();
        int seconds = match.captured(2).toInt();
        return minutes * 60 + seconds;
    }

    // Plain seconds
    bool ok;
    int seconds = str.toInt(&ok);
    if(ok) {
        return seconds;
    }

    return 0;
}
