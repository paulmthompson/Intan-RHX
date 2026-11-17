/**
 * @file pulseresistancedialog.cpp
 * @brief Pulse resistance dialog.
 * @ingroup PulseResponse
*/

#include "boardadcscaling.hpp"
#include "channel.h"
#include "pulseresistancedialog.h"
#include "PulseResponse/pulseresponseplot.h"
#include "signalsources.h"
#include "waveformfifo.h"

#include <QtWidgets>

#include <cmath>
#include <optional>

PulseResistanceDialog::PulseResistanceDialog(SystemState* state_, QWidget* parent) :
    QDialog(parent),
    state(state_),
    _watchedChannelNativeName(QString()),
    _configFingerprint(QString()),
    _voltageQuantizationLsbVolts(pulseElectrodeResistance::DcAmplifierLsbVolts),
    _captureAcsfOnNextPulse(false),
    _captureBrainOnNextPulse(false)
{
    connect(state, SIGNAL(stateChanged()), this, SLOT(updateFromState()));

    modeComboBox = new QComboBox(this);
    state->pulseResistanceMode->setupComboBox(modeComboBox);
    connect(modeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setMode(int)));

    rhsPage = new QWidget(this);
    channelName = new QLabel(QString(), rhsPage);
    lockScopeCheckbox = new QCheckBox(tr("Lock Plot to Selected"), rhsPage);
    connect(lockScopeCheckbox, SIGNAL(clicked()), this, SLOT(toggleLock()));
    setToSelectedButton = new QPushButton(tr("Set to Selected"), rhsPage);
    connect(setToSelectedButton, SIGNAL(clicked()), this, SLOT(setToSelected()));
    QHBoxLayout* rhsTopRow = new QHBoxLayout;
    rhsTopRow->addWidget(channelName);
    rhsTopRow->addStretch(1);
    rhsTopRow->addWidget(lockScopeCheckbox);
    QHBoxLayout* rhsButtonRow = new QHBoxLayout;
    rhsButtonRow->addWidget(setToSelectedButton);
    rhsButtonRow->addStretch(1);
    QVBoxLayout* rhsLayout = new QVBoxLayout(rhsPage);
    rhsLayout->addLayout(rhsTopRow);
    rhsLayout->addLayout(rhsButtonRow);

    externalPage = new QWidget(this);
    externalDcAdcComboBox = new QComboBox(externalPage);
    connect(externalDcAdcComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setExternalDcAdc(int)));
    externalTraceAdcComboBox = new QComboBox(externalPage);
    state->pulseResistanceExternalTraceAdc->setupComboBox(externalTraceAdcComboBox);
    connect(externalTraceAdcComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setExternalTraceAdc(int)));
    externalCurrentSpinBox = new QDoubleSpinBox(externalPage);
    state->pulseResistanceExternalCurrentNanoAmps->setupSpinBox(externalCurrentSpinBox);
    externalCurrentSpinBox->setDecimals(1);
    externalCurrentSpinBox->setSingleStep(0.1);
    connect(externalCurrentSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setExternalCurrentNanoAmps(double)));
    externalTtlMarkerComboBox = new QComboBox(externalPage);
    state->pulseResistanceExternalTtlMarker->setupComboBox(externalTtlMarkerComboBox);
    connect(externalTtlMarkerComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setExternalTtlMarker(int)));
    externalTtlActiveHighCheckbox = new QCheckBox(tr("Stim active when TTL high"), externalPage);
    externalTtlActiveHighCheckbox->setChecked(state->pulseResistanceExternalTtlActiveHigh->getValue());
    connect(externalTtlActiveHighCheckbox, SIGNAL(toggled(bool)), this, SLOT(setExternalTtlActiveHigh(bool)));

    QFormLayout* externalForm = new QFormLayout;
    externalForm->addRow(tr("Resistance ADC:"), externalDcAdcComboBox);
    externalForm->addRow(tr("Trace ADC (optional):"), externalTraceAdcComboBox);
    externalForm->addRow(tr("Stimulus current (nA):"), externalCurrentSpinBox);
    externalForm->addRow(tr("Stim TTL marker:"), externalTtlMarkerComboBox);
    externalForm->addRow(QString(), externalTtlActiveHighCheckbox);
    QVBoxLayout* externalLayout = new QVBoxLayout(externalPage);
    externalLayout->addLayout(externalForm);

    modeStack = new QStackedWidget(this);
    modeStack->addWidget(rhsPage);
    modeStack->addWidget(externalPage);

    captureAcsfButton = new QPushButton(tr("Capture ACSF"), this);
    connect(captureAcsfButton, SIGNAL(clicked()), this, SLOT(captureAcsfReference()));

    captureBrainButton = new QPushButton(tr("Capture Brain"), this);
    connect(captureBrainButton, SIGNAL(clicked()), this, SLOT(captureBrainReference()));

    clearReferencesButton = new QPushButton(tr("Clear References"), this);
    connect(clearReferencesButton, SIGNAL(clicked()), this, SLOT(clearReferences()));

    clearHistoryButton = new QPushButton(tr("Clear History"), this);
    connect(clearHistoryButton, SIGNAL(clicked()), this, SLOT(clearHistory()));

    warningThresholdSpinBox = new QDoubleSpinBox(this);
    state->pulseResistanceWarningThresholdPercent->setupSpinBox(warningThresholdSpinBox);
    warningThresholdSpinBox->setDecimals(1);
    warningThresholdSpinBox->setSingleStep(5.0);
    warningThresholdSpinBox->setPrefix(tr("> +"));
    warningThresholdSpinBox->setSuffix(tr("%"));
    connect(warningThresholdSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setWarningThresholdPercent(double)));

    warningColorButton = new QPushButton(tr("Alert Color..."), this);
    connect(warningColorButton, SIGNAL(clicked()), this, SLOT(chooseWarningColor()));

    resistanceValueLabel = new QLabel(tr("R: —"), this);
    QFont valueFont = resistanceValueLabel->font();
    valueFont.setPointSize(valueFont.pointSize() + 4);
    valueFont.setBold(true);
    resistanceValueLabel->setFont(valueFont);

    acsfReferenceLabel = new QLabel(tr("ACSF Ref: —"), this);
    acsfDiffLabel = new QLabel(tr("Δ vs ACSF: —"), this);
    brainReferenceLabel = new QLabel(tr("Brain Ref: —"), this);
    brainDiffLabel = new QLabel(tr("Δ vs Brain: —"), this);

    deltaVoltageLabel = new QLabel(tr("ΔV: —"), this);
    currentLabel = new QLabel(tr("I: —"), this);
    statusLabel = new QLabel(tr("Waiting for stim pulse on watched channel."), this);
    statusLabel->setWordWrap(true);

    helpLabel = new QLabel(QString(), this);
    helpLabel->setWordWrap(true);

    traceTimeSpanComboBox = new QComboBox(this);
    state->pulseResistanceTraceTimeMs->setupComboBox(traceTimeSpanComboBox);
    connect(traceTimeSpanComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setTraceTimeSpan(int)));

    traceYScaleAcLabel = new QLabel(tr("AC Y:"), this);
    traceYScaleAcComboBox = new QComboBox(this);
    state->pulseResistanceTraceYScaleAc->setupComboBox(traceYScaleAcComboBox);
    connect(traceYScaleAcComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setTraceYScaleUpper(int)));

    traceYScaleDcLabel = new QLabel(tr("DC Y:"), this);
    traceYScaleDcComboBox = new QComboBox(this);
    state->pulseResistanceTraceYScaleDc->setupComboBox(traceYScaleDcComboBox);
    connect(traceYScaleDcComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setTraceYScaleDc(int)));

    responsePlot = new PulseResponsePlot(state, this);

    historyTable = new QTableWidget(0, 7, this);
    historyTable->setHorizontalHeaderLabels(
        QStringList() << tr("Timestamp") << tr("R") << tr("% vs ACSF") << tr("% vs Brain")
                      << tr("ΔV (mV)") << tr("I (µA)") << tr("Status"));
    historyTable->horizontalHeader()->setStretchLastSection(true);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QHBoxLayout* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(captureAcsfButton);
    buttonRow->addWidget(captureBrainButton);
    buttonRow->addWidget(clearReferencesButton);
    buttonRow->addWidget(clearHistoryButton);
    buttonRow->addStretch(1);

    QHBoxLayout* alertControlsRow = new QHBoxLayout;
    alertControlsRow->addWidget(new QLabel(tr("Alert increase:"), this));
    alertControlsRow->addWidget(warningThresholdSpinBox);
    alertControlsRow->addWidget(warningColorButton);
    alertControlsRow->addStretch(1);

    QHBoxLayout* acsfReferenceRow = new QHBoxLayout;
    acsfReferenceRow->addWidget(acsfReferenceLabel);
    acsfReferenceRow->addSpacing(16);
    acsfReferenceRow->addWidget(acsfDiffLabel);
    acsfReferenceRow->addStretch(1);

    QHBoxLayout* brainReferenceRow = new QHBoxLayout;
    brainReferenceRow->addWidget(brainReferenceLabel);
    brainReferenceRow->addSpacing(16);
    brainReferenceRow->addWidget(brainDiffLabel);
    brainReferenceRow->addStretch(1);

    QHBoxLayout* traceControlsRow = new QHBoxLayout;
    traceControlsRow->addWidget(new QLabel(tr("Trace time:"), this));
    traceControlsRow->addWidget(traceTimeSpanComboBox);
    traceControlsRow->addWidget(traceYScaleAcLabel);
    traceControlsRow->addWidget(traceYScaleAcComboBox);
    traceControlsRow->addWidget(traceYScaleDcLabel);
    traceControlsRow->addWidget(traceYScaleDcComboBox);
    traceControlsRow->addStretch(1);

    modeLabel = new QLabel(tr("Mode:"), this);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(modeLabel);
    mainLayout->addWidget(modeComboBox);
    mainLayout->addWidget(modeStack);
    mainLayout->addLayout(buttonRow);
    mainLayout->addLayout(alertControlsRow);
    mainLayout->addWidget(resistanceValueLabel);
    mainLayout->addLayout(acsfReferenceRow);
    mainLayout->addLayout(brainReferenceRow);
    mainLayout->addWidget(deltaVoltageLabel);
    mainLayout->addWidget(currentLabel);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(helpLabel);
    mainLayout->addLayout(traceControlsRow);
    mainLayout->addWidget(responsePlot);
    mainLayout->addWidget(historyTable);
    setLayout(mainLayout);

    resize(640, 820);
    refreshExternalAdcComboBoxes();
    updateFromState();
}

