#pragma once

#include <utils/settings/settingspage.h>

namespace Fooyin {
class SettingsManager;
}

class TaggingManager;

class TaggerSettingsPage : public Fooyin::SettingsPage
{
    Q_OBJECT

public:
    explicit TaggerSettingsPage(Fooyin::SettingsManager* settings, TaggingManager* manager);
};

class TaggerSettingsPageWidget : public Fooyin::SettingsPageWidget
{
    Q_OBJECT

public:
    explicit TaggerSettingsPageWidget(Fooyin::SettingsManager* settings, TaggingManager* manager);

    void load() override;
    void apply() override;
    void reset() override;

private:
    Fooyin::SettingsManager* m_settings;
    TaggingManager* m_manager;

    class QComboBox* m_sourceCombo;
    class QSpinBox* m_confidenceSpin;
    class QSpinBox* m_durationSpin;
};
