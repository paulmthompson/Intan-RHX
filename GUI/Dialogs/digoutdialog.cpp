//------------------------------------------------------------------------------
// Modeless RHS digital-output configuration dialog.
//------------------------------------------------------------------------------

/**
 * @file digoutdialog.cpp
 * @ingroup RhsDigitalOut
 */

#include "digoutdialog.hpp"

#include "analogintriggerthreshold.hpp"
#include "channel.h"
#include "controllerinterface.h"
#include "digfigure.h"
#include "systemstate.h"
#include "ui_digoutdialog.h"

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

DigOutDialog::DigOutDialog(SystemState* state,
                           ControllerInterface* controllerInterface,
                           Channel* channel,
                           QWidget* parent)
    : QDialog(parent)
    , _ui(new Ui::DigOutDialog)
    , _state(state)
    , _controllerInterface(controllerInterface)
    , _channel(channel)
    , _digFigure(nullptr)
    , _draft(rhsDigOutConfigurationFrom(*channel->stimParameters))
    , _appliedBaseline(_draft)
    , _loading(false)
{
    _ui->setupUi(this);
    configureAnalogInTriggerThresholdSpinBox(_ui->analogThresholdSpinBox);
    _ui->analogThresholdSpinBox->setValue(quantizeAnalogInTriggerThresholdVolts(
        _state->triggerAnalogVoltageThreshold->getValue()));
    const QString comparatorDetailsToolTip = tr(
        "This threshold applies to every board analog input and to all amplifier "
        "stimulation and digital-output programs that use any ANALOG IN trigger. "
        "It is not the DAC-threshold mode on TTL outputs 1–8 in the Analog tab. "
        "The FPGA fires when voltage is above this level; use Trigger on Low to "
        "respond to the inverted line. Host recording, episodic triggers, and PSTH "
        "use the same TriggerAnalogThresholdVolts value when synced to hardware.");
    _ui->analogComparatorGroupBox->setToolTip(comparatorDetailsToolTip);
    _ui->analogThresholdHelpButton->setToolTip(comparatorDetailsToolTip);
    setWindowModality(Qt::NonModal);
    setAttribute(Qt::WA_DeleteOnClose, false);

    _ui->sourceComboBox->addItem(tr("Manual Level"));
    _ui->sourceComboBox->addItem(tr("Sequencer"));
    _ui->modeComboBox->addItem(tr("Single Pulse"));
    _ui->modeComboBox->addItem(tr("Finite Train"));
    _ui->modeComboBox->addItem(tr("Continuous Train"));
    for (int index = 1; index <= 16; ++index) {
        _ui->triggerSourceComboBox->addItem(tr("DIGITAL IN %1").arg(index));
    }
    for (int index = 1; index <= 8; ++index) {
        _ui->triggerSourceComboBox->addItem(tr("ANALOG IN %1").arg(index));
    }
    for (int index = 1; index <= 8; ++index) {
        _ui->triggerSourceComboBox->addItem(tr("KEYPRESS: F%1").arg(index));
    }
    _ui->triggerSourceComboBox->addItem(tr("Manual (GUI button)"));
    _ui->edgeComboBox->addItem(tr("Edge Triggered"));
    _ui->edgeComboBox->addItem(tr("Level Triggered"));
    _ui->polarityComboBox->addItem(tr("Trigger on High"));
    _ui->polarityComboBox->addItem(tr("Trigger on Low"));

    _digFigure = new DigFigure(channel->stimParameters, _ui->figurePlaceholder);
    auto* figureLayout = new QVBoxLayout(_ui->figurePlaceholder);
    figureLayout->setContentsMargins(0, 0, 0, 0);
    figureLayout->addWidget(_digFigure);

    connect(_ui->buttonBox, &QDialogButtonBox::accepted,
            this, &DigOutDialog::accept);
    connect(_ui->buttonBox, &QDialogButtonBox::rejected,
            this, &DigOutDialog::reject);
    connect(_ui->buttonBox->button(QDialogButtonBox::Apply),
            &QAbstractButton::clicked, this, [this]() { applyDraft(); });

    const auto changed = [this]() { _readWidgetsIntoDraft(); };
    connect(_ui->sourceComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this, changed);
    connect(_ui->modeComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this, changed);
    connect(_ui->enabledCheckBox, &QCheckBox::toggled, this, changed);
    connect(_ui->triggerSourceComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this, changed);
    connect(_ui->triggerSourceComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this]() { _updateAnalogInTriggerThresholdVisibility(); });
    connect(_ui->analogThresholdSpinBox, &QDoubleSpinBox::editingFinished, this,
            [this]() { _commitAnalogInTriggerThresholdFromSpinBox(); });
    connect(_ui->edgeComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this, changed);
    connect(_ui->polarityComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this, changed);
    connect(_ui->pulseCountSpinBox,
            qOverload<int>(&QSpinBox::valueChanged), this, changed);
    connect(_ui->delaySpinBox,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this, changed);
    connect(_ui->durationSpinBox,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this, changed);
    connect(_ui->periodSpinBox,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this, changed);
    connect(_ui->refractorySpinBox,
            qOverload<double>(&QDoubleSpinBox::valueChanged), this, changed);

    _loadDraftIntoWidgets();
    setWindowTitle(tr("Digital Output Parameters: %1 (%2)")
        .arg(channel->getNativeName(), channel->getCustomName()));
}

