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

#ifndef PULSERESISTANCEDIALOG_H
#define PULSERESISTANCEDIALOG_H

#include <QDialog>

#include "pulseelectroderesistance.hpp"
#include "systemstate.h"

class QLabel;
class QPushButton;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QStackedWidget;
class QTableWidget;
class PulseResponsePlot;
class WaveformFifo;

class PulseResistanceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PulseResistanceDialog(SystemState* state_, QWidget* parent = nullptr);

    void updateForRun();
    void updateForLoad();
    void updateForStop();
    void updateForChangeHeadstages();

    /**
     * @brief Process new display FIFO data for the watched channel or external ADC inputs.
     * @pre Acquisition running and dialog visible for meaningful updates.
     * @post UI updated when a stim pulse completes.
     */
    void updatePulseResistance(WaveformFifo* waveformFifo, int numSamples);

    /**
     * @brief Show dialog and refresh from state.
     */
    void activate();

private slots:
    void updateFromState();
    void changeCurrentChannel(const QString& nativeChannelName)
    {
        state->pulseResistanceChannel->setValue(nativeChannelName);
    }
    void toggleLock() { updateFromState(); }
    void setToSelected();
    void clearHistory();
    void setMode(int index);
    void setExternalDcAdc(int index);
    void setExternalTraceAdc(int index);
    void setExternalCurrentNanoAmps(double value);
    void setExternalTtlMarker(int index);
    void setExternalTtlActiveHigh(bool activeHigh);
    void setTraceTimeSpan(int index) { state->pulseResistanceTraceTimeMs->setIndex(index); }
    void setTraceYScaleAc(int index) { state->pulseResistanceTraceYScaleAc->setIndex(index); }
    void setTraceYScaleDc(int index) { state->pulseResistanceTraceYScaleDc->setIndex(index); }

private:
    void updateTitle();
    void resetTrackerForChannelChange();
    void syncConfigFingerprint();
    QString buildConfigFingerprint() const;
    QStringList boardAdcChannelNames() const;
    void refreshExternalAdcComboBoxes();
    void updateHelpText();
    void updateModeUi();
    void applyResult(const pulseElectrodeResistance::PulseResistanceResult& result);
    void updatePulseResistanceRhs(WaveformFifo* waveformFifo, int numSamples);
    void updatePulseResistanceExternal(WaveformFifo* waveformFifo, int numSamples);
    static QString formatResistanceOhms(double ohms);
    QString formatResistanceOhmsWithUncertainty(double ohms, double deltaVolts) const;
    static QString failureReasonText(pulseElectrodeResistance::PulseResistanceFailure failure);

    SystemState* state;

    QLabel* modeLabel;
    QComboBox* modeComboBox;
    QStackedWidget* modeStack;

    QWidget* rhsPage;
    QLabel* channelName;
    QCheckBox* lockScopeCheckbox;
    QPushButton* setToSelectedButton;

    QWidget* externalPage;
    QComboBox* externalDcAdcComboBox;
    QComboBox* externalTraceAdcComboBox;
    QDoubleSpinBox* externalCurrentSpinBox;
    QComboBox* externalTtlMarkerComboBox;
    QCheckBox* externalTtlActiveHighCheckbox;

    QPushButton* clearHistoryButton;

    QLabel* resistanceValueLabel;
    QLabel* deltaVoltageLabel;
    QLabel* currentLabel;
    QLabel* statusLabel;
    QLabel* helpLabel;

    QTableWidget* historyTable;

    QComboBox* traceTimeSpanComboBox;
    QComboBox* traceYScaleAcComboBox;
    QLabel* traceYScaleAcLabel;
    QComboBox* traceYScaleDcComboBox;
    PulseResponsePlot* responsePlot;

    pulseElectrodeResistance::PulseResistanceTrackerState _trackerState;
    pulseElectrodeResistance::PulseResponseCaptureState _responseCaptureState;
    QString _watchedChannelNativeName;
    QString _configFingerprint;
    double _voltageQuantizationLsbVolts;

    static constexpr int kMaxHistoryRows = 50;
};

#endif
