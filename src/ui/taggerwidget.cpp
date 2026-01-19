#include "taggerwidget.h"
#include "core/taggingmanager.h"
#include "settings/taggersettings.h"
#include "ui/trackmatchdialog.h"

#include <core/track.h>
#include <utils/settings/settingsmanager.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QVBoxLayout>

TaggerWidget::TaggerWidget(TaggingManager* manager, Fooyin::SettingsManager* settings, QWidget* parent)
    : FyWidget(parent)
    , m_manager(manager)
    , m_settings(settings)
{
    setupUI();

    // Connect manager signals
    connect(m_manager, &TaggingManager::fetchStarted, this, [this]() {
        setUIEnabled(false);
        updateStatus(tr("Fetching metadata..."));
        m_progressBar->setRange(0, 0); // Indeterminate
    });

    connect(m_manager, &TaggingManager::fetchProgress, this, [this](int percent) {
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(percent);
    });

    connect(m_manager, &TaggingManager::fetchCompleted, this, &TaggerWidget::onFetchCompleted);
    connect(m_manager, &TaggingManager::fetchFailed, this, &TaggerWidget::onFetchFailed);
    connect(m_manager, &TaggingManager::searchResults, this, &TaggerWidget::onSearchResults);
    connect(m_manager, &TaggingManager::tagWriteProgress, this, &TaggerWidget::onTagWriteProgress);
    connect(m_manager, &TaggingManager::tagWriteCompleted, this, &TaggerWidget::onTagWriteCompleted);
}

void TaggerWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Source selection
    auto* sourceGroup = new QGroupBox(tr("Source"), this);
    auto* sourceLayout = new QHBoxLayout(sourceGroup);

    m_sourceGroup = new QButtonGroup(this);
    m_wikipediaRadio = new QRadioButton(tr("Wikipedia"), this);
    m_musicbrainzRadio = new QRadioButton(tr("MusicBrainz"), this);
    m_sourceGroup->addButton(m_wikipediaRadio, static_cast<int>(Tagger::SourceType::Wikipedia));
    m_sourceGroup->addButton(m_musicbrainzRadio, static_cast<int>(Tagger::SourceType::MusicBrainz));
    m_wikipediaRadio->setChecked(true);

    sourceLayout->addWidget(m_wikipediaRadio);
    sourceLayout->addWidget(m_musicbrainzRadio);
    sourceLayout->addStretch();

    connect(m_sourceGroup, &QButtonGroup::idClicked, this, &TaggerWidget::onSourceChanged);

    // Wikipedia panel
    m_wikipediaPanel = new QWidget(this);
    auto* wikiLayout = new QHBoxLayout(m_wikipediaPanel);
    wikiLayout->setContentsMargins(0, 0, 0, 0);

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText(tr("https://en.wikipedia.org/wiki/FilmName"));
    m_fetchButton = new QPushButton(tr("Fetch"), this);

    wikiLayout->addWidget(new QLabel(tr("URL:"), this));
    wikiLayout->addWidget(m_urlEdit, 1);
    wikiLayout->addWidget(m_fetchButton);

    connect(m_fetchButton, &QPushButton::clicked, this, &TaggerWidget::onFetchClicked);
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &TaggerWidget::onFetchClicked);

    // MusicBrainz panel
    m_musicbrainzPanel = new QWidget(this);
    auto* mbLayout = new QVBoxLayout(m_musicbrainzPanel);
    mbLayout->setContentsMargins(0, 0, 0, 0);

    // Search type selection
    auto* searchTypeLayout = new QHBoxLayout();
    m_searchTypeGroup = new QButtonGroup(this);
    m_searchRadio = new QRadioButton(tr("Search by Artist/Album"), this);
    m_directRadio = new QRadioButton(tr("Direct Release/Group ID"), this);
    m_searchTypeGroup->addButton(m_searchRadio);
    m_searchTypeGroup->addButton(m_directRadio);
    m_searchRadio->setChecked(true);

    searchTypeLayout->addWidget(m_searchRadio);
    searchTypeLayout->addWidget(m_directRadio);
    searchTypeLayout->addStretch();

    // Search by Artist/Album
    m_searchPanel = new QWidget(this);
    auto* searchLayout = new QHBoxLayout(m_searchPanel);
    m_artistEdit = new QLineEdit(this);
    m_artistEdit->setPlaceholderText(tr("Artist"));
    m_albumEdit = new QLineEdit(this);
    m_albumEdit->setPlaceholderText(tr("Album"));
    m_searchButton = new QPushButton(tr("Search"), this);

    searchLayout->addWidget(new QLabel(tr("Artist:"), this));
    searchLayout->addWidget(m_artistEdit, 1);
    searchLayout->addWidget(new QLabel(tr("Album:"), this));
    searchLayout->addWidget(m_albumEdit, 1);
    searchLayout->addWidget(m_searchButton);

    m_searchResultsList = new QListWidget(this);
    m_searchResultsList->setMaximumHeight(120);

    // Direct ID input
    m_directPanel = new QWidget(this);
    auto* directLayout = new QHBoxLayout(m_directPanel);
    m_releaseIdEdit = new QLineEdit(this);
    m_releaseIdEdit->setPlaceholderText(tr("Enter MusicBrainz Release or Release Group ID"));
    m_directFetchButton = new QPushButton(tr("Fetch"), this);

    directLayout->addWidget(new QLabel(tr("ID/URL:"), this));
    directLayout->addWidget(m_releaseIdEdit, 1);
    directLayout->addWidget(m_directFetchButton);

    m_directPanel->hide();

    mbLayout->addLayout(searchTypeLayout);
    mbLayout->addWidget(m_searchPanel);
    mbLayout->addWidget(new QLabel(tr("Search results (double-click to select):"), this));
    mbLayout->addWidget(m_searchResultsList);
    mbLayout->addWidget(m_directPanel);

    connect(m_searchTypeGroup, &QButtonGroup::idClicked, this, &TaggerWidget::onSearchTypeChanged);
    connect(m_searchButton, &QPushButton::clicked, this, &TaggerWidget::onSearchClicked);
    connect(m_artistEdit, &QLineEdit::returnPressed, this, &TaggerWidget::onSearchClicked);
    connect(m_albumEdit, &QLineEdit::returnPressed, this, &TaggerWidget::onSearchClicked);
    connect(m_searchResultsList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        int row = m_searchResultsList->row(item);
        onSearchResultDoubleClicked(row);
    });
    connect(m_directFetchButton, &QPushButton::clicked, this, &TaggerWidget::onDirectFetchClicked);
    connect(m_releaseIdEdit, &QLineEdit::returnPressed, this, &TaggerWidget::onDirectFetchClicked);

    m_musicbrainzPanel->hide();

    // Match preview table
    auto* matchGroup = new QGroupBox(tr("Track Matching"), this);
    auto* matchLayout = new QVBoxLayout(matchGroup);

    m_matchTable = new QTableWidget(this);
    m_matchTable->setColumnCount(5);
    m_matchTable->setHorizontalHeaderLabels({tr("Apply"), tr("Your Track"), tr("Matched Title"), tr("Artist"), tr("Confidence")});
    m_matchTable->horizontalHeader()->setStretchLastSection(false);
    m_matchTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_matchTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_matchTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_matchTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_matchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_matchTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(m_matchTable, &QTableWidget::cellChanged, this, &TaggerWidget::onMatchCheckChanged);

    matchLayout->addWidget(m_matchTable);

    // Field selection
    auto* fieldsLayout = new QHBoxLayout();
    fieldsLayout->addWidget(new QLabel(tr("Write fields:"), this));

    m_writeTitleCheck = new QCheckBox(tr("Title"), this);
    m_writeArtistCheck = new QCheckBox(tr("Artist"), this);
    m_writeAlbumCheck = new QCheckBox(tr("Album"), this);
    m_writeLyricsCheck = new QCheckBox(tr("Lyricist"), this);
    m_writeYearCheck = new QCheckBox(tr("Year"), this);
    m_writeComposerCheck = new QCheckBox(tr("Composer"), this);

    m_writeTitleCheck->setChecked(true);
    m_writeArtistCheck->setChecked(true);
    m_writeAlbumCheck->setChecked(true);

    fieldsLayout->addWidget(m_writeTitleCheck);
    fieldsLayout->addWidget(m_writeArtistCheck);
    fieldsLayout->addWidget(m_writeAlbumCheck);
    fieldsLayout->addWidget(m_writeLyricsCheck);
    fieldsLayout->addWidget(m_writeYearCheck);
    fieldsLayout->addWidget(m_writeComposerCheck);
    fieldsLayout->addStretch();

    matchLayout->addLayout(fieldsLayout);

    // Status and buttons
    auto* bottomLayout = new QHBoxLayout();

    m_statusLabel = new QLabel(tr("Select tracks and fetch metadata"), this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setMaximumWidth(200);

    m_overrideMatchesButton = new QPushButton(tr("Override Matches"), this);
    m_overrideMatchesButton->setEnabled(false);
    m_overrideMatchesButton->setToolTip(tr("Manually override track matching between source and destination"));

    m_applyButton = new QPushButton(tr("Apply Tags"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_applyButton->setEnabled(false);

    connect(m_overrideMatchesButton, &QPushButton::clicked, this, &TaggerWidget::onOverrideMatchesClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &TaggerWidget::onApplyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QWidget::close);

    bottomLayout->addWidget(m_statusLabel, 1);
    bottomLayout->addWidget(m_progressBar);
    bottomLayout->addWidget(m_overrideMatchesButton);
    bottomLayout->addWidget(m_applyButton);
    bottomLayout->addWidget(m_cancelButton);

    // Assemble layout
    mainLayout->addWidget(sourceGroup);
    mainLayout->addWidget(m_wikipediaPanel);
    mainLayout->addWidget(m_musicbrainzPanel);
    mainLayout->addWidget(matchGroup, 1);
    mainLayout->addLayout(bottomLayout);
}

void TaggerWidget::loadTracks(const Fooyin::TrackList& tracks)
{
    m_tracks = tracks;
    m_fetchedMetadata = Tagger::AlbumMetadata();
    m_matchResults.clear();

    // Try to pre-fill artist/album from first track
    if(!tracks.empty()) {
        const auto& first = tracks.front();
        m_artistEdit->setText(first.albumArtist().isEmpty() ? first.artist() : first.albumArtist());
        m_albumEdit->setText(first.album());
    }

    // Clear match table
    m_matchTable->setRowCount(0);

    updateStatus(tr("%1 track(s) loaded").arg(tracks.size()));
    m_applyButton->setEnabled(false);
    m_overrideMatchesButton->setEnabled(false);
}

void TaggerWidget::onSearchTypeChanged()
{
    bool isSearch = m_searchRadio->isChecked();
    m_searchPanel->setVisible(isSearch);
    m_searchResultsList->setVisible(isSearch);
    m_directPanel->setVisible(!isSearch);
}

void TaggerWidget::onSourceChanged()
{
    updateSourcePanel();
}

void TaggerWidget::updateSourcePanel()
{
    int sourceId = m_sourceGroup->checkedId();
    bool isWikipedia = (sourceId == static_cast<int>(Tagger::SourceType::Wikipedia));

    m_wikipediaPanel->setVisible(isWikipedia);
    m_musicbrainzPanel->setVisible(!isWikipedia);
}

void TaggerWidget::onFetchClicked()
{
    QString url = m_urlEdit->text().trimmed();
    if(url.isEmpty()) {
        updateStatus(tr("Please enter a Wikipedia URL"));
        return;
    }

    m_manager->fetchFromUrl(Tagger::SourceType::Wikipedia, url);
}

void TaggerWidget::onSearchClicked()
{
    QString artist = m_artistEdit->text().trimmed();
    QString album = m_albumEdit->text().trimmed();

    if(artist.isEmpty() && album.isEmpty()) {
        updateStatus(tr("Please enter artist and/or album"));
        return;
    }

    m_searchResultsList->clear();
    m_searchResultsCache.clear();

    m_manager->searchAlbum(Tagger::SourceType::MusicBrainz, artist, album);
}

void TaggerWidget::onDirectFetchClicked()
{
    QString id = m_releaseIdEdit->text().trimmed();
    if(id.isEmpty()) {
        updateStatus(tr("Please enter a MusicBrainz Release or Release Group ID"));
        return;
    }

    m_manager->fetchFromUrl(Tagger::SourceType::MusicBrainz, id);
}

void TaggerWidget::onSearchResultDoubleClicked(int row)
{
    if(row < 0 || row >= m_searchResultsCache.size()) {
        return;
    }

    const auto& selected = m_searchResultsCache[row];
    m_manager->fetchFromUrl(Tagger::SourceType::MusicBrainz, selected.releaseId);
}

void TaggerWidget::onFetchCompleted(const Tagger::AlbumMetadata& metadata)
{
    m_fetchedMetadata = metadata;
    setUIEnabled(true);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(100);

    // Perform matching
    m_matchResults = m_manager->matchTracks(m_tracks, metadata);

    updateMatchPreview();

    int matchedCount = 0;
    for(const auto& match : m_matchResults) {
        if(match.isValid() && match.confidence >= m_manager->confidenceThreshold()) {
            matchedCount++;
        }
    }

    updateStatus(tr("Fetched %1 tracks, matched %2/%3")
                     .arg(metadata.tracks.size())
                     .arg(matchedCount)
                     .arg(m_tracks.size()));

    m_applyButton->setEnabled(matchedCount > 0);
    m_overrideMatchesButton->setEnabled(!metadata.tracks.isEmpty() && !m_tracks.empty());
}

void TaggerWidget::onFetchFailed(const QString& error)
{
    setUIEnabled(true);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    updateStatus(tr("Error: %1").arg(error));
}

void TaggerWidget::onSearchResults(const QList<Tagger::AlbumMetadata>& results)
{
    setUIEnabled(true);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    m_searchResultsCache = results;
    m_searchResultsList->clear();

    for(const auto& result : results) {
        QString text = QString("%1 - %2").arg(result.album, result.albumArtist);
        if(result.year > 0) {
            text += QString(" (%1)").arg(result.year);
        }
        if(!result.country.isEmpty()) {
            text += QString(" [%1]").arg(result.country);
        }
        m_searchResultsList->addItem(text);
    }

    updateStatus(tr("Found %1 release(s)").arg(results.size()));
}

void TaggerWidget::updateMatchPreview()
{
    m_matchTable->blockSignals(true);
    m_matchTable->setRowCount(m_matchResults.size());

    for(int i = 0; i < m_matchResults.size(); ++i) {
        const auto& match = m_matchResults[i];

        // Checkbox column
        auto* checkItem = new QTableWidgetItem();
        checkItem->setCheckState(match.selected ? Qt::Checked : Qt::Unchecked);
        checkItem->setFlags(checkItem->flags() | Qt::ItemIsUserCheckable);
        m_matchTable->setItem(i, 0, checkItem);

        // Your track column
        QFileInfo fi(match.targetFilepath);
        auto* trackItem = new QTableWidgetItem(fi.fileName());
        trackItem->setToolTip(match.targetFilepath);
        m_matchTable->setItem(i, 1, trackItem);

        // Matched title column
        QString matchedTitle = match.isValid() ? match.sourceMetadata.title : tr("(No match)");
        auto* titleItem = new QTableWidgetItem(matchedTitle);
        if(!match.isValid()) {
            titleItem->setForeground(Qt::gray);
        }
        m_matchTable->setItem(i, 2, titleItem);

        // Artist column
        QString artist = match.isValid() ? match.sourceMetadata.artist : QString();
        m_matchTable->setItem(i, 3, new QTableWidgetItem(artist));

        // Confidence column
        QString confStr;
        if(match.isValid()) {
            int confPct = static_cast<int>(match.confidence * 100);
            confStr = QString("%1%").arg(confPct);
        }
        else {
            confStr = "-";
        }
        auto* confItem = new QTableWidgetItem(confStr);
        confItem->setTextAlignment(Qt::AlignCenter);

        // Color-code confidence
        if(match.isHighConfidence()) {
            confItem->setForeground(QColor(0, 150, 0)); // Green
        }
        else if(match.isMediumConfidence()) {
            confItem->setForeground(QColor(200, 150, 0)); // Orange
        }
        else if(match.isLowConfidence()) {
            confItem->setForeground(QColor(200, 0, 0)); // Red
        }

        m_matchTable->setItem(i, 4, confItem);
    }

    m_matchTable->blockSignals(false);
}

void TaggerWidget::onMatchCheckChanged(int row, int column)
{
    if(column != 0 || row < 0 || row >= m_matchResults.size()) {
        return;
    }

    auto* item = m_matchTable->item(row, 0);
    if(item) {
        m_matchResults[row].selected = (item->checkState() == Qt::Checked);
    }

    // Update apply button state
    bool hasSelected = false;
    for(const auto& match : m_matchResults) {
        if(match.selected && match.isValid()) {
            hasSelected = true;
            break;
        }
    }
    m_applyButton->setEnabled(hasSelected);
}

void TaggerWidget::onApplyClicked()
{
    TaggingManager::TagWriteOptions options;
    options.writeTitle = m_writeTitleCheck->isChecked();
    options.writeArtist = m_writeArtistCheck->isChecked();
    options.writeAlbum = m_writeAlbumCheck->isChecked();
    options.writeLyrics = m_writeLyricsCheck->isChecked();
    options.writeYear = m_writeYearCheck->isChecked();
    options.writeComposer = m_writeComposerCheck->isChecked();

    setUIEnabled(false);
    updateStatus(tr("Writing tags..."));
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);

    m_manager->applyTags(m_matchResults, options);
}

void TaggerWidget::onTagWriteProgress(int current, int total)
{
    if(total > 0) {
        m_progressBar->setValue(current * 100 / total);
    }
    updateStatus(tr("Writing tags... %1/%2").arg(current).arg(total));
}

void TaggerWidget::onTagWriteCompleted(int successCount, int failCount)
{
    setUIEnabled(true);
    m_progressBar->setValue(100);

    QString message = tr("Tags written: %1 succeeded").arg(successCount);
    if(failCount > 0) {
        message += tr(", %1 failed").arg(failCount);
    }
    updateStatus(message);

    if(failCount == 0) {
        QMessageBox::information(this, tr("Tagging Complete"), message);
    }
    else {
        QMessageBox::warning(this, tr("Tagging Complete"), message);
    }
}

void TaggerWidget::updateStatus(const QString& message)
{
    m_statusLabel->setText(message);
}

void TaggerWidget::setUIEnabled(bool enabled)
{
    m_wikipediaPanel->setEnabled(enabled);
    m_musicbrainzPanel->setEnabled(enabled);
    m_matchTable->setEnabled(enabled);
    m_overrideMatchesButton->setEnabled(enabled && !m_fetchedMetadata.tracks.isEmpty() && !m_tracks.empty());
    m_applyButton->setEnabled(enabled && !m_matchResults.isEmpty());
    m_sourceGroup->setExclusive(!enabled); // Allow unchecking during operations
}

void TaggerWidget::closeEvent(QCloseEvent* event)
{
    // Save window size
    if(m_settings) {
        m_settings->set<TaggerSettings::WindowWidth>(width());
        m_settings->set<TaggerSettings::WindowHeight>(height());
    }

    m_manager->cancelFetch();
    event->accept();
}

void TaggerWidget::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    FyWidget::keyPressEvent(event);
}

void TaggerWidget::onOverrideMatchesClicked()
{
    if(m_fetchedMetadata.tracks.isEmpty() || m_tracks.empty()) {
        QMessageBox::warning(this, tr("No Data"), tr("No tracks available for matching override."));
        return;
    }

    // Create and show the track match dialog
    Tagger::TrackMatchDialog dialog(m_fetchedMetadata, m_tracks, m_matchResults, this);

    if(dialog.exec() == QDialog::Accepted) {
        // Update match results with user's overrides
        m_matchResults = dialog.getMatches();

        // Update the match preview table
        updateMatchPreview();

        // Update apply button state
        bool hasSelected = false;
        for(const auto& match : m_matchResults) {
            if(match.selected && match.isValid()) {
                hasSelected = true;
                break;
            }
        }
        m_applyButton->setEnabled(hasSelected);

        int matchedCount = 0;
        for(const auto& match : m_matchResults) {
            if(match.isValid()) {
                matchedCount++;
            }
        }

        updateStatus(tr("Track matching updated: %1/%2 tracks matched")
                         .arg(matchedCount)
                         .arg(m_tracks.size()));
    }
}
