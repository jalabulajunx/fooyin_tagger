#pragma once

#include "models/matchresult.h"
#include "models/albummetadata.h"
#include <tagger/tagger_common.h>

#include <QDialog>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>

#include <core/track.h>

class QTableView;
class QPushButton;
class QLabel;
class QSplitter;

namespace Tagger {

// Model for source tracks (Wiki/MusicBrainz)
class SourceTrackModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        TrackNumber,
        Title,
        Artist,
        Duration,
        Confidence,
        ColumnCount
    };

    explicit SourceTrackModel(QObject* parent = nullptr);

    void setTracks(const QList<TrackMetadata>& tracks, const QList<double>& confidences);
    [[nodiscard]] QList<TrackMetadata> tracks() const { return m_tracks; }
    [[nodiscard]] QList<double> confidences() const { return m_confidences; }
    [[nodiscard]] int trackCount() const { return m_tracks.size(); }

    // Drag and drop support
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                 const QModelIndex& destinationParent, int destinationChild);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QList<TrackMetadata> m_tracks;
    QList<double> m_confidences;
};

// Model for destination tracks (user's tracks)
class DestinationTrackModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        Filename,
        Title,
        Artist,
        Duration,
        ColumnCount
    };

    explicit DestinationTrackModel(QObject* parent = nullptr);

    void setTracks(const Fooyin::TrackList& tracks);
    [[nodiscard]] Fooyin::TrackList tracks() const { return m_tracks; }
    [[nodiscard]] int trackCount() const { return m_tracks.size(); }

    // Drag and drop support
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                 const QModelIndex& destinationParent, int destinationChild);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    Fooyin::TrackList m_tracks;
};

// Delegate for drawing connection lines between matched tracks
class TrackMatchDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit TrackMatchDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

// Main dialog for track matching override
class TrackMatchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrackMatchDialog(const AlbumMetadata& sourceMetadata,
                             const Fooyin::TrackList& userTracks,
                             const QList<MatchResult>& initialMatches,
                             QWidget* parent = nullptr);

    [[nodiscard]] QList<MatchResult> getMatches() const { return m_matches; }

private slots:
    void onSourceSelectionChanged();
    void onDestinationSelectionChanged();
    void onMatchTracks();
    void onUnmatchTracks();
    void onAutoMatch();
    void onClearAllMatches();
    void onApply();
    void onCancel();
    void onMoveSourceUp();
    void onMoveSourceDown();
    void onMoveDestUp();
    void onMoveDestDown();

private:
    void setupUI();
    void updateMatchVisuals();
    void updateMatchButtons();
    [[nodiscard]] QString formatDuration(int seconds) const;
    [[nodiscard]] QString formatConfidence(double confidence) const;

    AlbumMetadata m_sourceMetadata;
    Fooyin::TrackList m_userTracks;
    QList<MatchResult> m_matches;

    SourceTrackModel* m_sourceModel;
    DestinationTrackModel* m_destinationModel;

    QTableView* m_sourceTable;
    QTableView* m_destinationTable;
    QPushButton* m_matchButton;
    QPushButton* m_unmatchButton;
    QPushButton* m_autoMatchButton;
    QPushButton* m_clearAllButton;
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
    QPushButton* m_moveSourceUpButton;
    QPushButton* m_moveSourceDownButton;
    QPushButton* m_moveDestUpButton;
    QPushButton* m_moveDestDownButton;
    QLabel* m_statusLabel;
};

} // namespace Tagger