DigOutDialog::~DigOutDialog()
{
    delete _ui;
}

void DigOutDialog::activate()
{
    show();
    raise();
    activateWindow();
}

bool DigOutDialog::hasUnappliedChanges() const
{
    return !rhsDigOutConfigurationsEqual(_draft, _appliedBaseline);
}

bool DigOutDialog::applyDraft()
{
    _readWidgetsIntoDraft();
    if (isAnalogInStimTriggerSource(_ui->triggerSourceComboBox->currentIndex())) {
        _commitAnalogInTriggerThresholdFromSpinBox();
    }
    if (_thresholdOwned()) {
        QMessageBox::warning(
            this, tr("Digital Output Owned by DAC Threshold"),
            tr("Disable this output's DAC threshold mode in the Analog tab before applying Manual Level or Sequencer control."));
        return false;
    }
    if (const auto error =
            _controllerInterface->commitRhsDigOutConfiguration(_channel, _draft)) {
        QMessageBox::warning(this, tr("Cannot Apply Digital Output"), *error);
        return false;
    }
    _appliedBaseline = _draft;
    emit configurationApplied(_channel);
    return true;
}

void DigOutDialog::discardDraft()
{
    _draft = _appliedBaseline;
    _loadDraftIntoWidgets();
}

void DigOutDialog::accept()
{
    if (applyDraft()) {
        done(QDialog::Accepted);
    }
}

void DigOutDialog::reject()
{
    discardDraft();
    done(QDialog::Rejected);
}

void DigOutDialog::_readWidgetsIntoDraft()
{
    if (_loading) {
        return;
    }
    _draft.m_source =
        static_cast<RhsDigOutSource>(_ui->sourceComboBox->currentIndex());
    _draft.m_sequenceMode =
        static_cast<RhsDigOutSequenceMode>(_ui->modeComboBox->currentIndex());
    _draft.m_enabled = _ui->enabledCheckBox->isChecked();
    const int triggerIndex = _ui->triggerSourceComboBox->currentIndex();
    _draft.m_manualFire = triggerIndex == 32;
    if (!_draft.m_manualFire) {
        _draft.m_triggerSource = triggerIndex;
    }
    _draft.m_edgeTriggered = _ui->edgeComboBox->currentIndex() == TriggerEdge;
    _draft.m_triggerOnLow = _ui->polarityComboBox->currentIndex() == TriggerLow;
    _draft.m_numberOfPulses = _ui->pulseCountSpinBox->value();
    _draft.m_postTriggerDelayUs = _ui->delaySpinBox->value();
    _draft.m_highDurationUs = _ui->durationSpinBox->value();
    _draft.m_periodUs = _ui->periodSpinBox->value();
    _draft.m_refractoryPeriodUs = _ui->refractorySpinBox->value();
    _updateWidgetsForMode();
    _updatePreview();
}

void DigOutDialog::_updateWidgetsForMode()
{
    const bool sequencer = _draft.m_source == RhsDigOutSource::Sequencer;
    const bool continuous =
        _draft.m_sequenceMode == RhsDigOutSequenceMode::Continuous;
    const bool finite =
        _draft.m_sequenceMode == RhsDigOutSequenceMode::Finite;
    _ui->enableLabel->setVisible(sequencer);
    _ui->enabledCheckBox->setVisible(sequencer);
    _ui->triggerGroupBox->setVisible(sequencer && !continuous);
    _ui->waveformGroupBox->setVisible(sequencer);
    _ui->pulseCountLabel->setVisible(finite);
    _ui->pulseCountSpinBox->setVisible(finite);
    _ui->periodLabel->setVisible(finite || continuous);
    _ui->periodSpinBox->setVisible(finite || continuous);
    const bool externalTrigger = !_draft.m_manualFire;
    _ui->edgeLabel->setVisible(externalTrigger);
    _ui->edgeComboBox->setVisible(externalTrigger);
    _ui->polarityLabel->setVisible(externalTrigger);
    _ui->polarityComboBox->setVisible(externalTrigger);

    const bool thresholdOwned = _thresholdOwned();
    _ui->thresholdOwnershipLabel->setVisible(thresholdOwned);
    _ui->thresholdOwnershipLabel->setText(
        tr("DAC threshold mode currently owns this pin. Disable it in the Analog tab before applying digital control."));
    _updateAnalogInTriggerThresholdVisibility();
}

