#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include "src/sources/wikipediasource.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "=== Simple Tracklist Detection Test ===";

    // Create Wikipedia source instance
    WikipediaSource source(nullptr, &app);

    // Read the annakili.html file
    QFile file("/home/radnus/Projects/fooyin/fooyin_tagger/annakili.html");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Could not open file";
        return 1;
    }

    QByteArray html = file.readAll();
    file.close();

    QString htmlStr = QString::fromUtf8(html);

    // Step 1: Check for Track_listing section
    qDebug() << "\n1. Checking for Track_listing section...";
    static QRegularExpression sectionRe(
        R"RE(<span[^>]*class="[^"]*mw-headline[^"]*"[^>]*id="(?:Soundtrack|Music|Track_listing)"[^>]*>.*?</span>)RE",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );

    QRegularExpressionMatch sectionMatch = sectionRe.match(htmlStr);
    if(!sectionMatch.hasMatch()) {
        qDebug() << "ERROR: No Track_listing section found";

        // Search for any section with "Track" or "Listing" in id
        qDebug() << "\nLooking for track-related section ids...";
        static QRegularExpression idRe(R"(id="[^"]*track[^"]*")", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator idIt = idRe.globalMatch(htmlStr);
        while(idIt.hasNext()) {
            qDebug() << "Found id:" << idIt.next().captured(0);
        }

        return 2;
    }

    qDebug() << "SUCCESS: Track_listing section found";
    int sectionStart = sectionMatch.capturedEnd();

    // Step 2: Find end of section
    qDebug() << "\n2. Finding section boundaries...";
    static QRegularExpression nextSectionRe(R"(<h[23][^>]*>)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch nextMatch = nextSectionRe.match(htmlStr, sectionStart);
    int sectionEnd = nextMatch.hasMatch() ? nextMatch.capturedStart() : htmlStr.length();
    qDebug() << "Section from" << sectionStart << "to" << sectionEnd;

    // Step 3: Check if tracklist exists
    qDebug() << "\n3. Looking for tracklist table...";
    QString sectionHtml = htmlStr.mid(sectionStart, sectionEnd - sectionStart);
    static QRegularExpression tracklistRe(
        R"(<table[^>]*class="[^"]*tracklist[^"]*"[^>]*>(.*?)</table>)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );

    QRegularExpressionMatch tracklistMatch = tracklistRe.match(sectionHtml);
    if(!tracklistMatch.hasMatch()) {
        qDebug() << "ERROR: No tracklist table found in section";

        qDebug() << "\nChecking section contents (first 300 chars):";
        qDebug() << sectionHtml.left(300);

        return 3;
    }

    qDebug() << "SUCCESS: Tracklist table found";
    QString tracklistHtml = tracklistMatch.captured(1);

    // Step 4: Parse table
    qDebug() << "\n4. Parsing tracklist...";
    QList<Tagger::TrackMetadata> tracks;

    static QRegularExpression rowRe(R"(<tr[^>]*>(.*?)</tr>)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator rowIt = rowRe.globalMatch(tracklistHtml);

    // Skip header row
    if(rowIt.hasNext()) {
        rowIt.next();
    }

    while(rowIt.hasNext()) {
        QString rowHtml = rowIt.next().captured(1);

        if(rowHtml.contains("tracklist-total-length")) {
            continue;
        }

        static QRegularExpression cellRe(R"(<t[hd][^>]*>(.*?)</t[hd]>)",
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatchIterator cellIt = cellRe.globalMatch(rowHtml);

        QStringList cells;
        while(cellIt.hasNext()) {
            QString cell = cellIt.next().captured(1);
            // Clean cell
            static QRegularExpression tagRe(R"(<[^>]+>)");
            cell.remove(tagRe);
            cell.remove(QRegularExpression(R"(\[\d+\])"));
            cell.remove(QRegularExpression(R"(\[edit\])"));
            cell = cell.simplified();
            cells.append(cell);
        }

        if(cells.size() >= 3) {
            Tagger::TrackMetadata track;
            track.trackNumber = tracks.size() + 1;
            track.title = cells[1];
            track.artist = cells[2];
            if(cells.size() > 3) {
                track.durationSeconds = source.parseDuration(cells[3]);
            }
            tracks.append(track);
        }
    }

    qDebug() << "SUCCESS: Parsed" << tracks.size() << "tracks";

    // Print tracks
    qDebug() << "\n=== Extracted Tracks ===";
    for(int i = 0; i < tracks.size(); ++i) {
        const auto& track = tracks[i];
        qDebug() << QString("%1. %2 - %3 (%4s)")
                    .arg(track.trackNumber)
                    .arg(track.title)
                    .arg(track.artist)
                    .arg(track.durationSeconds);
    }

    return 0;
}
