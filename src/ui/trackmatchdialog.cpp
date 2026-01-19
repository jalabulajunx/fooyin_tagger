#include "trackmatchdialog.h"
#include "core/track.h"

#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QFileInfo>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <algorithm>

namespace Tagger {

// ============================================================================
// SourceTrackModel Implementation
// ============================================================================

SourceTrackModel::SourceTrackModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void SourceTrackModel::setTracks(const QList<TrackMetadata>& tracks, const QList<double>& confidences)
{
    beginResetModel();
    m_tracks = tracks;
    m_confidences = confidences;
    // Ensure confidences list matches tracks size
    while(m_confidences.size() < m_tracks.size()) {
        m_confidences.append(0.0);
    }
    while(m_confidences.size() > m_tracks.size()) {
        m_confidences.removeLast();
    }
    endResetModel();
}

Qt::ItemFlags SourceTrackModel::flags(const QModelIndex& index) const
{
    if(!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool SourceTrackModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                              const QModelIndex& destinationParent, int destinationChild)
{
    if(sourceParent != destinationParent || sourceRow < 0 || destinationChild < 0 ||
       sourceRow >= m_tracks.size() || destinationChild > m_tracks.size()) {
        return false;
    }

    if(sourceRow == destinationChild) {
        return true;
    }

    beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild);

    // Move tracks
    QList<TrackMetadata> movedTracks;
    QList<double> movedConfidences;

    for(int i = 0; i < count; ++i) {
        movedTracks.append(m_tracks.takeAt(sourceRow));
        movedConfidences.append(m_confidences.takeAt(sourceRow));
    }

    // Insert at destination
    int insertPos = destinationChild;
    if(destinationChild > sourceRow) {
        insertPos = destinationChild - count;
    }

    for(int i = 0; i < count; ++i) {
        m_tracks.insert(insertPos + i, movedTracks[i]);
        m_confidences.insert(insertPos + i, movedConfidences[i]);
    }

    endMoveRows();
    return true;
}

int SourceTrackModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_tracks.size();
}

int SourceTrackModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant SourceTrackModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() >= m_tracks.size()) {
        return QVariant();
    }

    const auto& track = m_tracks[index.row()];

    if(role == Qt::DisplayRole) {
        switch(index.column()) {
            case TrackNumber:
                if(track.trackNumber > 0) {
                    return QString::number(track.trackNumber);
                }
                return QString();
            case Title:
                return track.title;
            case Artist:
                return track.artist;
            case Duration:
                if(track.durationSeconds > 0) {
                    int minutes = track.durationSeconds / 60;
                    int seconds = track.durationSeconds % 60;
                    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
                }
                return QString();
            case Confidence:
                if(index.row() < m_confidences.size()) {
                    double conf = m_confidences[index.row()];
                    return QString("%1%").arg(static_cast<int>(conf * 100));
                }
                return QString();
        }
    }
    else if(role == Qt::TextAlignmentRole) {
        if(index.column() == TrackNumber || index.column() == Duration || index.column() == Confidence) {
            return Qt::AlignCenter;
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }
    else if(role == Qt::ForegroundRole) {
        if(index.column() == Confidence && index.row() < m_confidences.size()) {
            double conf = m_confidences[index.row()];
            if(conf >= 0.8) {
                return QColor(0, 150, 0); // Green
            }
            else if(conf >= 0.6) {
                return QColor(200, 150, 0); // Orange
            }
            else if(conf > 0.0) {
                return QColor(200, 0, 0); // Red
            }
        }
    }
    else if(role == Qt::ToolTipRole) {
        QString tooltip;
        tooltip += tr("Title: %1\n").arg(track.title);
        tooltip += tr("Artist: %1\n").arg(track.artist);
        if(!track.albumArtist.isEmpty()) {
            tooltip += tr("Album Artist: %1\n").arg(track.albumArtist);
        }
        if(track.trackNumber > 0) {
            tooltip += tr("Track: %1").arg(track.trackNumber);
        }
        if(track.durationSeconds > 0) {
            tooltip += tr("\nDuration: %1:%2").arg(track.durationSeconds / 60)
                .arg(track.durationSeconds % 60, 2, 10, QChar('0'));
        }
        if(index.column() == Confidence && index.row() < m_confidences.size()) {
            double conf = m_confidences[index.row()];
            tooltip += tr("\nConfidence: %1%").arg(static_cast<int>(conf * 100));
        }
        return tooltip;
    }

    return QVariant();
}

