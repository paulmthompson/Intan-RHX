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
class QTableWidget;
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
     * @brief Process new display FIFO data for the watched channel.
     * @pre Acquisition running and dialog visible for meaningful updates.
     * @post UI updated when a stim pulse completes on the watched channel.
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

private:
    void updateTitle();
    void resetTrackerForChannelChange();
    void applyResult(const pulseElectrodeResistance::PulseResistanceResult& result);
    static QString formatResistanceOhms(double ohms);
    static QString failureReasonText(pulseElectrodeResistance::PulseResistanceFailure failure);

    SystemState* state;

    QLabel* channelName;
    QCheckBox* lockScopeCheckbox;
    QPushButton* setToSelectedButton;
    QPushButton* clearHistoryButton;

    QLabel* resistanceValueLabel;
    QLabel* deltaVoltageLabel;
    QLabel* currentLabel;
    QLabel* statusLabel;

    QTableWidget* historyTable;

    pulseElectrodeResistance::PulseResistanceTrackerState _trackerState;
    QString _watchedChannelNativeName;

    static constexpr int kMaxHistoryRows = 50;
};

#endif
