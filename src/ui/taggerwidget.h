#pragma once

#include "core/taggingmanager.h"
#include "models/matchresult.h"
#include <tagger/tagger_common.h>

#include <gui/fywidget.h>
#include <core/track.h>

namespace Fooyin {
class SettingsManager;
}

namespace Tagger {
class TrackMatchDialog;
}

class TaggingManager;
class QButtonGroup;
class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QTableWidget;

class TaggerWidget : public Fooyin::FyWidget
{
    Q_OBJECT

public:
    explicit TaggerWidget(TaggingManager* manager, Fooyin::SettingsManager* settings = nullptr, QWidget* parent = nullptr);

    [[nodiscard]] QString name() const override { return QStringLiteral("Audio Tagger"); }
    [[nodiscard]] QString layoutName() const override { return QStringLiteral("AudioTagger"); }

    void loadTracks(const Fooyin::TrackList& tracks);

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSourceChanged();
    void onSearchTypeChanged();
    void onFetchClicked();
    void onSearchClicked();
    void onDirectFetchClicked();
    void onSearchResultDoubleClicked(int row);
    void onFetchCompleted(const Tagger::AlbumMetadata& metadata);
    void onFetchFailed(const QString& error);
    void onSearchResults(const QList<Tagger::AlbumMetadata>& results);
    void onApplyClicked();
    void onMatchCheckChanged(int row, int column);
    void onTagWriteProgress(int current, int total);
    void onTagWriteCompleted(int successCount, int failCount);
    void onOverrideMatchesClicked();

private:
    void setupUI();
    void updateSourcePanel();
    void updateMatchPreview();
    void updateStatus(const QString& message);
    void setUIEnabled(bool enabled);

    TaggingManager* m_manager;
    Fooyin::SettingsManager* m_settings;

    // Loaded data
    Fooyin::TrackList m_tracks;
    Tagger::AlbumMetadata m_fetchedMetadata;
    QList<Tagger::MatchResult> m_matchResults;
    QList<Tagger::AlbumMetadata> m_searchResultsCache;

    // Source selection
    QButtonGroup* m_sourceGroup;
    QRadioButton* m_wikipediaRadio;
    QRadioButton* m_musicbrainzRadio;

    // Wikipedia panel
    QWidget* m_wikipediaPanel;
    QLineEdit* m_urlEdit;
    QPushButton* m_fetchButton;

    // MusicBrainz panel
    QWidget* m_musicbrainzPanel;
    QButtonGroup* m_searchTypeGroup;
    QRadioButton* m_searchRadio;
    QRadioButton* m_directRadio;
    QWidget* m_searchPanel;
    QWidget* m_directPanel;
    QLineEdit* m_artistEdit;
    QLineEdit* m_albumEdit;
    QPushButton* m_searchButton;
    QLineEdit* m_releaseIdEdit;
    QPushButton* m_directFetchButton;
    QListWidget* m_searchResultsList;

    // Match preview table
    QTableWidget* m_matchTable;

    // Field selection
    QCheckBox* m_writeTitleCheck;
    QCheckBox* m_writeArtistCheck;
    QCheckBox* m_writeAlbumCheck;
    QCheckBox* m_writeLyricsCheck;
    QCheckBox* m_writeYearCheck;
    QCheckBox* m_writeComposerCheck;

    // Status and actions
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QPushButton* m_overrideMatchesButton;
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
};