QVariant SourceTrackModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
            case TrackNumber: return tr("#");
            case Title: return tr("Title");
            case Artist: return tr("Artist");
            case Duration: return tr("Duration");
            case Confidence: return tr("Confidence");
        }
    }
    return QVariant();
}

// ============================================================================
// DestinationTrackModel Implementation
// ============================================================================

DestinationTrackModel::DestinationTrackModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void DestinationTrackModel::setTracks(const Fooyin::TrackList& tracks)
{
    beginResetModel();
    m_tracks = tracks;
    endResetModel();
}

Qt::ItemFlags DestinationTrackModel::flags(const QModelIndex& index) const
{
    if(!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool DestinationTrackModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                                   const QModelIndex& destinationParent, int destinationChild)
{
    if(sourceParent != destinationParent || sourceRow < 0 || destinationChild < 0 ||
       sourceRow >= m_tracks.size() || destinationChild > m_tracks.size()) {
        return false;
    }

    if(sourceRow == destinationChild) {
        return true;
    }

    beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild);

    // Move tracks
    Fooyin::TrackList movedTracks;

    for(int i = 0; i < count; ++i) {
        movedTracks.push_back(m_tracks[sourceRow]);
        m_tracks.erase(m_tracks.begin() + sourceRow);
    }

    // Insert at destination
    int insertPos = destinationChild;
    if(destinationChild > sourceRow) {
        insertPos = destinationChild - count;
    }

    for(int i = 0; i < count; ++i) {
        m_tracks.insert(m_tracks.begin() + insertPos + i, movedTracks[i]);
    }

    endMoveRows();
    return true;
}

int DestinationTrackModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_tracks.size();
}

int DestinationTrackModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant DestinationTrackModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() >= m_tracks.size()) {
        return QVariant();
    }

    const auto& track = m_tracks[index.row()];

    if(role == Qt::DisplayRole) {
        switch(index.column()) {
            case Filename: {
                QFileInfo fi(track.filepath());
                return fi.fileName();
            }
            case Title:
                return track.title();
            case Artist:
                return track.artist();
            case Duration: {
                int duration = track.duration() / 1000; // Convert ms to seconds
                if(duration > 0) {
                    int minutes = duration / 60;
                    int seconds = duration % 60;
                    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
                }
                return QString();
            }
        }
    }
    else if(role == Qt::TextAlignmentRole) {
        if(index.column() == Duration) {
            return Qt::AlignCenter;
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }
    else if(role == Qt::ToolTipRole) {
        QString tooltip;
        tooltip += tr("File: %1\n").arg(track.filepath());
        tooltip += tr("Title: %1\n").arg(track.title());
        tooltip += tr("Artist: %1\n").arg(track.artist());
        if(!track.albumArtist().isEmpty()) {
            tooltip += tr("Album Artist: %1\n").arg(track.albumArtist());
        }
        if(!track.trackNumber().isEmpty()) {
            tooltip += tr("Track: %1").arg(track.trackNumber());
        }
        int duration = track.duration() / 1000;
        if(duration > 0) {
            tooltip += tr("\nDuration: %1:%2").arg(duration / 60)
                .arg(duration % 60, 2, 10, QChar('0'));
        }
        return tooltip;
    }

    return QVariant();
}

QVariant DestinationTrackModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
            case Filename: return tr("Filename");
            case Title: return tr("Title");
            case Artist: return tr("Artist");
            case Duration: return tr("Duration");
        }
    }
    return QVariant();
}

// ============================================================================
// TrackMatchDelegate Implementation
// ============================================================================

TrackMatchDelegate::TrackMatchDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void TrackMatchDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);
}

// ============================================================================
// TrackMatchDialog Implementation
// ============================================================================

