**NOTE:** This project was completely written by AI. It doesn't prove anything except for the fact that it helps me in finding alternative for my workflow. If this helps, go ahead and use it as you like.

# fooyin_tagger - Music Metadata Tagging Extension

## Overview
fooyin_tagger is an extension for the fooyin music player that provides automatic metadata tagging capabilities using online music databases. It supports fetching and updating metadata for your music library, including track names, artists, album information, and more.

## Features

### Track Matching Override
- Manual track matching override modal with two-column view
- Multi-selection support for batch matching operations
- Drag-and-drop reordering of tracks within each table
- Move Up/Down buttons for precise track reordering
- Auto Match feature to automatically reorder tracks based on similarity
- Visual highlighting of matched tracks
- Confidence score display with color coding
- Detailed metadata tooltips for all tracks

### Metadata Sources
1. **MusicBrainz** (https://musicbrainz.org)
   - Supports release URLs (e.g., https://musicbrainz.org/release/e6f19063-4772-485a-b77c-2ee35d80ddc0)
   - Supports release group URLs (e.g., https://musicbrainz.org/release-group/abc123...)
   - Allows searching by artist and album name
   - Provides comprehensive metadata including track listings, durations, and artist information

2. **Wikipedia** (https://wikipedia.org)
   - Supports URLs to soundtrack or music pages (e.g., https://en.wikipedia.org/wiki/Annakili_(soundtrack))
   - Extracts track listings from tables in soundtrack sections
   - Also supports "Track listing" sections for albums and soundtracks
   - Provides track information including singers, lyricists, and music directors

## Installation

1. Download or build the fooyin_tagger plugin
2. Copy the plugin to fooyin's plugins directory
3. Enable the plugin in fooyin's settings

## Usage

### Using with MusicBrainz

1. **Search by Artist/Album**: Enter the artist and album name to search for matches
2. **Direct ID/URL Input**: Choose the "Direct Release/Group ID" option to:
   - Enter a raw MBID (UUID format)
   - Paste a MusicBrainz release or release group URL
3. **Select Best Match**: From search results, select the most accurate metadata entry
4. **Apply Changes**: Review and apply the metadata to your music files

### Using with Wikipedia

1. **Find Wikipedia Page**: Locate the Wikipedia page for your soundtrack or album
2. **Copy URL**: Copy the Wikipedia URL (including #Track_listing anchor if needed)
3. **Paste URL**: Paste the URL into the tagger input field
4. **Extract Metadata**: The tagger will automatically extract track information from the page
5. **Apply Changes**: Review and apply the metadata to your music files

### Track Matching Override

The tagger provides a track matching override modal that allows you to manually adjust how tracks are matched between the source metadata and your local files.

1. **Access the Modal**: After fetching metadata, click the "Override Matches" button to open the track matching dialog
2. **Two-Column View**: The dialog shows source tracks (Wiki/MusicBrainz) on the left and your tracks on the right
3. **Multi-Selection**: Select multiple tracks on both sides using Ctrl+Click or Shift+Click
4. **Match Selected**: Click "Match Selected" to pair the selected tracks in order (first selected source with first selected destination, etc.)
5. **Unmatch Selected**: Click "Unmatch Selected" to remove matches for all selected tracks
6. **Move Up/Down**: Use "Move Up" and "Move Down" buttons to reorder individual tracks (single selection only)
7. **Drag and Drop**: Drag tracks to reorder them within their respective tables
8. **Auto Match**: Click "Auto Match" to automatically reorder your tracks to match source tracks based on title and duration similarity
9. **Reset Order**: Click "Reset Order" to restore your tracks to their original order
10. **Apply Matches**: Click "Apply Matches" to save your manual matching and return to the main tagger interface

**Note**: The matching is based on row positions - row 1 on the left matches row 1 on the right, etc. Extra tracks on either side remain at the bottom and are not matched.

## Supported URL Formats

### MusicBrainz
- Release URLs: `https://musicbrainz.org/release/[mbid]`
- Release Group URLs: `https://musicbrainz.org/release-group/[mbid]`
- Raw MBIDs: `[mbid]` (must be valid UUID format)

### Wikipedia
- Any valid Wikipedia URL with a soundtrack or track listing section
- Examples:
  - `https://en.wikipedia.org/wiki/Annakili_(soundtrack)`
  - `https://en.wikipedia.org/wiki/Annakili_(soundtrack)#Track_listing`

## Metadata Fields Supported

### Track Level
- Track title
- Track number
- Total tracks
- Disc number
- Total discs
- Artist(s)
- Duration
- MusicBrainz Recording ID

### Album Level
- Album name
- Album artist
- Release year
- Country
- Music director (from Wikipedia)

### Additional Fields (Wikipedia only)
- Lyricist
- Composer

## Development

### Building
The plugin uses CMake for building. See the CMakeLists.txt for details.

### Project Structure
```
fooyin_tagger/
├── include/            - Header files
├── src/
│   ├── core/          - Core functionality
│   ├── models/        - Data models
│   ├── settings/      - Settings UI
│   ├── sources/       - Metadata source implementations
│   └── ui/            - User interface
│       ├── taggerwidget.cpp/h      - Main tagger widget
│       └── trackmatchdialog.cpp/h  - Track matching override modal
├── build/             - Build directory (generated)
├── CMakeLists.txt     - Build configuration
└── metadata.json      - Plugin metadata
```

## Contributing

Contributions are welcome! Feel free to:
1. Submit bug reports
2. Suggest new features
3. Improve existing code
4. Add support for new metadata sources

## License

See the LICENSE file for more information.

## Acknowledgments

This plugin uses:
- **MusicBrainz API**: For music metadata
- **Wikipedia**: For soundtrack and album information
- **Qt**: For UI and networking
