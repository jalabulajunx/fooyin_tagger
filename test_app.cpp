#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include "src/sources/wikipediasource.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "=== Wikipedia Parser Test Application ===";

    // Read the downloaded page
    QFile file("/home/radnus/Projects/fooyin/fooyin_tagger/annakili.html");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Could not open 'annakili.html'";
        return -1;
    }

    QByteArray html = file.readAll();
    file.close();

    qDebug() << "\nSuccessfully read file:" << html.size() << "bytes";

    // Create parser instance
    WikipediaSource* parser = new WikipediaSource(nullptr, &app);

    qDebug() << "\n=== Parsing Wikipedia Page ===";
    Tagger::AlbumMetadata metadata = parser->parseWikipediaPage(html, "https://en.wikipedia.org/wiki/Annakili_(soundtrack)#Track_listing");

    qDebug() << "\n=== Parsing Results ===";
    qDebug() << "Album Name:" << metadata.album;
    qDebug() << "Source URL:" << metadata.sourceUrl;
    qDebug() << "Track Count:" << metadata.tracks.size();

    if(!metadata.tracks.isEmpty()) {
        qDebug() << "\n=== Track Listing ===";
        for(int i = 0; i < metadata.tracks.size(); ++i) {
            const auto& track = metadata.tracks[i];
            qDebug() << QString("%1. %2 - %3 (%4s)")
                        .arg(track.trackNumber)
                        .arg(track.title)
                        .arg(track.artist)
                        .arg(track.durationSeconds);
        }
    } else {
        qDebug() << "\nWarning: No tracks found!";
    }

    delete parser;
    return 0;
}