TrackMatchDialog::TrackMatchDialog(const AlbumMetadata& sourceMetadata,
                                   const Fooyin::TrackList& userTracks,
                                   const QList<MatchResult>& initialMatches,
                                   QWidget* parent)
    : QDialog(parent)
    , m_sourceMetadata(sourceMetadata)
    , m_userTracks(userTracks)
    , m_matches(initialMatches)
    , m_sourceModel(new SourceTrackModel(this))
    , m_destinationModel(new DestinationTrackModel(this))
{
    setWindowTitle(tr("Track Matching Override"));
    setMinimumSize(1000, 600);
    resize(1200, 700);

    setupUI();

    // Initialize models
    QList<double> confidences;
    for(const auto& match : m_matches) {
        if(match.isValid()) {
            while(confidences.size() <= match.metadataIndex) {
                confidences.append(0.0);
            }
            confidences[match.metadataIndex] = match.confidence;
        }
    }
    m_sourceModel->setTracks(m_sourceMetadata.tracks, confidences);
    m_destinationModel->setTracks(m_userTracks);

    updateMatchVisuals();
    updateMatchButtons();
}

void TrackMatchDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Header with album info
    auto* headerLayout = new QHBoxLayout();
    auto* headerLabel = new QLabel(this);
    QString headerText = tr("<b>Source:</b> %1 - %2").arg(m_sourceMetadata.albumArtist, m_sourceMetadata.album);
    if(m_sourceMetadata.year > 0) {
        headerText += QString(" (%1)").arg(m_sourceMetadata.year);
    }
    headerLabel->setText(headerText);
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Splitter for source and destination tables
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Source table (left)
    auto* sourceContainer = new QWidget(splitter);
    auto* sourceLayout = new QVBoxLayout(sourceContainer);
    sourceLayout->setContentsMargins(0, 0, 0, 0);

    auto* sourceLabel = new QLabel(tr("<b>Source Tracks (Wiki/MusicBrainz)</b>"), sourceContainer);
    sourceLayout->addWidget(sourceLabel);

    m_sourceTable = new QTableView(sourceContainer);
    m_sourceTable->setModel(m_sourceModel);
    m_sourceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sourceTable->setSelectionMode(QAbstractItemView::MultiSelection);
    m_sourceTable->setAlternatingRowColors(true);
    m_sourceTable->setDragEnabled(true);
    m_sourceTable->setAcceptDrops(true);
    m_sourceTable->setDropIndicatorShown(true);
    m_sourceTable->setDragDropMode(QAbstractItemView::InternalMove);
    m_sourceTable->setDragDropOverwriteMode(false);
    m_sourceTable->horizontalHeader()->setStretchLastSection(false);
    m_sourceTable->horizontalHeader()->setSectionResizeMode(SourceTrackModel::TrackNumber, QHeaderView::ResizeToContents);
    m_sourceTable->horizontalHeader()->setSectionResizeMode(SourceTrackModel::Title, QHeaderView::Stretch);
    m_sourceTable->horizontalHeader()->setSectionResizeMode(SourceTrackModel::Artist, QHeaderView::Stretch);
    m_sourceTable->horizontalHeader()->setSectionResizeMode(SourceTrackModel::Duration, QHeaderView::ResizeToContents);
    m_sourceTable->horizontalHeader()->setSectionResizeMode(SourceTrackModel::Confidence, QHeaderView::ResizeToContents);
    m_sourceTable->verticalHeader()->setVisible(false);
    sourceLayout->addWidget(m_sourceTable);

    // Source move buttons
    auto* sourceMoveLayout = new QHBoxLayout();
    m_moveSourceUpButton = new QPushButton(tr("Move Up"), this);
    m_moveSourceUpButton->setEnabled(false);
    m_moveSourceUpButton->setToolTip(tr("Move selected track up"));
    m_moveSourceDownButton = new QPushButton(tr("Move Down"), this);
    m_moveSourceDownButton->setEnabled(false);
    m_moveSourceDownButton->setToolTip(tr("Move selected track down"));
    sourceMoveLayout->addWidget(m_moveSourceUpButton);
    sourceMoveLayout->addWidget(m_moveSourceDownButton);
    sourceMoveLayout->addStretch();
    sourceLayout->addLayout(sourceMoveLayout);

    splitter->addWidget(sourceContainer);

    // Destination table (right)
    auto* destContainer = new QWidget(splitter);
    auto* destLayout = new QVBoxLayout(destContainer);
    destLayout->setContentsMargins(0, 0, 0, 0);

    auto* destLabel = new QLabel(tr("<b>Your Tracks</b>"), destContainer);
    destLayout->addWidget(destLabel);

    m_destinationTable = new QTableView(destContainer);
    m_destinationTable->setModel(m_destinationModel);
    m_destinationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_destinationTable->setSelectionMode(QAbstractItemView::MultiSelection);
    m_destinationTable->setAlternatingRowColors(true);
    m_destinationTable->setDragEnabled(true);
    m_destinationTable->setAcceptDrops(true);
    m_destinationTable->setDropIndicatorShown(true);
    m_destinationTable->setDragDropMode(QAbstractItemView::InternalMove);
    m_destinationTable->setDragDropOverwriteMode(false);
    m_destinationTable->horizontalHeader()->setStretchLastSection(false);
    m_destinationTable->horizontalHeader()->setSectionResizeMode(DestinationTrackModel::Filename, QHeaderView::Stretch);
    m_destinationTable->horizontalHeader()->setSectionResizeMode(DestinationTrackModel::Title, QHeaderView::Stretch);
    m_destinationTable->horizontalHeader()->setSectionResizeMode(DestinationTrackModel::Artist, QHeaderView::Stretch);
    m_destinationTable->horizontalHeader()->setSectionResizeMode(DestinationTrackModel::Duration, QHeaderView::ResizeToContents);
    m_destinationTable->verticalHeader()->setVisible(false);
    destLayout->addWidget(m_destinationTable);

    // Destination move buttons
    auto* destMoveLayout = new QHBoxLayout();
    m_moveDestUpButton = new QPushButton(tr("Move Up"), this);
    m_moveDestUpButton->setEnabled(false);
    m_moveDestUpButton->setToolTip(tr("Move selected track up"));
    m_moveDestDownButton = new QPushButton(tr("Move Down"), this);
    m_moveDestDownButton->setEnabled(false);
    m_moveDestDownButton->setToolTip(tr("Move selected track down"));
    destMoveLayout->addWidget(m_moveDestUpButton);
    destMoveLayout->addWidget(m_moveDestDownButton);
    destMoveLayout->addStretch();
    destLayout->addLayout(destMoveLayout);

    splitter->addWidget(destContainer);

    // Set splitter proportions
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter, 1);

    // Match controls
    auto* controlsLayout = new QHBoxLayout();

    m_matchButton = new QPushButton(tr("Match Selected"), this);
    m_matchButton->setEnabled(false);
    m_matchButton->setToolTip(tr("Use drag-and-drop to reorder tracks for position-based matching"));

    m_unmatchButton = new QPushButton(tr("Unmatch Selected"), this);
    m_unmatchButton->setEnabled(false);
    m_unmatchButton->setToolTip(tr("Use drag-and-drop to reorder tracks for position-based matching"));

    m_autoMatchButton = new QPushButton(tr("Auto Match"), this);
    m_autoMatchButton->setToolTip(tr("Automatically reorder destination tracks to match source tracks"));

    m_clearAllButton = new QPushButton(tr("Reset Order"), this);
    m_clearAllButton->setToolTip(tr("Reset destination tracks to original order"));

    controlsLayout->addWidget(m_matchButton);
    controlsLayout->addWidget(m_unmatchButton);
    controlsLayout->addWidget(m_autoMatchButton);
    controlsLayout->addWidget(m_clearAllButton);
    controlsLayout->addStretch();

    mainLayout->addLayout(controlsLayout);

    // Status label
    m_statusLabel = new QLabel(tr("Drag and drop tracks to reorder. Row positions determine matching."), this);
    mainLayout->addWidget(m_statusLabel);

    // Button box
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
    m_applyButton->setText(tr("Apply Matches"));
    m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);

    mainLayout->addWidget(buttonBox);

    // Connect signals
    connect(m_sourceTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &TrackMatchDialog::onSourceSelectionChanged);
    connect(m_destinationTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &TrackMatchDialog::onDestinationSelectionChanged);
    connect(m_matchButton, &QPushButton::clicked, this, &TrackMatchDialog::onMatchTracks);
    connect(m_unmatchButton, &QPushButton::clicked, this, &TrackMatchDialog::onUnmatchTracks);
    connect(m_autoMatchButton, &QPushButton::clicked, this, &TrackMatchDialog::onAutoMatch);
    connect(m_clearAllButton, &QPushButton::clicked, this, &TrackMatchDialog::onClearAllMatches);
    connect(m_applyButton, &QPushButton::clicked, this, &TrackMatchDialog::onApply);
    connect(m_cancelButton, &QPushButton::clicked, this, &TrackMatchDialog::onCancel);
    connect(m_moveSourceUpButton, &QPushButton::clicked, this, &TrackMatchDialog::onMoveSourceUp);
    connect(m_moveSourceDownButton, &QPushButton::clicked, this, &TrackMatchDialog::onMoveSourceDown);
    connect(m_moveDestUpButton, &QPushButton::clicked, this, &TrackMatchDialog::onMoveDestUp);
    connect(m_moveDestDownButton, &QPushButton::clicked, this, &TrackMatchDialog::onMoveDestDown);
}