void PulseResistanceDialog::updateForRun()
{
}

void PulseResistanceDialog::updateForLoad()
{
    updateForRun();
}

void PulseResistanceDialog::updateForStop()
{
}

void PulseResistanceDialog::updateForChangeHeadstages()
{
    resetTrackerForChannelChange();
    refreshExternalAdcComboBoxes();
    updateFromState();
}

void PulseResistanceDialog::activate()
{
    updateFromState();
    show();
    raise();
    activateWindow();
}

QStringList PulseResistanceDialog::boardAdcChannelNames() const
{
    QStringList names;
    for (int groupIndex = 0; groupIndex < state->signalSources->numGroups(); ++groupIndex) {
        const SignalGroup* group = state->signalSources->groupByIndex(groupIndex);
        for (int channelIndex = 0; channelIndex < group->numChannels(); ++channelIndex) {
            const Channel* channel = group->channelByIndex(channelIndex);
            if (channel->getSignalType() == BoardAdcSignal) {
                names.append(channel->getNativeName());
            }
        }
    }
    names.sort();
    return names;
}

void PulseResistanceDialog::refreshExternalAdcComboBoxes()
{
    const QStringList adcNames = boardAdcChannelNames();
    const QString currentDc = state->pulseResistanceExternalDcAdc->getValueString();

    externalDcAdcComboBox->blockSignals(true);
    externalDcAdcComboBox->clear();
    for (const QString& name : adcNames) {
        externalDcAdcComboBox->addItem(name);
    }
    int dcIndex = externalDcAdcComboBox->findText(currentDc);
    if (dcIndex < 0 && !adcNames.isEmpty()) {
        dcIndex = 0;
        state->pulseResistanceExternalDcAdc->setValue(adcNames.first());
    }
    if (dcIndex >= 0) {
        externalDcAdcComboBox->setCurrentIndex(dcIndex);
    }
    externalDcAdcComboBox->blockSignals(false);
}

