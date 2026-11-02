//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//  Version 3.5.2
//
//  Copyright (c) 2020-2026 Intan Technologies
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
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//  This software is provided 'as-is', without any express or implied warranty.
//  In no event will the authors be held liable for any damages arising from
//  the use of this software.
//
//  See <https://www.intantech.com> for documentation and product information.
//
//------------------------------------------------------------------------------

#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include "abstractpanel.h"

class ControlPanelBandwidthTab;
class ControlPanelImpedanceTab;
class ControlPanelAudioAnalogTab;
class ControlPanelTriggerTab;
class RhsDigOutLiveControlsWidget;

class ControlPanel : public AbstractPanel
{
    Q_OBJECT
public:
    explicit ControlPanel(ControllerInterface* controllerInterface_, SystemState* state_, CommandParser* parser_,
                          ControlWindow *parent = nullptr);

    void updateForRun() override;
    void updateForStop() override;

    void updateSlidersEnabled(YScaleUsed yScaleUsed) override;
    YScaleUsed slidersEnabled() const override;

    void setCurrentTabName(QString tabName) override;
    QString currentTabName() const override;

public slots:
    void updateFromState() override;

protected:
    QHBoxLayout* createSelectionLayout() override;
    QHBoxLayout* createSelectionToolsLayout() override;
    QHBoxLayout* createDisplayLayout() override;

    void updateYScales() override;
    void updateStimTrigger();
    void updateStimParamDialogButton();

protected slots:
    void openStimParametersDialog();

private slots:
    void onStimParamDialogAccepted();
    void onStimParamDialogRejected();
    /** @brief Refresh main-panel live controls after a successful DigOut Apply/OK. */
    void onDigOutConfigurationApplied(Channel* channel);
    /** @brief Sync live Manual/Continuous/Fire controls with selection and runtime. */
    void updateRhsDigOutLiveControls();
    void onRhsDigOutManualLevelRequested(bool high);
    void onRhsDigOutContinuousRequested(bool start);
    void onRhsDigOutFireRequested();
    void setExternalAmplifierGainFromUi(double gain);

private:
    QToolButton *hideControlPanelButton;
    QLabel* topLabel;
    ControlPanelBandwidthTab *bandwidthTab;
    ControlPanelImpedanceTab *impedanceTab;
    ControlPanelAudioAnalogTab *audioAnalogTab;
    ControlPanelTriggerTab *triggerTab;

    Channel* stimParamDialogChannel;
    Channel* digOutDialogChannel;
    RhsDigOutLiveControlsWidget* rhsDigOutLiveControlsWidget;

    QLabel *externalAmplifierGainLabel;
    QDoubleSpinBox *externalAmplifierGainSpinBox;

    QSlider *lowSlider;
    QSlider *highSlider;
    QSlider *analogSlider;

    QLabel *lowLabel;
    QLabel *highLabel;
    QLabel *analogLabel;

    void updateExternalAmplifierGain();
};

#endif // CONTROLPANEL_H