void TrackMatchDialog::onSourceSelectionChanged()
{
    QModelIndexList selected = m_sourceTable->selectionModel()->selectedRows();
    bool hasSelection = !selected.isEmpty();

    // Move up/down buttons only work with single selection
    m_moveSourceUpButton->setEnabled(hasSelection && selected.size() == 1 && selected.first().row() > 0);
    m_moveSourceDownButton->setEnabled(hasSelection && selected.size() == 1 && selected.first().row() < m_sourceModel->trackCount() - 1);

    updateMatchButtons();
}

void TrackMatchDialog::onDestinationSelectionChanged()
{
    QModelIndexList selected = m_destinationTable->selectionModel()->selectedRows();
    bool hasSelection = !selected.isEmpty();

    // Move up/down buttons only work with single selection
    m_moveDestUpButton->setEnabled(hasSelection && selected.size() == 1 && selected.first().row() > 0);
    m_moveDestDownButton->setEnabled(hasSelection && selected.size() == 1 && selected.first().row() < m_destinationModel->trackCount() - 1);

    updateMatchButtons();
}

void TrackMatchDialog::onMatchTracks()
{
    // Multi-selection batch matching
    QModelIndexList sourceIndexes = m_sourceTable->selectionModel()->selectedRows();
    QModelIndexList destIndexes = m_destinationTable->selectionModel()->selectedRows();

    if(sourceIndexes.isEmpty() || destIndexes.isEmpty()) {
        m_statusLabel->setText(tr("Select tracks on both sides to match"));
        return;
    }

    // Sort selections by row index to ensure proper order
    std::sort(sourceIndexes.begin(), sourceIndexes.end(),
              [](const QModelIndex& a, const QModelIndex& b) {
                  return a.row() < b.row();
              });
    std::sort(destIndexes.begin(), destIndexes.end(),
              [](const QModelIndex& a, const QModelIndex& b) {
                  return a.row() < b.row();
              });

    // Match tracks in order
    int matchCount = qMin(sourceIndexes.size(), destIndexes.size());
    for(int i = 0; i < matchCount; ++i) {
        int sourceRow = sourceIndexes[i].row();
        int destRow = destIndexes[i].row();

        // Remove any existing match for this destination track
        for(auto it = m_matches.begin(); it != m_matches.end(); ) {
            if(it->trackIndex == destRow) {
                it = m_matches.erase(it);
            }
            else {
                ++it;
            }
        }

        // Remove any existing match for this source track
        for(auto it = m_matches.begin(); it != m_matches.end(); ) {
            if(it->metadataIndex == sourceRow) {
                it = m_matches.erase(it);
            }
            else {
                ++it;
            }
        }

        // Create new match
        MatchResult match;
        match.trackIndex = destRow;
        match.metadataIndex = sourceRow;
        match.confidence = 1.0; // Manual match gets full confidence
        match.matchReason = tr("Manual match");
        match.selected = true;
        match.sourceMetadata = m_sourceModel->tracks()[sourceRow];
        match.targetFilepath = m_destinationModel->tracks()[destRow].filepath();
        match.targetTitle = m_destinationModel->tracks()[destRow].title();

        m_matches.append(match);
    }

    updateMatchVisuals();
    updateMatchButtons();

    m_statusLabel->setText(tr("Matched %1 track(s)").arg(matchCount));
}

