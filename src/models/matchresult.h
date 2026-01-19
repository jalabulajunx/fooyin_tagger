#pragma once

#include <tagger/tagger_common.h>
#include <QString>

namespace Tagger {

struct MatchResult
{
    int trackIndex{-1};           // Index in selected tracks list
    int metadataIndex{-1};        // Index in fetched metadata tracks
    double confidence{0.0};       // 0.0 to 1.0
    QString matchReason;          // "Title match", "Duration match", etc.
    bool selected{true};          // Whether user wants to apply this match

    TrackMetadata sourceMetadata; // The fetched metadata
    QString targetFilepath;       // The file to apply tags to
    QString targetTitle;          // Current title of the target file

    [[nodiscard]] bool isValid() const { return trackIndex >= 0 && metadataIndex >= 0; }
    [[nodiscard]] bool isHighConfidence() const { return confidence >= 0.8; }
    [[nodiscard]] bool isMediumConfidence() const { return confidence >= 0.6 && confidence < 0.8; }
    [[nodiscard]] bool isLowConfidence() const { return confidence > 0.0 && confidence < 0.6; }
};

} // namespace Tagger

Q_DECLARE_METATYPE(Tagger::MatchResult)
