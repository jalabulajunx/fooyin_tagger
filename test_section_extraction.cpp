#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>

QString extractSoundtrackSection(const QString& html)
{
    // Look for Soundtrack or Music section
    // Wikipedia uses <span class="mw-headline" id="Soundtrack">Soundtrack</span>
    static QRegularExpression sectionRe(
        R"RE(<span[^>]*id="(?:Soundtrack|Music|Track_listing)"[^>]*>.*?</span>)RE",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );

    QRegularExpressionMatch sectionMatch = sectionRe.match(html);
    if(!sectionMatch.hasMatch()) {
        qDebug() << "No Soundtrack/Music/Track_listing section found";
        return QString();
    }

    int sectionStart = sectionMatch.capturedEnd();

    // Find the next section (next h2 or h3)
    static QRegularExpression nextSectionRe(R"(<h[23][^>]*>)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch nextMatch = nextSectionRe.match(html, sectionStart);

    int sectionEnd = nextMatch.hasMatch() ? nextMatch.capturedStart() : html.length();

    qDebug() << "Section found from" << sectionStart << "to" << sectionEnd;
    QString section = html.mid(sectionStart, sectionEnd - sectionStart);
    qDebug() << "Section snippet:" << section.left(200);

    return section;
}

bool findTableInSection(const QString& sectionHtml)
{
    // Find table element - wikitable or tracklist
    static QRegularExpression tableRe(
        R"(<table[^>]*class="[^"]*(?:wikitable|tracklist)[^"]*"[^>]*>(.*?)</table>)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );

    QRegularExpressionMatch tableMatch = tableRe.match(sectionHtml);
    if(!tableMatch.hasMatch()) {
        qDebug() << "No wikitable/tracklist found in section";
        return false;
    } else {
        qDebug() << "Found table in section";
        qDebug() << "Table snippet:" << tableMatch.captured(0).left(200);
        return true;
    }
}

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

    qDebug() << "\n=== Extracting Track Listing Section ===";
    QString trackSection = extractSoundtrackSection(QString::fromUtf8(html));

    qDebug() << "\n=== Searching for Tracklist Table ===";
    bool foundTable = findTableInSection(trackSection);

    qDebug() << "\n=== Complete ===";
    qDebug() << "Table found:" << foundTable;

    return 0;
}