void TrackMatchDialog::onUnmatchTracks()
{
    // Multi-selection unmatching
    QModelIndexList sourceIndexes = m_sourceTable->selectionModel()->selectedRows();
    QModelIndexList destIndexes = m_destinationTable->selectionModel()->selectedRows();

    int removedCount = 0;

    // Remove matches for selected source tracks
    for(const QModelIndex& index : sourceIndexes) {
        int sourceRow = index.row();
        for(auto it = m_matches.begin(); it != m_matches.end(); ) {
            if(it->metadataIndex == sourceRow) {
                it = m_matches.erase(it);
                removedCount++;
            }
            else {
                ++it;
            }
        }
    }

    // Remove matches for selected destination tracks
    for(const QModelIndex& index : destIndexes) {
        int destRow = index.row();
        for(auto it = m_matches.begin(); it != m_matches.end(); ) {
            if(it->trackIndex == destRow) {
                it = m_matches.erase(it);
                removedCount++;
            }
            else {
                ++it;
            }
        }
    }

    if(removedCount > 0) {
        updateMatchVisuals();
        updateMatchButtons();
        m_statusLabel->setText(tr("Unmatched %1 track(s)").arg(removedCount));
    }
    else {
        m_statusLabel->setText(tr("No tracks selected to unmatch"));
    }
}