void PulseResistanceDialog::setMode(int index)
{
    state->pulseResistanceMode->setIndex(index);
}

void PulseResistanceDialog::setTraceYScaleUpper(int index)
{
    if (state->pulseResistanceUsesExternalAdcTtl()) {
        state->pulseResistanceExternalTraceYScaleVolts->setIndex(index);
    } else {
        state->pulseResistanceTraceYScaleAc->setIndex(index);
    }
}

void PulseResistanceDialog::setExternalDcAdc(int index)
{
    if (index < 0 || index >= externalDcAdcComboBox->count()) {
        return;
    }
    state->pulseResistanceExternalDcAdc->setValue(externalDcAdcComboBox->itemText(index));
}

void PulseResistanceDialog::setExternalTraceAdc(int index)
{
    state->pulseResistanceExternalTraceAdc->setIndex(index);
}

void PulseResistanceDialog::setExternalCurrentNanoAmps(double value)
{
    state->pulseResistanceExternalCurrentNanoAmps->setValue(value);
}

void PulseResistanceDialog::setExternalTtlMarker(int index)
{
    state->pulseResistanceExternalTtlMarker->setIndex(index);
}

void PulseResistanceDialog::setExternalTtlActiveHigh(bool activeHigh)
{
    state->pulseResistanceExternalTtlActiveHigh->setValue(activeHigh);
}

QString PulseResistanceDialog::buildConfigFingerprint() const
{
    if (state->pulseResistanceUsesExternalAdcTtl()) {
        QString dcGainString = QStringLiteral("1");
        if (Channel* dcChannel =
                state->signalSources->channelByName(state->pulseResistanceExternalDcAdc->getValueString())) {
            dcGainString = QString::number(dcChannel->getExternalAmplifierGain());
        }
        QString traceGainString = QStringLiteral("1");
        const QString traceName = state->pulseResistanceExternalTraceAdc->getValueString();
        if (traceName != QStringLiteral("None")) {
            if (Channel* traceChannel = state->signalSources->channelByName(traceName)) {
                traceGainString = QString::number(traceChannel->getExternalAmplifierGain());
            }
        }
        return QStringLiteral("ext|") + state->pulseResistanceExternalDcAdc->getValueString() + QLatin1Char('|')
               + traceName + QLatin1Char('|') + dcGainString + QLatin1Char('|') + traceGainString + QLatin1Char('|')
               + QString::number(state->pulseResistanceExternalCurrentNanoAmps->getValue()) + QLatin1Char('|')
               + state->pulseResistanceExternalTtlMarker->getValueString() + QLatin1Char('|')
               + QString::number(state->pulseResistanceExternalTtlActiveHigh->getValue());
    }
    return QStringLiteral("rhs|") + state->pulseResistanceChannel->getValue();
}

void PulseResistanceDialog::syncConfigFingerprint()
{
    const QString fingerprint = buildConfigFingerprint();
    if (fingerprint != _configFingerprint) {
        _configFingerprint = fingerprint;
        resetTrackerForChannelChange();
    }
}

void PulseResistanceDialog::updateHelpText()
{
    if (state->pulseResistanceUsesExternalAdcTtl()) {
        helpLabel->setText(
            tr("Estimates pulse resistance (ΔV/I) from a board analog input during marked stimulation. "
               "Set external amplifier gain on each analog input in the control panel (Amp gain). "
               "Enter the stimulus current you apply externally. Enable analog inputs in the acquisition stream. "
               "Pick a digital input (external sync) or digital output (when Intan drives the stimulator via Dig Out). "
               "This is not AC impedance from the Impedance tab."));
    } else {
        helpLabel->setText(
            tr("Estimates DC pulse resistance (ΔV/I) from the DC amplifier during stimulation. "
               "This is not the AC impedance from the Impedance tab (Zcheck). "
               "Peri-pulse traces show AC (high-pass) and DC (10-bit, ±10 V) with stim onset at the center."));
    }
}

void PulseResistanceDialog::updateModeUi()
{
    const bool rhsStimModeAvailable = state->pulseResistanceSupportsRhsStimMode();
    modeLabel->setVisible(rhsStimModeAvailable);
    modeComboBox->setVisible(rhsStimModeAvailable);

    const bool external = state->pulseResistanceUsesExternalAdcTtl();
    modeStack->setCurrentWidget(external ? externalPage : rhsPage);

    traceYScaleAcLabel->setText(external ? tr("Trace Y:") : tr("AC Y:"));
    traceYScaleDcLabel->setText(external ? tr("Resistance Y:") : tr("DC Y:"));

    traceYScaleAcComboBox->blockSignals(true);
    if (external) {
        state->pulseResistanceExternalTraceYScaleVolts->setupComboBox(traceYScaleAcComboBox);
        traceYScaleAcComboBox->setCurrentIndex(state->pulseResistanceExternalTraceYScaleVolts->getIndex());
    } else {
        state->pulseResistanceTraceYScaleAc->setupComboBox(traceYScaleAcComboBox);
        traceYScaleAcComboBox->setCurrentIndex(state->pulseResistanceTraceYScaleAc->getIndex());
    }
    traceYScaleAcComboBox->blockSignals(false);

    updateHelpText();
}

