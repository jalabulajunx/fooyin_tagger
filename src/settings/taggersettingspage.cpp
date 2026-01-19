#include "taggersettingspage.h"
#include "taggersettings.h"
#include "core/taggingmanager.h"

#include <utils/settings/settingsmanager.h>

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

TaggerSettingsPage::TaggerSettingsPage(Fooyin::SettingsManager* settings, TaggingManager* manager)
    : SettingsPage{nullptr}
{
    Q_UNUSED(settings)
    Q_UNUSED(manager)
    setId("fooyin.settings.tagger");
    setName(tr("Audio Tagger"));
    setCategory({"Plugins", "Audio Tagger"});
    setWidgetCreator([settings, manager]() {
        return new TaggerSettingsPageWidget(settings, manager);
    });
}

TaggerSettingsPageWidget::TaggerSettingsPageWidget(Fooyin::SettingsManager* settings, TaggingManager* manager)
    : m_settings{settings}
    , m_manager{manager}
{
    auto* layout = new QVBoxLayout(this);

    // Source settings group
    auto* sourceGroup = new QGroupBox(tr("Default Settings"), this);
    auto* sourceLayout = new QFormLayout(sourceGroup);

    m_sourceCombo = new QComboBox(this);
    m_sourceCombo->addItem(tr("Wikipedia"), static_cast<int>(Tagger::SourceType::Wikipedia));
    m_sourceCombo->addItem(tr("MusicBrainz"), static_cast<int>(Tagger::SourceType::MusicBrainz));
    sourceLayout->addRow(tr("Default source:"), m_sourceCombo);

    // Matching settings group
    auto* matchGroup = new QGroupBox(tr("Matching Settings"), this);
    auto* matchLayout = new QFormLayout(matchGroup);

    m_confidenceSpin = new QSpinBox(this);
    m_confidenceSpin->setRange(0, 100);
    m_confidenceSpin->setSingleStep(5);
    m_confidenceSpin->setSuffix(tr("%"));
    m_confidenceSpin->setToolTip(tr("Minimum confidence score (0-100%) for automatic track matching"));
    matchLayout->addRow(tr("Confidence threshold:"), m_confidenceSpin);

    m_durationSpin = new QSpinBox(this);
    m_durationSpin->setRange(0, 30);
    m_durationSpin->setSuffix(tr(" seconds"));
    m_durationSpin->setToolTip(tr("Maximum duration difference for matching tracks"));
    matchLayout->addRow(tr("Duration tolerance:"), m_durationSpin);

    layout->addWidget(sourceGroup);
    layout->addWidget(matchGroup);
    layout->addStretch();
}

void TaggerSettingsPageWidget::load()
{
    int source = m_settings->value<TaggerSettings::DefaultSource>();
    int index = m_sourceCombo->findData(source);
    if(index >= 0) {
        m_sourceCombo->setCurrentIndex(index);
    }

    m_confidenceSpin->setValue(m_settings->value<TaggerSettings::ConfidenceThreshold>());
    m_durationSpin->setValue(m_settings->value<TaggerSettings::DurationTolerance>());
}

void TaggerSettingsPageWidget::apply()
{
    m_settings->set<TaggerSettings::DefaultSource>(m_sourceCombo->currentData().toInt());
    m_settings->set<TaggerSettings::ConfidenceThreshold>(m_confidenceSpin->value());
    m_settings->set<TaggerSettings::DurationTolerance>(m_durationSpin->value());
}

void TaggerSettingsPageWidget::reset()
{
    m_sourceCombo->setCurrentIndex(0);
    m_confidenceSpin->setValue(60);
    m_durationSpin->setValue(3);
}