void TrackMatchDialog::onAutoMatch()
{
    const auto& sourceTracks = m_sourceModel->tracks();
    auto destTracks = m_destinationModel->tracks();

    // Simple auto-match based on title similarity and duration
    // Reorder destination tracks to match source tracks
    Fooyin::TrackList reorderedDestTracks;
    QList<bool> usedDestTracks;
    usedDestTracks.resize(destTracks.size(), false);

    for(int i = 0; i < sourceTracks.size(); ++i) {
        const auto& sourceTrack = sourceTracks[i];
        QString sourceTitle = sourceTrack.title.toLower().simplified();
        int sourceDuration = sourceTrack.durationSeconds;

        int bestMatch = -1;
        double bestScore = 0.0;

        for(int j = 0; j < destTracks.size(); ++j) {
            if(usedDestTracks[j]) {
                continue;
            }

            const auto& destTrack = destTracks[j];
            QString destTitle = destTrack.title().toLower().simplified();
            int destDuration = destTrack.duration() / 1000;

            // Calculate similarity score
            double score = 0.0;

            // Title similarity (exact match gets high score)
            if(destTitle == sourceTitle) {
                score += 0.8;
            }
            else if(destTitle.contains(sourceTitle) || sourceTitle.contains(destTitle)) {
                score += 0.5;
            }

            // Duration similarity
            if(destDuration > 0 && sourceDuration > 0) {
                int diff = qAbs(destDuration - sourceDuration);
                if(diff <= 2) {
                    score += 0.2;
                }
                else if(diff <= 5) {
                    score += 0.1;
                }
            }

            if(score > bestScore && score >= 0.5) {
                bestScore = score;
                bestMatch = j;
            }
        }

        if(bestMatch >= 0) {
            reorderedDestTracks.push_back(destTracks[bestMatch]);
            usedDestTracks[bestMatch] = true;
        }
        else {
            // No match found, add placeholder or skip
            // We'll add unmatched tracks at the end
        }
    }

    // Add remaining unmatched destination tracks
    for(int j = 0; j < destTracks.size(); ++j) {
        if(!usedDestTracks[j]) {
            reorderedDestTracks.push_back(destTracks[j]);
        }
    }

    // Update the destination model with reordered tracks
    m_destinationModel->setTracks(reorderedDestTracks);

    updateMatchVisuals();
    updateMatchButtons();

    int matchedCount = static_cast<int>(qMin(static_cast<int>(sourceTracks.size()), static_cast<int>(reorderedDestTracks.size())));
    m_statusLabel->setText(tr("Auto-matched and reordered %1/%2 tracks").arg(matchedCount).arg(destTracks.size()));
}

void TrackMatchDialog::onClearAllMatches()
{
    // Reset destination tracks to original order
    m_destinationModel->setTracks(m_userTracks);
    updateMatchVisuals();
    updateMatchButtons();
    m_statusLabel->setText(tr("Destination tracks reset to original order"));
}

void TrackMatchDialog::onApply()
{
    // Create position-based matches
    m_matches.clear();

    int maxRows = qMin(m_sourceModel->trackCount(), m_destinationModel->trackCount());

    for(int i = 0; i < maxRows; ++i) {
        MatchResult match;
        match.trackIndex = i;
        match.metadataIndex = i;
        match.confidence = 1.0; // Position-based match gets full confidence
        match.matchReason = tr("Position match");
        match.selected = true;
        match.sourceMetadata = m_sourceModel->tracks()[i];
        match.targetFilepath = m_destinationModel->tracks()[i].filepath();
        match.targetTitle = m_destinationModel->tracks()[i].title();

        m_matches.append(match);
    }

    accept();
}