void PulseResistanceDialog::updateFromState()
{
    state->ensurePulseResistanceModeCompatible();
    syncConfigFingerprint();

    if (modeComboBox->currentIndex() != state->pulseResistanceMode->getIndex()) {
        modeComboBox->blockSignals(true);
        modeComboBox->setCurrentIndex(state->pulseResistanceMode->getIndex());
        modeComboBox->blockSignals(false);
    }
    updateModeUi();

    const bool external = state->pulseResistanceUsesExternalAdcTtl();
    const bool rhsStimModeAvailable = state->pulseResistanceSupportsRhsStimMode();

    if (!external) {
        QString channelNativeName = state->signalSources->singleSelectedAmplifierChannelName();
        if (channelNativeName != state->pulseResistanceChannel->getValue()) {
            if (!channelNativeName.isEmpty()) {
                setToSelectedButton->setEnabled(rhsStimModeAvailable);
                if (lockScopeCheckbox->isChecked()) {
                    changeCurrentChannel(channelNativeName);
                }
            } else {
                setToSelectedButton->setEnabled(false);
            }
        }

        const QString watched = state->pulseResistanceChannel->getValue();
        if (watched != _watchedChannelNativeName) {
            _watchedChannelNativeName = watched;
            resetTrackerForChannelChange();
        }

        QString displayName = state->signalSources->getNativeAndCustomNames(watched);
        if (displayName.isEmpty()) {
            displayName = tr("N/A");
        }
        if (channelName->text() != displayName) {
            channelName->setText(displayName);
        }
        lockScopeCheckbox->setEnabled(rhsStimModeAvailable);
        setToSelectedButton->setEnabled(rhsStimModeAvailable
                                        && !state->signalSources->singleSelectedAmplifierChannelName().isEmpty());

        if (!rhsStimModeAvailable) {
            statusLabel->setText(tr("RHS stim channel mode requires an RHS stim/record controller."));
        }
    } else {
        refreshExternalAdcComboBoxes();

        if (externalTraceAdcComboBox->currentIndex() != state->pulseResistanceExternalTraceAdc->getIndex()) {
            externalTraceAdcComboBox->blockSignals(true);
            externalTraceAdcComboBox->setCurrentIndex(state->pulseResistanceExternalTraceAdc->getIndex());
            externalTraceAdcComboBox->blockSignals(false);
        }
        if (externalTtlMarkerComboBox->currentIndex() != state->pulseResistanceExternalTtlMarker->getIndex()) {
            externalTtlMarkerComboBox->blockSignals(true);
            externalTtlMarkerComboBox->setCurrentIndex(state->pulseResistanceExternalTtlMarker->getIndex());
            externalTtlMarkerComboBox->blockSignals(false);
        }
        if (std::abs(externalCurrentSpinBox->value() - state->pulseResistanceExternalCurrentNanoAmps->getValue())
            > 1.0e-9) {
            externalCurrentSpinBox->blockSignals(true);
            externalCurrentSpinBox->setValue(state->pulseResistanceExternalCurrentNanoAmps->getValue());
            externalCurrentSpinBox->blockSignals(false);
        }
        if (externalTtlActiveHighCheckbox->isChecked() != state->pulseResistanceExternalTtlActiveHigh->getValue()) {
            externalTtlActiveHighCheckbox->blockSignals(true);
            externalTtlActiveHighCheckbox->setChecked(state->pulseResistanceExternalTtlActiveHigh->getValue());
            externalTtlActiveHighCheckbox->blockSignals(false);
        }
    }

    if (traceTimeSpanComboBox->currentIndex() != state->pulseResistanceTraceTimeMs->getIndex()) {
        traceTimeSpanComboBox->setCurrentIndex(state->pulseResistanceTraceTimeMs->getIndex());
    }
    if (external) {
        if (traceYScaleAcComboBox->currentIndex() != state->pulseResistanceExternalTraceYScaleVolts->getIndex()) {
            traceYScaleAcComboBox->blockSignals(true);
            traceYScaleAcComboBox->setCurrentIndex(state->pulseResistanceExternalTraceYScaleVolts->getIndex());
            traceYScaleAcComboBox->blockSignals(false);
        }
    } else if (traceYScaleAcComboBox->currentIndex() != state->pulseResistanceTraceYScaleAc->getIndex()) {
        traceYScaleAcComboBox->blockSignals(true);
        traceYScaleAcComboBox->setCurrentIndex(state->pulseResistanceTraceYScaleAc->getIndex());
        traceYScaleAcComboBox->blockSignals(false);
    }
    if (traceYScaleDcComboBox->currentIndex() != state->pulseResistanceTraceYScaleDc->getIndex()) {
        traceYScaleDcComboBox->setCurrentIndex(state->pulseResistanceTraceYScaleDc->getIndex());
    }

    if (std::abs(warningThresholdSpinBox->value() - state->pulseResistanceWarningThresholdPercent->getValue())
        > 1.0e-9) {
        warningThresholdSpinBox->blockSignals(true);
        warningThresholdSpinBox->setValue(state->pulseResistanceWarningThresholdPercent->getValue());
        warningThresholdSpinBox->blockSignals(false);
    }
    updateWarningColorButtonStyle();
    refreshReferenceDisplays();

    updateTitle();
}

void PulseResistanceDialog::setToSelected()
{
    QString channelNativeName = state->signalSources->singleSelectedAmplifierChannelName();
    if (channelNativeName.isEmpty()) {
        qDebug() << "PulseResistanceDialog: setToSelected without single channel selection";
        return;
    }
    changeCurrentChannel(channelNativeName);
}

void PulseResistanceDialog::captureAcsfReference()
{
    if (_latestValidResistanceOhms.has_value()) {
        _acsfReferenceOhms = *_latestValidResistanceOhms;
        _captureAcsfOnNextPulse = false;
        statusLabel->setText(tr("Captured ACSF reference: %1").arg(formatResistanceOhms(*_acsfReferenceOhms)));
    } else {
        _captureAcsfOnNextPulse = true;
        statusLabel->setText(tr("ACSF reference armed — will capture on next valid pulse."));
    }
    refreshReferenceDisplays();
}