void DigOutDialog::_updateAnalogInTriggerThresholdVisibility()
{
    const int triggerIndex = _ui->triggerSourceComboBox->currentIndex();
    const bool analogTrigger = isAnalogInStimTriggerSource(triggerIndex);
    const bool sequencerVisible = _ui->triggerGroupBox->isVisible();
    const bool show = sequencerVisible && analogTrigger && !_draft.m_manualFire;
    _ui->analogComparatorGroupBox->setVisible(show);
    _ui->analogThresholdLabel->setEnabled(show);
    _ui->analogThresholdSpinBox->setEnabled(show);
    _ui->analogThresholdSummaryLabel->setEnabled(show);
    _ui->analogThresholdHelpButton->setEnabled(show);
    if (show) {
        _ui->analogThresholdSpinBox->setValue(quantizeAnalogInTriggerThresholdVolts(
            _state->triggerAnalogVoltageThreshold->getValue()));
    }
}

void DigOutDialog::_commitAnalogInTriggerThresholdFromSpinBox()
{
    const double quantized = quantizeAnalogInTriggerThresholdVolts(
        _ui->analogThresholdSpinBox->value());
    _ui->analogThresholdSpinBox->setValue(quantized);
    _state->triggerAnalogVoltageThreshold->setValue(quantized);
}

void DigOutDialog::_loadDraftIntoWidgets()
{
    _loading = true;
    _ui->sourceComboBox->setCurrentIndex(static_cast<int>(_draft.m_source));
    _ui->modeComboBox->setCurrentIndex(static_cast<int>(_draft.m_sequenceMode));
    _ui->enabledCheckBox->setChecked(_draft.m_enabled);
    _ui->triggerSourceComboBox->setCurrentIndex(
        _draft.m_manualFire ? 32 : _draft.m_triggerSource);
    _ui->edgeComboBox->setCurrentIndex(
        _draft.m_edgeTriggered ? TriggerEdge : TriggerLevel);
    _ui->polarityComboBox->setCurrentIndex(
        _draft.m_triggerOnLow ? TriggerLow : TriggerHigh);
    _ui->pulseCountSpinBox->setValue(_draft.m_numberOfPulses);
    _ui->delaySpinBox->setValue(_draft.m_postTriggerDelayUs);
    _ui->durationSpinBox->setValue(_draft.m_highDurationUs);
    _ui->periodSpinBox->setValue(_draft.m_periodUs);
    _ui->refractorySpinBox->setValue(_draft.m_refractoryPeriodUs);
    _loading = false;
    _updateWidgetsForMode();
    _updatePreview();
}

void DigOutDialog::_updatePreview()
{
    RhsDigOutQuantizedTiming timing;
    const auto error = validateAndQuantizeRhsDigOutConfiguration(
        _draft, _state->sampleRate->getNumericValue(), timing);
    if (error) {
        _ui->achievedLabel->setText(tr("Achieved: %1").arg(*error));
    } else if (_draft.m_source == RhsDigOutSource::Sequencer &&
               _draft.m_sequenceMode != RhsDigOutSequenceMode::Single) {
        _ui->achievedLabel->setText(
            tr("Achieved: %1 Hz, %2% duty")
                .arg(timing.m_achievedFrequencyHz, 0, 'f', 3)
                .arg(timing.m_achievedDutyPercent, 0, 'f', 2));
    } else {
        _ui->achievedLabel->setText(tr("Achieved: --"));
    }
    _digFigure->updateSource(static_cast<int>(_draft.m_source));
    _digFigure->updatePulseOrTrain(static_cast<int>(_draft.m_sequenceMode));
    _digFigure->update();
}

bool DigOutDialog::_thresholdOwned() const
{
    const int channelIndex = _channel->getNativeChannelNumber();
    if (channelIndex < 0 || channelIndex >= 8) {
        return false;
    }
    BooleanItem* thresholdModes[8] = {
        _state->analogOut1ThresholdEnabled, _state->analogOut2ThresholdEnabled,
        _state->analogOut3ThresholdEnabled, _state->analogOut4ThresholdEnabled,
        _state->analogOut5ThresholdEnabled, _state->analogOut6ThresholdEnabled,
        _state->analogOut7ThresholdEnabled, _state->analogOut8ThresholdEnabled
    };
    return thresholdModes[channelIndex]->getValue();
}
