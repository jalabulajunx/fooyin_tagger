#include "taggerplugin.h"
#include "core/taggingmanager.h"
#include "ui/taggerwidget.h"
#include "settings/taggersettings.h"
#include "settings/taggersettingspage.h"

#include <gui/widgetprovider.h>
#include <gui/trackselectioncontroller.h>
#include <gui/guiconstants.h>
#include <utils/actions/actionmanager.h>
#include <utils/actions/actioncontainer.h>
#include <utils/settings/settingsmanager.h>
#include <utils/id.h>

#include <QDebug>
#include <QAction>

void TaggerPlugin::initialise(const Fooyin::CorePluginContext& context)
{
    m_settings = context.settingsManager;

    // Register settings with default values
    m_settings->createSetting<TaggerSettings::DefaultSource>(
        static_cast<int>(Tagger::SourceType::Wikipedia),
        "AudioTagger/DefaultSource"
    );
    m_settings->createSetting<TaggerSettings::ConfidenceThreshold>(
        60,  // Stored as int percentage (60 = 0.6)
        "AudioTagger/ConfidenceThreshold"
    );
    m_settings->createSetting<TaggerSettings::DurationTolerance>(
        3,
        "AudioTagger/DurationTolerance"
    );
    m_settings->createSetting<TaggerSettings::WindowWidth>(700, "AudioTagger/WindowWidth");
    m_settings->createSetting<TaggerSettings::WindowHeight>(600, "AudioTagger/WindowHeight");

    qInfo() << "Audio Tagger plugin: Settings registered";
}

void TaggerPlugin::initialise(const Fooyin::GuiPluginContext& context)
{
    // Initialize tagging manager
    m_manager = new TaggingManager(this);

    // Store track selection controller
    m_trackSelection = context.trackSelection;

    // Register settings page
    new TaggerSettingsPage(m_settings, m_manager);

    // Register widget with fooyin
    context.widgetProvider->registerWidget(
        "AudioTagger",
        [this]() {
            return new TaggerWidget(m_manager, m_settings);
        },
        "Audio Tagger"
    );

    // Create and register context menu action
    m_tagAction = new QAction(tr("Tag with metadata..."), this);
    m_tagAction->setStatusTip(tr("Fetch and apply metadata from Wikipedia or MusicBrainz"));
    connect(m_tagAction, &QAction::triggered, this, &TaggerPlugin::showTaggerDialog);

    // Register action with Fooyin's action manager
    auto* command = context.actionManager->registerAction(
        m_tagAction,
        Fooyin::Id{"AudioTagger.Tag"},
        Fooyin::Context{Fooyin::Constants::Context::Global}
    );

    if(command) {
        // Add to track selection context menu
        auto* trackMenu = context.actionManager->actionContainer(
            Fooyin::Id{Fooyin::Constants::Menus::Context::TrackSelection}
        );
        if(trackMenu) {
            trackMenu->addAction(command);
            qInfo() << "Audio Tagger action added to track context menu";
        }
        else {
            qWarning() << "Track selection menu not found";
        }
    }
    else {
        qWarning() << "Failed to register Audio Tagger action";
    }

    qInfo() << "Audio Tagger plugin GUI initialized";
}

void TaggerPlugin::showTaggerDialog()
{
    if(!m_trackSelection->hasTracks()) {
        qWarning() << "No tracks selected";
        return;
    }

    auto tracks = m_trackSelection->selectedTracks();
    if(tracks.empty()) {
        qWarning() << "No tracks selected";
        return;
    }

    qInfo() << "Tagging" << tracks.size() << "track(s)";

    // Create or reuse tagger dialog
    if(!m_taggerDialog) {
        m_taggerDialog = new TaggerWidget(m_manager, m_settings);
        m_taggerDialog->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
        m_taggerDialog->setWindowModality(Qt::ApplicationModal);

        // Apply window size from settings
        int width = m_settings->value<TaggerSettings::WindowWidth>();
        int height = m_settings->value<TaggerSettings::WindowHeight>();
        m_taggerDialog->resize(width, height);
    }

    // Load selected tracks into the tagger
    m_taggerDialog->loadTracks(tracks);

    // Show the dialog
    m_taggerDialog->show();
    m_taggerDialog->raise();
    m_taggerDialog->activateWindow();
}