void PulseResistanceDialog::captureBrainReference()
{
    if (_latestValidResistanceOhms.has_value()) {
        _brainReferenceOhms = *_latestValidResistanceOhms;
        _captureBrainOnNextPulse = false;
        statusLabel->setText(tr("Captured brain reference: %1").arg(formatResistanceOhms(*_brainReferenceOhms)));
    } else {
        _captureBrainOnNextPulse = true;
        statusLabel->setText(tr("Brain reference armed — will capture on next valid pulse."));
    }
    refreshReferenceDisplays();
}

void PulseResistanceDialog::clearReferences()
{
    _acsfReferenceOhms.reset();
    _brainReferenceOhms.reset();
    _captureAcsfOnNextPulse = false;
    _captureBrainOnNextPulse = false;
    refreshReferenceDisplays();
}

void PulseResistanceDialog::setWarningThresholdPercent(double value)
{
    state->pulseResistanceWarningThresholdPercent->setValue(value);
    refreshReferenceDisplays();
}

void PulseResistanceDialog::chooseWarningColor()
{
    const QColor chosen = QColorDialog::getColor(warningColor(), this, tr("Select High Impedance Alert Color"));
    if (chosen.isValid()) {
        state->pulseResistanceWarningColor->setValue(chosen.name());
        updateWarningColorButtonStyle();
        refreshReferenceDisplays();
    }
}

QColor PulseResistanceDialog::warningColor() const
{
    const QColor configured(state->pulseResistanceWarningColor->getValueString());
    return configured.isValid() ? configured : QColor(Qt::red);
}

void PulseResistanceDialog::updateWarningColorButtonStyle()
{
    QPixmap swatch(12, 12);
    swatch.fill(warningColor());
    warningColorButton->setIcon(QIcon(swatch));
}

void PulseResistanceDialog::clearHistory()
{
    historyTable->setRowCount(0);
    _latestValidResistanceOhms.reset();
    resistanceValueLabel->setText(tr("R: —"));
    resistanceValueLabel->setStyleSheet(QString());
    deltaVoltageLabel->setText(tr("ΔV: —"));
    currentLabel->setText(tr("I: —"));
    statusLabel->setText(state->pulseResistanceUsesExternalAdcTtl()
                             ? tr("Waiting for TTL-marked stim pulse.")
                             : tr("Waiting for stim pulse on watched channel."));
    resetTrackerForChannelChange();
    responsePlot->clearSnippet();
    refreshReferenceDisplays();
}

void PulseResistanceDialog::resetTrackerForChannelChange()
{
    _trackerState = pulseElectrodeResistance::PulseResistanceTrackerState();
    _responseCaptureState = pulseElectrodeResistance::PulseResponseCaptureState();
    _latestValidResistanceOhms.reset();
    responsePlot->clearSnippet();
    refreshReferenceDisplays();
}

void PulseResistanceDialog::updateTitle()
{
    if (state->pulseResistanceUsesExternalAdcTtl()) {
        setWindowTitle(tr("Pulse Resistance") + " ("
                       + state->pulseResistanceExternalDcAdc->getValueString() + ")");
    } else {
        setWindowTitle(tr("Pulse Resistance") + " (" + state->pulseResistanceChannel->getValue() + ")");
    }
}

QString PulseResistanceDialog::formatResistanceOhms(double ohms)
{
    if (ohms >= 1.0e6) {
        return QString::number(ohms / 1.0e6, 'f', 2) + tr(" MΩ");
    }
    if (ohms >= 1.0e3) {
        return QString::number(ohms / 1.0e3, 'f', 2) + tr(" kΩ");
    }
    return QString::number(ohms, 'f', 1) + tr(" Ω");
}

QString PulseResistanceDialog::formatPercentDifference(double percentDiff)
{
    const double normalized = std::abs(percentDiff) < 0.05 ? 0.0 : percentDiff;
    const QString signPrefix = normalized >= 0.0 ? QStringLiteral("+") : QString();
    return signPrefix + QString::number(normalized, 'f', 1) + tr("%");
}

QString PulseResistanceDialog::formatResistanceOhmsWithUncertainty(double ohms, double deltaVolts) const
{
    QString formatted = formatResistanceOhms(ohms);
    const double uncertaintyPercent =
        pulseElectrodeResistance::resistanceQuantizationUncertaintyPercent(deltaVolts, _voltageQuantizationLsbVolts);
    if (!std::isfinite(uncertaintyPercent)) {
        return formatted;
    }
    const QString percentText =
        uncertaintyPercent >= 10.0 ? QString::number(uncertaintyPercent, 'f', 0)
                                   : QString::number(uncertaintyPercent, 'f', 1);
    return formatted + tr(" +/- %1%").arg(percentText);
}

QString PulseResistanceDialog::failureReasonText(pulseElectrodeResistance::PulseResistanceFailure failure)
{
    using pulseElectrodeResistance::PulseResistanceFailure;
    switch (failure) {
    case PulseResistanceFailure::None:
        return tr("OK");
    case PulseResistanceFailure::Compliance:
        return tr("Invalid (compliance)");
    case PulseResistanceFailure::InsufficientBaseline:
        return tr("Invalid (baseline)");
    case PulseResistanceFailure::InsufficientPlateau:
        return tr("Invalid (pulse too short)");
    case PulseResistanceFailure::CurrentTooSmall:
        return tr("Invalid (zero or negative current)");
    }
    return tr("Invalid");
}

