#pragma once

#include <core/plugins/coreplugin.h>
#include <core/plugins/plugin.h>
#include <gui/plugins/guiplugin.h>

class QAction;

namespace Fooyin {
class TrackSelectionController;
class SettingsManager;
}

class TaggingManager;
class TaggerWidget;

class TaggerPlugin : public QObject,
                     public Fooyin::Plugin,
                     public Fooyin::CorePlugin,
                     public Fooyin::GuiPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.fooyin.fooyin.plugin/1.0" FILE "../metadata.json")
    Q_INTERFACES(Fooyin::Plugin Fooyin::CorePlugin Fooyin::GuiPlugin)

public:
    void initialise(const Fooyin::CorePluginContext& context) override;
    void initialise(const Fooyin::GuiPluginContext& context) override;

private slots:
    void showTaggerDialog();

private:
    TaggingManager* m_manager{nullptr};
    TaggerWidget* m_taggerDialog{nullptr};
    Fooyin::TrackSelectionController* m_trackSelection{nullptr};
    Fooyin::SettingsManager* m_settings{nullptr};
    QAction* m_tagAction{nullptr};
};