void TrackMatchDialog::onCancel()
{
    reject();
}

void TrackMatchDialog::onMoveSourceUp()
{
    QModelIndexList selected = m_sourceTable->selectionModel()->selectedRows();
    if(selected.isEmpty()) {
        return;
    }

    int row = selected.first().row();
    if(row <= 0) {
        return; // Already at top
    }

    m_sourceModel->moveRows({}, row, 1, {}, row - 1);
    updateMatchVisuals();
}

void TrackMatchDialog::onMoveSourceDown()
{
    QModelIndexList selected = m_sourceTable->selectionModel()->selectedRows();
    if(selected.isEmpty()) {
        return;
    }

    int row = selected.first().row();
    if(row >= m_sourceModel->trackCount() - 1) {
        return; // Already at bottom
    }

    m_sourceModel->moveRows({}, row, 1, {}, row + 2);
    updateMatchVisuals();
}

void TrackMatchDialog::onMoveDestUp()
{
    QModelIndexList selected = m_destinationTable->selectionModel()->selectedRows();
    if(selected.isEmpty()) {
        return;
    }

    int row = selected.first().row();
    if(row <= 0) {
        return; // Already at top
    }

    m_destinationModel->moveRows({}, row, 1, {}, row - 1);
    updateMatchVisuals();
}

void TrackMatchDialog::onMoveDestDown()
{
    QModelIndexList selected = m_destinationTable->selectionModel()->selectedRows();
    if(selected.isEmpty()) {
        return;
    }

    int row = selected.first().row();
    if(row >= m_destinationModel->trackCount() - 1) {
        return; // Already at bottom
    }

    m_destinationModel->moveRows({}, row, 1, {}, row + 2);
    updateMatchVisuals();
}

void TrackMatchDialog::updateMatchVisuals()
{
    // Position-based matching: row i on left matches row i on right
    // Highlight matched rows in both tables
    int maxRows = qMax(m_sourceModel->trackCount(), m_destinationModel->trackCount());

    for(int i = 0; i < maxRows; ++i) {
        bool sourceExists = (i < m_sourceModel->trackCount());
        bool destExists = (i < m_destinationModel->trackCount());
        bool isMatched = sourceExists && destExists;

        // Highlight source row
        if(sourceExists) {
            for(int col = 0; col < m_sourceModel->columnCount(); ++col) {
                QModelIndex idx = m_sourceModel->index(i, col);
                QBrush bg = isMatched ? QColor(200, 230, 255) : Qt::white;
                m_sourceModel->setData(idx, bg, Qt::BackgroundRole);
            }
        }

        // Highlight destination row
        if(destExists) {
            for(int col = 0; col < m_destinationModel->columnCount(); ++col) {
                QModelIndex idx = m_destinationModel->index(i, col);
                QBrush bg = isMatched ? QColor(200, 230, 255) : Qt::white;
                m_destinationModel->setData(idx, bg, Qt::BackgroundRole);
            }
        }
    }
}

void TrackMatchDialog::updateMatchButtons()
{
    // Enable match button if both sides have selections
    bool hasSourceSelection = !m_sourceTable->selectionModel()->selectedRows().isEmpty();
    bool hasDestSelection = !m_destinationTable->selectionModel()->selectedRows().isEmpty();
    m_matchButton->setEnabled(hasSourceSelection && hasDestSelection);

    // Enable unmatch button if either side has selections
    m_unmatchButton->setEnabled(hasSourceSelection || hasDestSelection);

    // Apply is always enabled if there are tracks on both sides
    m_applyButton->setEnabled(m_sourceModel->trackCount() > 0 && m_destinationModel->trackCount() > 0);
}

QString TrackMatchDialog::formatDuration(int seconds) const
{
    if(seconds <= 0) {
        return QString();
    }
    int minutes = seconds / 60;
    int secs = seconds % 60;
    return QString("%1:%2").arg(minutes).arg(secs, 2, 10, QChar('0'));
}

QString TrackMatchDialog::formatConfidence(double confidence) const
{
    return QString("%1%").arg(static_cast<int>(confidence * 100));
}

} // namespace Tagger