void PulseResistanceDialog::refreshReferenceDisplays()
{
    const double thresholdPercent = state->pulseResistanceWarningThresholdPercent->getValue();
    const QColor alertColor = warningColor();
    const QString alertStyle = QStringLiteral("QLabel { color: %1; font-weight: bold; }").arg(alertColor.name());

    bool acsfExceeded = false;
    if (_acsfReferenceOhms.has_value()) {
        acsfReferenceLabel->setText(tr("ACSF Ref: ") + formatResistanceOhms(*_acsfReferenceOhms));
        if (_latestValidResistanceOhms.has_value()) {
            const std::optional<double> diff =
                pulseElectrodeResistance::relativePercentDifference(*_latestValidResistanceOhms, *_acsfReferenceOhms);
            if (diff.has_value()) {
                acsfDiffLabel->setText(tr("Δ vs ACSF: ") + formatPercentDifference(*diff));
                acsfExceeded = pulseElectrodeResistance::isRelativeIncreaseAboveThreshold(*diff, thresholdPercent);
            } else {
                acsfDiffLabel->setText(tr("Δ vs ACSF: —"));
            }
        } else {
            acsfDiffLabel->setText(tr("Δ vs ACSF: —"));
        }
    } else if (_captureAcsfOnNextPulse) {
        acsfReferenceLabel->setText(tr("ACSF Ref: (waiting for pulse...)"));
        acsfDiffLabel->setText(tr("Δ vs ACSF: —"));
    } else {
        acsfReferenceLabel->setText(tr("ACSF Ref: —"));
        acsfDiffLabel->setText(tr("Δ vs ACSF: —"));
    }
    acsfDiffLabel->setStyleSheet(acsfExceeded ? alertStyle : QString());

    bool brainExceeded = false;
    if (_brainReferenceOhms.has_value()) {
        brainReferenceLabel->setText(tr("Brain Ref: ") + formatResistanceOhms(*_brainReferenceOhms));
        if (_latestValidResistanceOhms.has_value()) {
            const std::optional<double> diff =
                pulseElectrodeResistance::relativePercentDifference(*_latestValidResistanceOhms, *_brainReferenceOhms);
            if (diff.has_value()) {
                brainDiffLabel->setText(tr("Δ vs Brain: ") + formatPercentDifference(*diff));
                brainExceeded = pulseElectrodeResistance::isRelativeIncreaseAboveThreshold(*diff, thresholdPercent);
            } else {
                brainDiffLabel->setText(tr("Δ vs Brain: —"));
            }
        } else {
            brainDiffLabel->setText(tr("Δ vs Brain: —"));
        }
    } else if (_captureBrainOnNextPulse) {
        brainReferenceLabel->setText(tr("Brain Ref: (waiting for pulse...)"));
        brainDiffLabel->setText(tr("Δ vs Brain: —"));
    } else {
        brainReferenceLabel->setText(tr("Brain Ref: —"));
        brainDiffLabel->setText(tr("Δ vs Brain: —"));
    }
    brainDiffLabel->setStyleSheet(brainExceeded ? alertStyle : QString());

    if (_latestValidResistanceOhms.has_value() && (acsfExceeded || brainExceeded)) {
        resistanceValueLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; }").arg(alertColor.name()));
    } else {
        resistanceValueLabel->setStyleSheet(QString());
    }

    for (int row = 0; row < historyTable->rowCount(); ++row) {
        updateHistoryRowReferenceColumns(row);
    }
}

void PulseResistanceDialog::updateHistoryRowReferenceColumns(int row)
{
    QTableWidgetItem* rItem = historyTable->item(row, 1);
    QTableWidgetItem* acsfItem = historyTable->item(row, 2);
    if (!acsfItem) {
        acsfItem = new QTableWidgetItem();
        historyTable->setItem(row, 2, acsfItem);
    }
    QTableWidgetItem* brainItem = historyTable->item(row, 3);
    if (!brainItem) {
        brainItem = new QTableWidgetItem();
        historyTable->setItem(row, 3, brainItem);
    }

    const QVariant resistanceVar = rItem ? rItem->data(Qt::UserRole) : QVariant();
    if (!resistanceVar.isValid()) {
        acsfItem->setText(QString());
        acsfItem->setData(Qt::ForegroundRole, QVariant());
        brainItem->setText(QString());
        brainItem->setData(Qt::ForegroundRole, QVariant());
        return;
    }

    const double resistanceOhms = resistanceVar.toDouble();
    const double thresholdPercent = state->pulseResistanceWarningThresholdPercent->getValue();
    const QColor alertColor = warningColor();

    auto applyDiffItem = [&](QTableWidgetItem* item, const std::optional<double>& referenceOhms) {
        const std::optional<double> diff =
            referenceOhms.has_value()
                ? pulseElectrodeResistance::relativePercentDifference(resistanceOhms, *referenceOhms)
                : std::nullopt;
        if (!diff.has_value()) {
            item->setText(QString());
            item->setData(Qt::ForegroundRole, QVariant());
            QFont f = item->font();
            f.setBold(false);
            item->setFont(f);
            return;
        }
        item->setText(formatPercentDifference(*diff));
        const bool exceeded = pulseElectrodeResistance::isRelativeIncreaseAboveThreshold(*diff, thresholdPercent);
        if (exceeded) {
            item->setForeground(QBrush(alertColor));
        } else {
            item->setData(Qt::ForegroundRole, QVariant());
        }
        QFont f = item->font();
        f.setBold(exceeded);
        item->setFont(f);
    };

    applyDiffItem(acsfItem, _acsfReferenceOhms);
    applyDiffItem(brainItem, _brainReferenceOhms);
}

