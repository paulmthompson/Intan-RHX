//------------------------------------------------------------------------------
// Selected-channel RHS digital-output live controls.
//------------------------------------------------------------------------------

#include "rhsdigoutlivecontrolswidget.hpp"

#include "ui_rhsdigoutlivecontrolswidget.h"

#include <QPushButton>

RhsDigOutLiveControlsWidget::RhsDigOutLiveControlsWidget(QWidget* parent)
    : QWidget(parent)
    , _ui(new Ui::RhsDigOutLiveControlsWidget)
{
    _ui->setupUi(this);
    connect(_ui->actionButton, &QPushButton::clicked,
            this, &RhsDigOutLiveControlsWidget::_actionClicked);
    hide();
}

RhsDigOutLiveControlsWidget::~RhsDigOutLiveControlsWidget()
{
    delete _ui;
}

void RhsDigOutLiveControlsWidget::updateState(
    const RhsDigOutConfiguration& configuration,
    const RhsDigOutRuntimeState& runtime,
    bool acquisitionRunning,
    bool commandAvailable,
    bool thresholdOwned,
    bool uploadInProgress)
{
    _configuration = configuration;
    _runtime = runtime;

    const bool manualLevel =
        configuration.m_source == RhsDigOutSource::ManualLevel;
    const bool continuous =
        configuration.m_source == RhsDigOutSource::Sequencer &&
        configuration.m_sequenceMode == RhsDigOutSequenceMode::Continuous &&
        configuration.m_enabled;
    const bool manualFire =
        configuration.m_source == RhsDigOutSource::Sequencer &&
        configuration.m_sequenceMode != RhsDigOutSequenceMode::Continuous &&
        configuration.m_enabled && configuration.m_manualFire;
    const bool applicable = manualLevel || continuous || manualFire;
    setVisible(applicable);
    if (!applicable) {
        return;
    }

    if (manualLevel) {
        _ui->modeLabel->setText(tr("Manual Level:"));
        _ui->actionButton->setText(
            runtime.m_manualLevelHigh ? tr("Set Low") : tr("Set High"));
    } else if (continuous) {
        _ui->modeLabel->setText(tr("Continuous:"));
        _ui->actionButton->setText(
            runtime.m_continuousRunning ? tr("Stop") : tr("Start"));
    } else {
        _ui->modeLabel->setText(tr("Manual Trigger:"));
        _ui->actionButton->setText(
            runtime.m_manualFireBusy ? tr("Busy") : tr("Fire"));
    }

    _ui->actionButton->setEnabled(
        acquisitionRunning && commandAvailable && !thresholdOwned &&
        !uploadInProgress && !(manualFire && runtime.m_manualFireBusy));
}

void RhsDigOutLiveControlsWidget::_actionClicked()
{
    if (_configuration.m_source == RhsDigOutSource::ManualLevel) {
        emit manualLevelRequested(!_runtime.m_manualLevelHigh);
    } else if (_configuration.m_sequenceMode ==
               RhsDigOutSequenceMode::Continuous) {
        emit continuousRequested(!_runtime.m_continuousRunning);
    } else if (_configuration.m_manualFire) {
        emit fireRequested();
    }
}
