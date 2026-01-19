#include <QCoreApplication>
#include <QString>
#include <QRegularExpression>
#include <QDebug>

// Copy of WikipediaSource's parsing methods for testing
class TestWikipediaParser
{
public:
    static QString extractPageTitle(const QString& html)
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

    static QString extractSoundtrackSection(const QString& html)
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

        return html.mid(sectionStart, sectionEnd - sectionStart);
    }

    static QString cleanWikiText(const QString& text)
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
        cleaned.replace("&", "&");
        cleaned.replace("<", "<");
        cleaned.replace(">", ">");
        cleaned.replace(""", "\"");
        cleaned.replace("'", "'");
        cleaned.replace("&nbsp;", " ");

        // Remove quotes around titles
        cleaned.remove(QChar('"'));
        cleaned.remove(QChar(0x201C)); // "
        cleaned.remove(QChar(0x201D)); // "

        // Normalize whitespace
        cleaned = cleaned.simplified();

        return cleaned;
    }

    static bool findTableInSection(const QString& sectionHtml)
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
            return true;
        }
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "=== Testing Wikipedia Parser ===";

    // Test 1: Simple tracklist
    QString testTable = R"(<table class="tracklist"><tbody><tr><th class="tracklist-number-header" scope="col"><abbr title="Number">No.</abbr></th><th scope="col" style="width:60%">Title</th><th scope="col" style="width:40%">Singer(s)</th><th class="tracklist-length-header" scope="col">Length</th></tr><tr><th id="track1" scope="row">1.</th><td>"Adi Raakayi"</td><td><a href="/wiki/S._Janaki" title="S. Janaki">S. Janaki</a></td><td class="tracklist-length">4:11</td></tr></tbody></table>)";

    qDebug() << "\n1. Testing table detection:";
    if(TestWikipediaParser::findTableInSection(testTable)) {
        qDebug() << "✓ Table with tracklist class detected";
    } else {
        qDebug() << "✗ Table with tracklist class NOT detected";
    }

    // Test 2: Page with Track Listing section
    QString testPageHtml = R"(
        <!DOCTYPE html>
        <html>
        <head><title>Annakili (soundtrack) - Wikipedia</title></head>
        <body>
            <h2><span class="mw-headline" id="Track_listing">Track listing</span></h2>
            <p>...</p>
            <table class="tracklist">
                ...
            </table>
            <h2><span class="mw-headline" id="Reception">Reception</span></h2>
            <p>...</p>
        </body>
        </html>
    )";

    qDebug() << "\n2. Testing section extraction:";
    QString section = TestWikipediaParser::extractSoundtrackSection(testPageHtml);
    if(!section.isEmpty()) {
        qDebug() << "✓ Track listing section extracted successfully";
        qDebug() << "  Section length:" << section.length();
    } else {
        qDebug() << "✗ Section extraction failed";
    }

    qDebug() << "\n=== Parsing Complete ===";

    return 0;
}