void PulseResistanceDialog::applyResult(const pulseElectrodeResistance::PulseResistanceResult& result)
{
    QString status;
    if (result.m_valid) {
        status = result.m_complianceLimited ? tr("OK (compliance limited)") : tr("OK");
    } else {
        status = failureReasonText(result.m_failure);
    }
    statusLabel->setText(status);

    if (result.m_valid) {
        _latestValidResistanceOhms = result.m_resistanceOhms;
        if (_captureAcsfOnNextPulse) {
            _acsfReferenceOhms = result.m_resistanceOhms;
            _captureAcsfOnNextPulse = false;
        }
        if (_captureBrainOnNextPulse) {
            _brainReferenceOhms = result.m_resistanceOhms;
            _captureBrainOnNextPulse = false;
        }

        resistanceValueLabel->setText(tr("R: ")
                                      + formatResistanceOhmsWithUncertainty(result.m_resistanceOhms, result.m_deltaVolts));
        deltaVoltageLabel->setText(tr("ΔV: ") + QString::number(result.m_deltaVolts * 1000.0, 'f', 3) + tr(" mV"));
        currentLabel->setText(tr("I: ") + QString::number(result.m_currentAmps * 1.0e6, 'f', 3) + tr(" µA"));
    }

    historyTable->insertRow(0);
    historyTable->setItem(0, 0, new QTableWidgetItem(QString::number(result.m_timestamp)));
    auto* rItem = new QTableWidgetItem(result.m_valid
                                           ? formatResistanceOhmsWithUncertainty(result.m_resistanceOhms,
                                                                                 result.m_deltaVolts)
                                           : QString());
    if (result.m_valid) {
        rItem->setData(Qt::UserRole, result.m_resistanceOhms);
    }
    historyTable->setItem(0, 1, rItem);
    historyTable->setItem(0, 2, new QTableWidgetItem());
    historyTable->setItem(0, 3, new QTableWidgetItem());
    historyTable->setItem(0, 4,
                          new QTableWidgetItem(result.m_valid ? QString::number(result.m_deltaVolts * 1000.0, 'f', 3)
                                                              : QString()));
    historyTable->setItem(0, 5,
                          new QTableWidgetItem(QString::number(result.m_currentAmps * 1.0e6, 'f', 3)));
    historyTable->setItem(0, 6, new QTableWidgetItem(status));

    while (historyTable->rowCount() > kMaxHistoryRows) {
        historyTable->removeRow(historyTable->rowCount() - 1);
    }

    refreshReferenceDisplays();
}

void PulseResistanceDialog::updatePulseResistance(WaveformFifo* waveformFifo, int numSamples)
{
    if (isHidden() || !waveformFifo || numSamples <= 0) {
        return;
    }

    syncConfigFingerprint();

    if (state->pulseResistanceUsesExternalAdcTtl()) {
        updatePulseResistanceExternal(waveformFifo, numSamples);
    } else {
        updatePulseResistanceRhs(waveformFifo, numSamples);
    }
}

void PulseResistanceDialog::updatePulseResistanceRhs(WaveformFifo* waveformFifo, int numSamples)
{
    if (state->getControllerTypeEnum() != ControllerStimRecord) {
        return;
    }

    _voltageQuantizationLsbVolts = pulseElectrodeResistance::DcAmplifierLsbVolts;

    const QString nativeName = state->pulseResistanceChannel->getValue();
    if (nativeName.isEmpty() || nativeName == QStringLiteral("N/A")) {
        return;
    }

    Channel* channel = state->signalSources->channelByName(nativeName);
    if (!channel || !channel->stimParameters) {
        return;
    }

    if (!channel->stimParameters->enabled->getValue()) {
        statusLabel->setText(tr("Stimulation disabled on watched channel."));
        return;
    }

    const std::string dcName = nativeName.toStdString() + "|DC";
    const std::string stimName = nativeName.toStdString() + "|STIM";
    const float* dcWaveform = waveformFifo->getAnalogWaveformPointer(dcName);
    const uint16_t* stimWaveform = waveformFifo->getDigitalWaveformPointer(stimName);
    if (!dcWaveform || !stimWaveform) {
        return;
    }

    const double sampleRateHz = state->sampleRate->getNumericValue();
    const double currentMicroAmps = channel->stimParameters->firstPhaseAmplitude->getValue();
    const WaveformFifo::Reader reader = WaveformFifo::ReaderDisplay;

    auto getDcVolts = [&](int timeIndex) {
        return waveformFifo->getAnalogData(reader, dcWaveform, timeIndex);
    };
    auto getStimFlags = [&](int timeIndex) {
        return waveformFifo->getDigitalData(reader, stimWaveform, timeIndex);
    };
    auto getTimeStamp = [&](int timeIndex) {
        return waveformFifo->getTimeStamp(reader, timeIndex);
    };

    const std::optional<pulseElectrodeResistance::PulseResistanceResult> result =
        pulseElectrodeResistance::processStimPulseSamples(numSamples, sampleRateHz, currentMicroAmps, _trackerState,
                                                          getDcVolts, getStimFlags, getTimeStamp);

    const int numWordsInMemory = waveformFifo->numWordsInMemory(reader);
    const std::string highBandName = nativeName.toStdString() + "|HIGH";
    const bool acAvailable = waveformFifo->gpuWaveformPresent(highBandName);
    GpuWaveformAddress acAddress;
    if (acAvailable) {
        acAddress = waveformFifo->getGpuWaveformAddress(highBandName);
    }

    const double traceTimeMs = state->pulseResistanceTraceTimeMs->getNumericValue();
    const std::optional<pulseElectrodeResistance::PulseResponseSnippet> snippet =
        pulseElectrodeResistance::tryCapturePulseResponseSnippet(
            numSamples, sampleRateHz, traceTimeMs, numWordsInMemory, acAvailable, _responseCaptureState, getStimFlags,
            getDcVolts, getTimeStamp, [&](int timeIndex) {
                return waveformFifo->getGpuAmplifierData(reader, acAddress, timeIndex);
            });

    if (snippet.has_value()) {
        responsePlot->setLatestSnippet(*snippet);
    }

    if (result.has_value()) {
        applyResult(*result);
    } else if (!statusLabel->text().contains(tr("Stimulation disabled"))) {
        const QString currentStatus = statusLabel->text();
        if (!currentStatus.startsWith(tr("Invalid"))) {
            statusLabel->setText(tr("Waiting for stim pulse on watched channel."));
        }
    }
}

