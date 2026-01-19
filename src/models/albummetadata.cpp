#include "albummetadata.h"

// Static registration of metatypes
static const bool registered = []() {
    qRegisterMetaType<Tagger::TrackMetadata>("Tagger::TrackMetadata");
    qRegisterMetaType<Tagger::AlbumMetadata>("Tagger::AlbumMetadata");
    qRegisterMetaType<Tagger::SourceType>("Tagger::SourceType");
    qRegisterMetaType<Tagger::FetchStatus>("Tagger::FetchStatus");
    return true;
}();
