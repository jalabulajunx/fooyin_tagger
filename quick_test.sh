#!/bin/bash

cd /home/radnus/Projects/fooyin/fooyin_tagger

# Create a temporary Qt project file
cat > temp_test.pro << 'EOF'
QT       += core network widgets
QT       -= gui

TARGET = quick_test
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += quick_test.cpp \
    src/sources/wikipediasource.cpp \
    src/core/httpclient.cpp

HEADERS += \
    src/sources/wikipediasource.h \
    src/core/httpclient.h \
    include/tagger/tagger_common.h

INCLUDEPATH += include
INCLUDEPATH += src
EOF

# Create the test program
cat > quick_test.cpp << 'EOF'
#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include "src/sources/wikipediasource.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "=== Wikipedia Parser Test ===";

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
EOF

# Try to build and run
if which qmake > /dev/null; then
    qmake temp_test.pro
    make -j$(nproc)
    if [ $? -eq 0 ]; then
        echo "=== Running Test ==="
        ./quick_test
    else
        echo "Error: Failed to compile"
    fi
else
    echo "Error: qmake not found"
fi

# Cleanup
rm -f temp_test.pro
rm -f quick_test.cpp
rm -f Makefile
rm -f quick_test.o
rm -f wikipediasource.o
rm -f httpclient.o
rm -f moc_*.cpp
rm -f quick_test