void PulseResistanceDialog::updatePulseResistanceExternal(WaveformFifo* waveformFifo, int numSamples)
{
    const QString dcAdcName = state->pulseResistanceExternalDcAdc->getValueString();
    if (dcAdcName.isEmpty()) {
        statusLabel->setText(tr("Select a resistance ADC input."));
        return;
    }

    Channel* dcAdcChannel = state->signalSources->channelByName(dcAdcName);
    if (!dcAdcChannel || dcAdcChannel->getSignalType() != BoardAdcSignal) {
        statusLabel->setText(tr("Select a valid board analog input for resistance."));
        return;
    }
    const double dcGain = dcAdcChannel->getExternalAmplifierGain();
    _voltageQuantizationLsbVolts =
        pulseElectrodeResistance::boardAdcLsbVolts(state->getControllerTypeEnum()) / dcGain;

    const float* dcWaveform = waveformFifo->getAnalogWaveformPointer(dcAdcName.toStdString());
    const QString ttlMarkerName = state->pulseResistanceExternalTtlMarker->getValueString();
    Channel* ttlMarkerChannel = state->signalSources->channelByName(ttlMarkerName);
    if (!ttlMarkerChannel) {
        statusLabel->setText(tr("Select a stim TTL marker channel."));
        return;
    }
    const SignalType markerType = ttlMarkerChannel->getSignalType();
    if (markerType != BoardDigitalInSignal && markerType != BoardDigitalOutSignal) {
        statusLabel->setText(tr("Stim TTL marker must be a digital input or output."));
        return;
    }
    const bool ttlOutputMarker = markerType == BoardDigitalOutSignal;
    const char* digitalWordName = ttlOutputMarker ? "DIGITAL-OUT-WORD" : "DIGITAL-IN-WORD";
    const uint16_t* digitalWord = waveformFifo->getDigitalWaveformPointer(digitalWordName);
    if (!dcWaveform || !digitalWord) {
        statusLabel->setText(ttlOutputMarker ? tr("ADC or digital output waveforms unavailable.")
                                             : tr("ADC or digital input waveforms unavailable."));
        return;
    }

    const double currentNanoAmps = state->pulseResistanceExternalCurrentNanoAmps->getValue();
    if (currentNanoAmps <= 0.0) {
        statusLabel->setText(tr("Enter a positive stimulus current (nA)."));
        return;
    }

    const int ttlBitIndex = ttlMarkerChannel->getNativeChannelNumber();
    const bool ttlActiveHigh = state->pulseResistanceExternalTtlActiveHigh->getValue();

    const QString traceAdcValue = state->pulseResistanceExternalTraceAdc->getValueString();
    const bool traceEnabled = traceAdcValue != QStringLiteral("None");
    const float* traceWaveform = nullptr;
    double traceGain = 1.0;
    if (traceEnabled) {
        Channel* traceAdcChannel = state->signalSources->channelByName(traceAdcValue);
        if (!traceAdcChannel || traceAdcChannel->getSignalType() != BoardAdcSignal) {
            statusLabel->setText(tr("Trace ADC must be a board analog input."));
            return;
        }
        traceGain = traceAdcChannel->getExternalAmplifierGain();
        traceWaveform = waveformFifo->getAnalogWaveformPointer(traceAdcValue.toStdString());
        if (!traceWaveform) {
            statusLabel->setText(tr("Trace ADC waveform unavailable."));
            return;
        }
    }

    const double sampleRateHz = state->sampleRate->getNumericValue();
    const WaveformFifo::Reader reader = WaveformFifo::ReaderDisplay;

    auto getDcVolts = [&](int timeIndex) {
        const float adcVolts = waveformFifo->getAnalogData(reader, dcWaveform, timeIndex);
        const std::optional<double> physical =
            boardAdcScaling::physicalVoltsFromBoardAdcVolts(adcVolts, dcGain);
        return physical.has_value() ? static_cast<float>(*physical) : 0.0f;
    };
    auto getStimFlags = [&](int timeIndex) {
        const uint16_t word = waveformFifo->getDigitalData(reader, digitalWord, timeIndex);
        return pulseElectrodeResistance::stimFlagsFromTtlWord(word, ttlBitIndex, ttlActiveHigh);
    };
    auto getTimeStamp = [&](int timeIndex) {
        return waveformFifo->getTimeStamp(reader, timeIndex);
    };

    const double currentMicroAmps = currentNanoAmps * 1.0e-3;
    const std::optional<pulseElectrodeResistance::PulseResistanceResult> result =
        pulseElectrodeResistance::processStimPulseSamples(numSamples, sampleRateHz, currentMicroAmps, _trackerState,
                                                          getDcVolts, getStimFlags, getTimeStamp);

    const int numWordsInMemory = waveformFifo->numWordsInMemory(reader);
    const double traceTimeMs = state->pulseResistanceTraceTimeMs->getNumericValue();

    std::function<float(int)> getTraceVolts;
    if (traceEnabled && traceWaveform) {
        getTraceVolts = [&](int timeIndex) {
            const float adcVolts = waveformFifo->getAnalogData(reader, traceWaveform, timeIndex);
            const std::optional<double> physical =
                boardAdcScaling::physicalVoltsFromBoardAdcVolts(adcVolts, traceGain);
            return physical.has_value() ? static_cast<float>(*physical) : 0.0f;
        };
    }

    const std::optional<pulseElectrodeResistance::PulseResponseSnippet> snippet =
        pulseElectrodeResistance::tryCapturePulseResponseSnippet(
            numSamples, sampleRateHz, traceTimeMs, numWordsInMemory, false, _responseCaptureState, getStimFlags,
            getDcVolts, getTimeStamp, {}, traceEnabled && traceWaveform, getTraceVolts);

    if (snippet.has_value()) {
        responsePlot->setLatestSnippet(*snippet);
    }

    if (result.has_value()) {
        applyResult(*result);
    } else {
        const QString currentStatus = statusLabel->text();
        if (!currentStatus.startsWith(tr("Invalid")) && !currentStatus.contains(tr("Enter a positive"))
            && !currentStatus.contains(tr("unavailable")) && !currentStatus.contains(tr("Select"))) {
            statusLabel->setText(tr("Waiting for TTL-marked stim pulse."));
        }
    }
}
