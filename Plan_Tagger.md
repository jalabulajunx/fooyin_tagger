# Fooyin Tagger Plugin - Implementation Plan

## Overview

A metadata tagger plugin for fooyin that fetches album/track information from Wikipedia and MusicBrainz, with fuzzy matching to apply tags to selected tracks. Designed for Indian regional film soundtracks where Wikipedia is often the best source.

## Project Location

```
/home/radnus/Projects/fooyin/fooyin_tagger/
```

## MVP Scope

- **Sources**: Wikipedia (URL input) + MusicBrainz (search + URL)
- **Wikipedia format**: Extended columns - Song, Singer(s), Lyrics, Music Director, Duration
- **Matching**: Fuzzy title matching with Jaro-Winkler similarity
- **UI**: Dedicated tagger widget/dialog

---

## Project Structure

```
fooyin_tagger/
├── CMakeLists.txt
├── metadata.json
├── include/tagger/
│   └── tagger_common.h              # Enums, shared types
└── src/
    ├── taggerplugin.h/cpp           # Plugin entry point
    ├── core/
    │   ├── taggingmanager.h/cpp     # Central coordinator
    │   ├── httpclient.h/cpp         # Rate-limited HTTP client
    │   ├── matchingengine.h/cpp     # Fuzzy matching
    │   └── taggerlogging.h/cpp      # Q_LOGGING_CATEGORY
    ├── sources/
    │   ├── metadatasource.h/cpp     # Abstract interface
    │   ├── wikipediasource.h/cpp    # Wikipedia HTML parser
    │   └── musicbrainzsource.h/cpp  # MusicBrainz API client
    ├── models/
    │   ├── albummetadata.h/cpp      # Album/Track data structures
    │   └── matchresult.h/cpp        # Match with confidence score
    ├── ui/
    │   ├── taggerwidget.h/cpp       # Main dialog widget
    │   ├── matchpreviewtable.h/cpp  # Track matching table
    │   └── sourceselectionpanel.h/cpp
    └── settings/
        ├── taggersettings.h
        └── taggersettingspage.h/cpp
```

---

## Core Architecture

### Plugin Class (TaggerPlugin)
```cpp
class TaggerPlugin : public QObject,
                     public Fooyin::Plugin,
                     public Fooyin::CorePlugin,
                     public Fooyin::GuiPlugin
```

### Abstract Source Interface
```cpp
class MetadataSource : public QObject {
    virtual void fetchFromUrl(const QString& url) = 0;
    virtual void searchAlbum(const QString& artist, const QString& album) = 0;
signals:
    void fetchCompleted(const AlbumMetadata& metadata);
    void searchResults(const QList<AlbumMetadata>& results);
};
```

### Data Flow
```
Selected Tracks → TaggerWidget → TaggingManager
                                      ↓
                    WikipediaSource / MusicBrainzSource
                                      ↓
                              AlbumMetadata (parsed)
                                      ↓
                              MatchingEngine
                                      ↓
                    MatchResult[] (track ↔ metadata with confidence)
                                      ↓
                         Preview Table → User Confirms
                                      ↓
                         TaggingManager.applyTags() → TagLib
```

---

## Key Components

### 1. Wikipedia Parser
- Parse HTML tables from `/wiki/FilmName#Soundtrack` sections
- Detect columns: Song, Singer(s), Lyrics, Music Director, Duration
- Clean text: remove `[1]` citations, HTML entities, quotes
- Handle varying column orders via header detection

### 2. MusicBrainz Client
- API: `https://musicbrainz.org/ws/2/release?query=...&fmt=json`
- Rate limit: 1 request/second (enforced by HttpClient)
- Search by artist+album, or fetch by MBID URL
- Extract: title, artist, tracks, year, country

### 3. Fuzzy Matching Engine
- **Primary**: Jaro-Winkler similarity on normalized titles
- **Secondary**: Duration matching (±3 seconds tolerance)
- **Normalization**: Remove track numbers, parentheticals, quotes, lowercase
- **Threshold**: 0.6 minimum confidence (configurable)

