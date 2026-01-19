#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include "src/sources/wikipediasource.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "=== Testing Wikipedia Parser ===";

    // Read the downloaded page
    QFile file("/home/radnus/Projects/fooyin/fooyin_tagger/annakili.html");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open annakili.html";
        return -1;
    }

    QByteArray html = file.readAll();
    file.close();

    qDebug() << "\nFile size:" << html.size() << "bytes";

    WikipediaSource* parser = new WikipediaSource(nullptr, &app);

    qDebug() << "\nParsing page...";
    Tagger::AlbumMetadata metadata = parser->parseWikipediaPage(html, "https://en.wikipedia.org/wiki/Annakili_(soundtrack)");

    qDebug() << "\n=== Results ===";
    qDebug() << "Album name:" << metadata.album;
    qDebug() << "Track count:" << metadata.tracks.size();

    for(int i = 0; i < metadata.tracks.size(); ++i) {
        const auto& track = metadata.tracks[i];
        qDebug() << QString("%1. %2 - %3").arg(i+1).arg(track.title).arg(track.artist);
    }

    delete parser;
    return 0;
}
