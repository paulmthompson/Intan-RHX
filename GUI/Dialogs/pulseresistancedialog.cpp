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

#include <QtWidgets>

#include "channel.h"
#include "pulseresistancedialog.h"
#include "signalsources.h"
#include "waveformfifo.h"

PulseResistanceDialog::PulseResistanceDialog(SystemState* state_, QWidget* parent) :
    QDialog(parent),
    state(state_),
    _watchedChannelNativeName(QString())
{
    connect(state, SIGNAL(stateChanged()), this, SLOT(updateFromState()));

    channelName = new QLabel(QString(), this);

    lockScopeCheckbox = new QCheckBox(tr("Lock Plot to Selected"), this);
    connect(lockScopeCheckbox, SIGNAL(clicked()), this, SLOT(toggleLock()));

    setToSelectedButton = new QPushButton(tr("Set to Selected"), this);
    connect(setToSelectedButton, SIGNAL(clicked()), this, SLOT(setToSelected()));

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

    QLabel* helpLabel = new QLabel(
        tr("Estimates DC pulse resistance (ΔV/I) from the DC amplifier during stimulation. "
           "This is not the AC impedance from the Impedance tab (Zcheck)."),
        this);
    helpLabel->setWordWrap(true);

    historyTable = new QTableWidget(0, 5, this);
    historyTable->setHorizontalHeaderLabels(
        QStringList() << tr("Timestamp") << tr("R") << tr("ΔV (mV)") << tr("I (µA)") << tr("Status"));
    historyTable->horizontalHeader()->setStretchLastSection(true);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QHBoxLayout* lockRow = new QHBoxLayout;
    lockRow->addWidget(channelName);
    lockRow->addStretch(1);
    lockRow->addWidget(lockScopeCheckbox);

    QHBoxLayout* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(setToSelectedButton);
    buttonRow->addWidget(clearHistoryButton);
    buttonRow->addStretch(1);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(lockRow);
    mainLayout->addLayout(buttonRow);
    mainLayout->addWidget(resistanceValueLabel);
    mainLayout->addWidget(deltaVoltageLabel);
    mainLayout->addWidget(currentLabel);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(helpLabel);
    mainLayout->addWidget(historyTable);
    setLayout(mainLayout);

    resize(520, 480);
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
    updateFromState();
}

void PulseResistanceDialog::activate()
{
    updateFromState();
    show();
    raise();
    activateWindow();
}

void PulseResistanceDialog::updateFromState()
{
    QString channelNativeName = state->signalSources->singleSelectedAmplifierChannelName();
    if (channelNativeName != state->pulseResistanceChannel->getValue()) {
        if (!channelNativeName.isEmpty()) {
            setToSelectedButton->setEnabled(true);
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
    statusLabel->setText(tr("Waiting for stim pulse on watched channel."));
    resetTrackerForChannelChange();
}

void PulseResistanceDialog::resetTrackerForChannelChange()
{
    _trackerState = pulseElectrodeResistance::PulseResistanceTrackerState();
}

void PulseResistanceDialog::updateTitle()
{
    setWindowTitle(tr("Pulse Resistance") + " (" + state->pulseResistanceChannel->getValue() + ")");
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
        resistanceValueLabel->setText(tr("R: ") + formatResistanceOhms(result.m_resistanceOhms));
        deltaVoltageLabel->setText(tr("ΔV: ") + QString::number(result.m_deltaVolts * 1000.0, 'f', 3) + tr(" mV"));
        currentLabel->setText(tr("I: ") + QString::number(result.m_currentAmps * 1.0e6, 'f', 3) + tr(" µA"));
    }

    historyTable->insertRow(0);
    historyTable->setItem(0, 0, new QTableWidgetItem(QString::number(result.m_timestamp)));
    historyTable->setItem(0, 1,
                          new QTableWidgetItem(result.m_valid ? formatResistanceOhms(result.m_resistanceOhms) : QString()));
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

    if (result.has_value()) {
        applyResult(*result);
    } else if (!statusLabel->text().contains(tr("Stimulation disabled"))) {
        const QString currentStatus = statusLabel->text();
        if (!currentStatus.startsWith(tr("Invalid"))) {
            statusLabel->setText(tr("Waiting for stim pulse on watched channel."));
        }
    }
}
