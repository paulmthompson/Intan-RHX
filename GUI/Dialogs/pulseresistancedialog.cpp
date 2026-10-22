//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//  Version 3.2.0
//
//  Copyright (c) 2020-2023 Intan Technologies
//
//  This file is part of the Intan Technologies RHX Data Acquisition Software.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published
//  by the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//  This software is provided 'as-is', without any express or implied warranty.
//  In no event will the authors be held liable for any damages arising from
//  the use of this software.
//
//  See <http://www.intantech.com> for documentation and product information.
//
//------------------------------------------------------------------------------

#include <cmath>
#include <optional>

#include <QtWidgets>

#include "boardadcscaling.hpp"
#include "channel.h"
#include "pulseresistancedialog.h"
#include "pulseresponseplot.h"
#include "signalsources.h"
#include "waveformfifo.h"

PulseResistanceDialog::PulseResistanceDialog(SystemState* state_, QWidget* parent) :
    QDialog(parent),
    state(state_),
    _watchedChannelNativeName(QString()),
    _configFingerprint(QString()),
    _voltageQuantizationLsbVolts(pulseElectrodeResistance::DcAmplifierLsbVolts)
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

    clearHistoryButton = new QPushButton(tr("Clear History"), this);
    connect(clearHistoryButton, SIGNAL(clicked()), this, SLOT(clearHistory()));

    resistanceValueLabel = new QLabel(tr("R: —"), this);
    QFont valueFont = resistanceValueLabel->font();
    valueFont.setPointSize(valueFont.pointSize() + 4);
    valueFont.setBold(true);
    resistanceValueLabel->setFont(valueFont);

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
    connect(traceYScaleAcComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setTraceYScaleAc(int)));

    traceYScaleDcComboBox = new QComboBox(this);
    state->pulseResistanceTraceYScaleDc->setupComboBox(traceYScaleDcComboBox);
    connect(traceYScaleDcComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setTraceYScaleDc(int)));

    responsePlot = new PulseResponsePlot(state, this);

    historyTable = new QTableWidget(0, 5, this);
    historyTable->setHorizontalHeaderLabels(
        QStringList() << tr("Timestamp") << tr("R") << tr("ΔV (mV)") << tr("I (µA)") << tr("Status"));
    historyTable->horizontalHeader()->setStretchLastSection(true);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QHBoxLayout* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(clearHistoryButton);
    buttonRow->addStretch(1);

    QHBoxLayout* traceControlsRow = new QHBoxLayout;
    traceControlsRow->addWidget(new QLabel(tr("Trace time:"), this));
    traceControlsRow->addWidget(traceTimeSpanComboBox);
    traceControlsRow->addWidget(traceYScaleAcLabel);
    traceControlsRow->addWidget(traceYScaleAcComboBox);
    traceControlsRow->addWidget(new QLabel(tr("DC Y:"), this));
    traceControlsRow->addWidget(traceYScaleDcComboBox);
    traceControlsRow->addStretch(1);

    modeLabel = new QLabel(tr("Mode:"), this);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(modeLabel);
    mainLayout->addWidget(modeComboBox);
    mainLayout->addWidget(modeStack);
    mainLayout->addLayout(buttonRow);
    mainLayout->addWidget(resistanceValueLabel);
    mainLayout->addWidget(deltaVoltageLabel);
    mainLayout->addWidget(currentLabel);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(helpLabel);
    mainLayout->addLayout(traceControlsRow);
    mainLayout->addWidget(responsePlot);
    mainLayout->addWidget(historyTable);
    setLayout(mainLayout);

    resize(560, 780);
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
    traceYScaleAcLabel->setVisible(!external);
    traceYScaleAcComboBox->setVisible(!external);
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
    if (traceYScaleAcComboBox->currentIndex() != state->pulseResistanceTraceYScaleAc->getIndex()) {
        traceYScaleAcComboBox->setCurrentIndex(state->pulseResistanceTraceYScaleAc->getIndex());
    }
    if (traceYScaleDcComboBox->currentIndex() != state->pulseResistanceTraceYScaleDc->getIndex()) {
        traceYScaleDcComboBox->setCurrentIndex(state->pulseResistanceTraceYScaleDc->getIndex());
    }

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

void PulseResistanceDialog::clearHistory()
{
    historyTable->setRowCount(0);
    resistanceValueLabel->setText(tr("R: —"));
    deltaVoltageLabel->setText(tr("ΔV: —"));
    currentLabel->setText(tr("I: —"));
    statusLabel->setText(state->pulseResistanceUsesExternalAdcTtl()
                             ? tr("Waiting for TTL-marked stim pulse.")
                             : tr("Waiting for stim pulse on watched channel."));
    resetTrackerForChannelChange();
    responsePlot->clearSnippet();
}

void PulseResistanceDialog::resetTrackerForChannelChange()
{
    _trackerState = pulseElectrodeResistance::PulseResistanceTrackerState();
    _responseCaptureState = pulseElectrodeResistance::PulseResponseCaptureState();
    responsePlot->clearSnippet();
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
        resistanceValueLabel->setText(tr("R: ")
                                      + formatResistanceOhmsWithUncertainty(result.m_resistanceOhms, result.m_deltaVolts));
        deltaVoltageLabel->setText(tr("ΔV: ") + QString::number(result.m_deltaVolts * 1000.0, 'f', 3) + tr(" mV"));
        currentLabel->setText(tr("I: ") + QString::number(result.m_currentAmps * 1.0e6, 'f', 3) + tr(" µA"));
    }

    historyTable->insertRow(0);
    historyTable->setItem(0, 0, new QTableWidgetItem(QString::number(result.m_timestamp)));
    historyTable->setItem(0, 1,
                          new QTableWidgetItem(result.m_valid
                                                   ? formatResistanceOhmsWithUncertainty(result.m_resistanceOhms,
                                                                                         result.m_deltaVolts)
                                                   : QString()));
    historyTable->setItem(0, 2,
                          new QTableWidgetItem(result.m_valid ? QString::number(result.m_deltaVolts * 1000.0, 'f', 3)
                                                              : QString()));
    historyTable->setItem(0, 3,
                          new QTableWidgetItem(QString::number(result.m_currentAmps * 1.0e6, 'f', 3)));
    historyTable->setItem(0, 4, new QTableWidgetItem(status));

    while (historyTable->rowCount() > kMaxHistoryRows) {
        historyTable->removeRow(historyTable->rowCount() - 1);
    }
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