### 4. UI Widget Layout
```
┌─────────────────────────────────────────────────────┐
│ [○ Wikipedia]  [○ MusicBrainz]                      │
├─────────────────────────────────────────────────────┤
│ URL: [_________________________] [Fetch]            │
│ -or-                                                │
│ Artist: [________]  Album: [________] [Search]      │
│ ┌─ Search Results ─────────────────────────────┐   │
│ │ • Varisu (2023) - Thaman S                   │   │
│ └───────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────┤
│ ┌─ Match Preview ──────────────────────────────┐   │
│ │ [✓] 01-track.flac → Ranjithame (95%)        │   │
│ │ [✓] 02-track.flac → Thee Thalapathy (87%)   │   │
│ │ [ ] 03-track.flac → (No match)              │   │
│ └───────────────────────────────────────────────┘   │
│ Fields: [✓]Title [✓]Artist [✓]Album [✓]Lyrics      │
├─────────────────────────────────────────────────────┤
│ Status: 3/4 tracks matched        [Apply] [Cancel] │
└─────────────────────────────────────────────────────┘
```

---

## Implementation Phases

### Phase 1: Project Scaffolding
- Create directory structure and CMakeLists.txt
- Implement TaggerPlugin with CorePlugin + GuiPlugin interfaces
- Create empty TaggerWidget, register with widgetProvider
- Add context menu action for "Tag with metadata..."

### Phase 2: Core Infrastructure
- Implement HttpClient with QNetworkAccessManager and rate limiting
- Create AlbumMetadata/TrackMetadata data structures
- Implement MetadataSource abstract interface
- Create TaggingManager skeleton

### Phase 3: Wikipedia Source
- Implement WikipediaSource with HTML parsing
- Build table parser with column detection
- Handle text cleanup (citations, entities)
- Test with Indian film Wikipedia pages (Pasumpon, Varisu, etc.)

### Phase 4: MusicBrainz Source
- Implement MusicBrainzSource with JSON API
- Build search query construction
- Parse release details and track listings
- Handle direct MBID URL input

### Phase 5: Matching Engine
- Implement Jaro-Winkler similarity
- Build title normalization
- Add duration-based secondary matching
- Create MatchResult with confidence scores

### Phase 6: UI Implementation
- Build complete TaggerWidget layout
- Implement source selection panels
- Create match preview table with checkboxes
- Add field selection and status display

### Phase 7: Tag Writing
- Integrate TagLib for writing tags
- Handle multiple formats (ID3v2, Vorbis, MP4)
- Write title, artist, album, lyrics fields
- Add error handling and progress feedback

### Phase 8: Settings & Polish
- Create settings page (confidence threshold, default source)
- Save/restore window geometry
- Add keyboard shortcuts
- Testing and bug fixes

---

## Critical Reference Files

| Purpose | Path |
|---------|------|
| Plugin pattern | `/home/radnus/Projects/fooyin/fooyin_conversion/fooyin-converter/src/converterplugin.cpp` |
| Widget pattern | `/home/radnus/Projects/fooyin/fooyin_conversion/fooyin-converter/src/converterwidget.cpp` |
| CMake template | `/home/radnus/Projects/fooyin/fooyin_conversion/fooyin-converter/CMakeLists.txt` |
| Multi-interface | `/home/radnus/Projects/fooyin/fooyin_chromecast/src/chromecastplugin.h` |
| Settings enum | `/home/radnus/Projects/fooyin/fooyin_conversion/fooyin-converter/src/convertersettings.h` |
| HTTP patterns | `/home/radnus/Projects/fooyin/fooyin_chromecast/src/core/httpserver.cpp` |

---

## Dependencies

- Qt6: Core, Widgets, Network
- Fooyin: Core, Gui, Utils
- TagLib (for tag writing)

---

## Verification Plan

1. **Build**: `cmake .. && make` - plugin compiles without errors
2. **Load**: Plugin appears in fooyin's plugin list
3. **UI**: Right-click tracks → "Tag with metadata..." opens dialog
4. **Wikipedia fetch**: Paste Tamil film URL → table parsed correctly
5. **MusicBrainz search**: Search "Thaman S" + "Varisu" → results appear
6. **Matching**: Select 4 tracks, fetch metadata → 3+ matches with >60% confidence
7. **Tag write**: Apply tags → verify with external tool (Kid3, mp3tag)
