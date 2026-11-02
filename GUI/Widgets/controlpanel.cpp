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

#include "stimparamdialog.h"
#include "anoutdialog.h"
#include "channel.h"
#include "digoutdialog.hpp"
#include "rhsdigoutconfiguration.hpp"
#include "rhsdigoutlivecontrolswidget.hpp"
#include "controlpanelbandwidthtab.h"
#include "controlpanelimpedancetab.h"
#include "controlpanelaudioanalogtab.h"
#include "controlpanelconfiguretab.h"
#include "controlpaneltriggertab.h"
#include "controlwindow.h"
#include "controlpanel.h"

#include <QMessageBox>

namespace {

bool controlPanelShowsBoardDigOutTools(SystemState* state)
{
    const ControllerType controllerType = state->getControllerTypeEnum();
    return controllerType == ControllerStimRecord ||
           controllerType == ControllerRecordUSB2 ||
           controllerType == ControllerRecordUSB3;
}

bool rhsDigOutThresholdOwned(SystemState* state, Channel* channel)
{
    const int channelIndex = channel->getNativeChannelNumber();
    if (channelIndex < 0 || channelIndex >= 8) {
        return false;
    }
    BooleanItem* thresholdModes[8] = {
        state->analogOut1ThresholdEnabled, state->analogOut2ThresholdEnabled,
        state->analogOut3ThresholdEnabled, state->analogOut4ThresholdEnabled,
        state->analogOut5ThresholdEnabled, state->analogOut6ThresholdEnabled,
        state->analogOut7ThresholdEnabled, state->analogOut8ThresholdEnabled
    };
    return thresholdModes[channelIndex]->getValue();
}

bool promptDigOutDialogDirtyResolution(QWidget* parent, DigOutDialog* dialog)
{
    if (!dialog->hasUnappliedChanges()) {
        return true;
    }
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QObject::tr("Unapplied Digital Output Changes"));
    box.setText(QObject::tr(
        "Apply uncommitted digital-output changes before switching channels?"));
    QPushButton* applyButton =
        box.addButton(QObject::tr("Apply"), QMessageBox::ApplyRole);
    QPushButton* discardButton =
        box.addButton(QObject::tr("Discard"), QMessageBox::DestructiveRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() == applyButton) {
        return dialog->applyDraft();
    }
    if (box.clickedButton() == discardButton) {
        dialog->discardDraft();
        return true;
    }
    return false;
}

} // namespace

ControlPanel::ControlPanel(ControllerInterface* controllerInterface_, SystemState* state_, CommandParser* parser_,
                           ControlWindow *parent) :
    AbstractPanel(controllerInterface_, state_, parser_, parent),
    hideControlPanelButton(nullptr),
    topLabel(nullptr),
    bandwidthTab(nullptr),
    impedanceTab(nullptr),
    audioAnalogTab(nullptr),
    triggerTab(nullptr),
    stimParamDialogChannel(nullptr),
    digOutDialogChannel(nullptr),
    rhsDigOutLiveControlsWidget(nullptr),
    externalAmplifierGainLabel(nullptr),
    externalAmplifierGainSpinBox(nullptr),
    lowSlider(nullptr),
    highSlider(nullptr),
    analogSlider(nullptr),
    lowLabel(nullptr),
    highLabel(nullptr),
    analogLabel(nullptr)
{
    topLabel = new QLabel(tr("Selection Properties"), this);

    hideControlPanelButton = new QToolButton(this);
    hideControlPanelButton->setIcon(QIcon(":/images/hideicon.png"));
    hideControlPanelButton->setToolTip(tr("Hide Control Panel"));
    connect(hideControlPanelButton, SIGNAL(clicked()), controlWindow, SLOT(hideControlPanel()));

    bandwidthTab = new ControlPanelBandwidthTab(controllerInterface, state, this);
    impedanceTab = new ControlPanelImpedanceTab(controllerInterface, state, parser, this);
    audioAnalogTab = new ControlPanelAudioAnalogTab(controllerInterface, state, this);
    triggerTab = new ControlPanelTriggerTab(controllerInterface, state, this);

    tabWidget->addTab(bandwidthTab, tr("BW"));
    tabWidget->addTab(impedanceTab, tr("Impedance"));
    tabWidget->addTab(audioAnalogTab, tr("Audio/Analog"));
    tabWidget->addTab(configureTab, tr("Config"));
    tabWidget->addTab(triggerTab, tr("Trigger"));

    QHBoxLayout *topRow = new QHBoxLayout;
    topRow->addWidget(hideControlPanelButton);
    topRow->addWidget(topLabel);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topRow);
    mainLayout->addLayout(createSelectionLayout());
    mainLayout->addLayout(createSelectionToolsLayout());
    mainLayout->addWidget(filterDisplaySelector);
    mainLayout->addLayout(createDisplayLayout());
    mainLayout->addWidget(tabWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *mainWidget = new QWidget(this);
    mainWidget->setLayout(mainLayout);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(mainWidget);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setMinimumWidth(mainWidget->sizeHint().width());

    QVBoxLayout *scrollLayout = new QVBoxLayout;
    scrollLayout->addWidget(scrollArea);

    setLayout(scrollLayout);

    YScaleUsed yScaleUsed;
    updateSlidersEnabled(yScaleUsed);
    connect(controllerInterface, &ControllerInterface::rhsDigOutRuntimeChanged,
            this, &ControlPanel::updateRhsDigOutLiveControls);

    updateFromState();
}

void ControlPanel::updateForRun()
{
    configureTab->updateForRun();
}

void ControlPanel::updateForStop()
{
    setEnabled(true);

    configureTab->updateForStop();
    updateRhsDigOutLiveControls();
}

void ControlPanel::updateSlidersEnabled(YScaleUsed yScaleUsed)
{
    wideSlider->setEnabled(yScaleUsed.widepassYScaleUsed);
    wideLabel->setEnabled(yScaleUsed.widepassYScaleUsed);
    lowSlider->setEnabled(yScaleUsed.lowpassYScaleUsed);
    lowLabel->setEnabled(yScaleUsed.lowpassYScaleUsed);
    highSlider->setEnabled(yScaleUsed.highpassYScaleUsed);
    highLabel->setEnabled(yScaleUsed.highpassYScaleUsed);
    if (state->getControllerTypeEnum() == ControllerStimRecord) {
        variableSlider->setEnabled(yScaleUsed.dcYScaleUsed);
        variableLabel->setEnabled(yScaleUsed.dcYScaleUsed);
    } else {
        variableSlider->setEnabled(yScaleUsed.auxInputYScaleUsed);
        variableLabel->setEnabled(yScaleUsed.auxInputYScaleUsed);
    }
    analogSlider->setEnabled(yScaleUsed.analogIOYScaleUsed);
    analogLabel->setEnabled(yScaleUsed.analogIOYScaleUsed);
}

YScaleUsed ControlPanel::slidersEnabled() const
{
    YScaleUsed yScaleUsed;
    yScaleUsed.widepassYScaleUsed = wideSlider->isEnabled();
    yScaleUsed.lowpassYScaleUsed = lowSlider->isEnabled();
    yScaleUsed.highpassYScaleUsed = highSlider->isEnabled();
    if (state->getControllerTypeEnum() == ControllerStimRecord) {
        yScaleUsed.dcYScaleUsed = variableSlider->isEnabled();
    } else {
        yScaleUsed.auxInputYScaleUsed = variableSlider->isEnabled();
    }
    yScaleUsed.analogIOYScaleUsed = analogSlider->isEnabled();
    return yScaleUsed;
}

void ControlPanel::setCurrentTabName(QString tabName)
{
    if (tabName == tr("BW")) {
        tabWidget->setCurrentWidget(bandwidthTab);
    } else if (tabName == tr("Impedance")) {
        tabWidget->setCurrentWidget(impedanceTab);
    } else if (tabName == tr("Audio/Analog")) {
        tabWidget->setCurrentWidget(audioAnalogTab);
    } else if (tabName == tr("Config")) {
        tabWidget->setCurrentWidget(configureTab);
    } else if (tabName == tr("Trigger")) {
        tabWidget->setCurrentWidget(triggerTab);
    } else {
        qDebug() << "Unrecognized tabName.";
    }
}

QString ControlPanel::currentTabName() const
{
    if (tabWidget->currentWidget() == bandwidthTab) {
        return tr("BW");
    } else if (tabWidget->currentWidget() == impedanceTab) {
        return tr("Impedance");
    } else if (tabWidget->currentWidget() == audioAnalogTab) {
        return tr("Audio/Analog");
    } else if (tabWidget->currentWidget() == configureTab) {
        return tr("Config");
    } else if (tabWidget->currentWidget() == triggerTab) {
        return tr("Trigger");
    } else {
        qDebug() << "Unrecognized tab widget.";
    }

    return QString();
}

QHBoxLayout* ControlPanel::createSelectionLayout()
{
    enableCheckBox = new QCheckBox(tr("Enable"), this);
    connect(enableCheckBox, SIGNAL(clicked()), this, SLOT(enableChannelsSlot()));

    colorAttribute = new ColorWidget(this);
    connect(colorAttribute, SIGNAL(clicked()), this, SLOT(promptColorChange()));

    selectionNameLabel = new QLabel(tr("no selection"), this);
    selectionImpedanceLabel = new QLabel(tr("no selection"), this);
    selectionReferenceLabel = new QLabel(tr("no selection"), this);
    if (state->getControllerTypeEnum() == ControllerStimRecord) {
        selectionStimTriggerLabel = new QLabel(tr("no selection"), this);
    }

    renameButton  = new QPushButton(tr("Rename"), this);
    renameButton->setFixedWidth(renameButton->fontMetrics().horizontalAdvance(renameButton->text()) + 14);
    connect(renameButton, SIGNAL(clicked()), controlWindow, SLOT(renameChannel()));

    setRefButton  = new QPushButton(tr("Set Ref"), this);
    setRefButton->setFixedWidth(setRefButton->fontMetrics().horizontalAdvance(setRefButton->text()) + 14);
    connect(setRefButton, SIGNAL(clicked()), controlWindow, SLOT(setReference()));

    if (controlPanelShowsBoardDigOutTools(state)) {
        setStimButton = new QPushButton(tr("Set Stim"), this);
        setStimButton->setFixedWidth(setStimButton->fontMetrics().horizontalAdvance(setStimButton->text()) + 14);
        connect(setStimButton, SIGNAL(clicked()), this, SLOT(openStimParametersDialog()));
    }

    QGridLayout* selectionGrid = new QGridLayout;
    selectionGrid->addWidget(new QLabel(tr("Name:"), this), 0, 0, Qt::AlignRight);
    selectionGrid->addWidget(selectionNameLabel, 0, 1, 1, 3);
    selectionGrid->addWidget(new QLabel(tr("Reference:"), this), 1, 0, Qt::AlignRight);
    selectionGrid->addWidget(selectionReferenceLabel, 1, 1, 1, 3);
    selectionGrid->addWidget(new QLabel(tr("Impedance:"), this), 2, 0, Qt::AlignRight);
    selectionGrid->addWidget(selectionImpedanceLabel, 2, 1);
    if (state->getControllerTypeEnum() == ControllerStimRecord) {
        selectionGrid->addWidget(new QLabel(tr("Stim Trigger:"), this), 2, 2, Qt::AlignRight);
        selectionGrid->addWidget(selectionStimTriggerLabel, 2, 3);
    }
    selectionGrid->setColumnMinimumWidth(1, 90);

    QHBoxLayout *selectionLayout = new QHBoxLayout;
    selectionLayout->addLayout(selectionGrid);
    selectionLayout->addStretch(1);
    return selectionLayout;
}

QHBoxLayout* ControlPanel::createSelectionToolsLayout()
{
    QHBoxLayout *selectionToolsLayout = new QHBoxLayout;
    selectionToolsLayout->addWidget(enableCheckBox);
    selectionToolsLayout->addWidget(new QLabel(tr("Color"), this));
    selectionToolsLayout->addWidget(colorAttribute);
    selectionToolsLayout->addWidget(renameButton);
    selectionToolsLayout->addWidget(setRefButton);

    externalAmplifierGainLabel = new QLabel(tr("Amp gain"), this);
    externalAmplifierGainSpinBox = new QDoubleSpinBox(this);
    externalAmplifierGainSpinBox->setDecimals(0);
    externalAmplifierGainSpinBox->setSingleStep(100.0);
    externalAmplifierGainSpinBox->setToolTip(
        tr("External amplifier gain from electrode voltage to voltage at this analog input (dimensionless)."));
    connect(externalAmplifierGainSpinBox, SIGNAL(valueChanged(double)), this,
            SLOT(setExternalAmplifierGainFromUi(double)));
    externalAmplifierGainLabel->setVisible(false);
    externalAmplifierGainSpinBox->setVisible(false);
    selectionToolsLayout->addWidget(externalAmplifierGainLabel);
    selectionToolsLayout->addWidget(externalAmplifierGainSpinBox);

    if (controlPanelShowsBoardDigOutTools(state)) {
        rhsDigOutLiveControlsWidget = new RhsDigOutLiveControlsWidget(this);
        connect(rhsDigOutLiveControlsWidget,
                &RhsDigOutLiveControlsWidget::manualLevelRequested, this,
                &ControlPanel::onRhsDigOutManualLevelRequested);
        connect(rhsDigOutLiveControlsWidget,
                &RhsDigOutLiveControlsWidget::continuousRequested, this,
                &ControlPanel::onRhsDigOutContinuousRequested);
        connect(rhsDigOutLiveControlsWidget,
                &RhsDigOutLiveControlsWidget::fireRequested, this,
                &ControlPanel::onRhsDigOutFireRequested);
        selectionToolsLayout->addWidget(rhsDigOutLiveControlsWidget);
        selectionToolsLayout->addWidget(setStimButton);
    }
    selectionToolsLayout->addStretch(1);
    return selectionToolsLayout;
}

QHBoxLayout* ControlPanel::createDisplayLayout()
{
    timeScaleComboBox = new QComboBox(this);
    state->tScale->setupComboBox(timeScaleComboBox);
    connect(timeScaleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeTimeScale(int)));

    clipWaveformsCheckBox = new QCheckBox(tr("Clip Waves"), this);
    connect(clipWaveformsCheckBox, SIGNAL(checkStateChanged(Qt::CheckState)), this, SLOT(clipWaveforms(Qt::CheckState)));

    QVBoxLayout *timeScaleColumn = new QVBoxLayout;
    timeScaleColumn->addWidget(clipWaveformsCheckBox);
    timeScaleColumn->addStretch();
    timeScaleColumn->addWidget(new QLabel(tr("Time Scale"), this));
    timeScaleColumn->addWidget(timeScaleComboBox);

    const int SliderHeight = 50;

    wideSlider = new QSlider(Qt::Vertical, this);
    wideSlider->setFixedHeight(SliderHeight);
    wideSlider->setRange(0, state->yScaleWide->numberOfItems() - 1);
    wideSlider->setInvertedAppearance(true);
    wideSlider->setInvertedControls(true);
    wideSlider->setPageStep(1);
    wideSlider->setValue(state->yScaleWide->getIndex());
    wideSlider->setTickPosition(QSlider::TicksRight);
//    wideSlider->setStyleSheet("width: 16px");
    connect(wideSlider, SIGNAL(valueChanged(int)), this, SLOT(changeWideScale(int)));

    lowSlider = new QSlider(Qt::Vertical, this);
    lowSlider->setFixedHeight(SliderHeight);
    lowSlider->setRange(0, state->yScaleLow->numberOfItems() - 1);
    lowSlider->setInvertedAppearance(true);
    lowSlider->setInvertedControls(true);
    lowSlider->setPageStep(1);
    lowSlider->setValue(state->yScaleLow->getIndex());
    lowSlider->setTickPosition(QSlider::TicksRight);
    connect(lowSlider, SIGNAL(valueChanged(int)), this, SLOT(changeLowScale(int)));

    highSlider = new QSlider(Qt::Vertical, this);
    highSlider->setFixedHeight(SliderHeight);
    highSlider->setRange(0, state->yScaleHigh->numberOfItems() - 1);
    highSlider->setInvertedAppearance(true);
    highSlider->setInvertedControls(true);
    highSlider->setPageStep(1);
    highSlider->setValue(state->yScaleHigh->getIndex());
    highSlider->setTickPosition(QSlider::TicksRight);
    connect(highSlider, SIGNAL(valueChanged(int)), this, SLOT(changeHighScale(int)));

    variableSlider = new QSlider(Qt::Vertical, this);
    variableSlider->setFixedHeight(SliderHeight);
    variableSlider->setValue(state->yScaleAux->getIndex());
    if (state->getControllerTypeEnum() == ControllerStimRecord) {
        variableSlider->setRange(0, state->yScaleDC->numberOfItems() - 1);
        variableSlider->setValue(state->yScaleDC->getIndex());
        connect(variableSlider, SIGNAL(valueChanged(int)), this, SLOT(changeDCScale(int)));
    } else {
        variableSlider->setRange(0, state->yScaleAux->numberOfItems() - 1);
        variableSlider->setValue(state->yScaleAux->getIndex());
        connect(variableSlider, SIGNAL(valueChanged(int)), this, SLOT(changeAuxScale(int)));
    }
    variableSlider->setInvertedAppearance(true);
    variableSlider->setInvertedControls(true);
    variableSlider->setPageStep(1);
    variableSlider->setTickPosition(QSlider::TicksRight);

    analogSlider = new QSlider(Qt::Vertical, this);
    analogSlider->setFixedHeight(SliderHeight);
    analogSlider->setRange(0, state->yScaleAnalog->numberOfItems() - 1);
    analogSlider->setInvertedAppearance(true);
    analogSlider->setInvertedControls(true);
    analogSlider->setPageStep(1);
    analogSlider->setValue(state->yScaleAnalog->getIndex());
    analogSlider->setTickPosition(QSlider::TicksRight);
    connect(analogSlider, SIGNAL(valueChanged(int)), this, SLOT(changeAnaScale(int)));

    wideLabel = new QLabel(tr("WIDE"), this);
    lowLabel = new QLabel(tr("LOW"), this);
    highLabel = new QLabel(tr("HIGH"), this);
    if (state->getControllerTypeEnum() == ControllerStimRecord) {
        variableLabel = new QLabel(tr("DC"), this);
    } else {
        variableLabel = new QLabel(tr("AUX"), this);
    }
    analogLabel = new QLabel(tr("ANALOG"), this);

    QVBoxLayout *wideColumn = new QVBoxLayout;
    wideColumn->addWidget(wideSlider);
    wideColumn->setAlignment(wideSlider, Qt::AlignHCenter);
    wideColumn->addWidget(wideLabel);
    wideColumn->setAlignment(wideLabel, Qt::AlignHCenter);

    QVBoxLayout *lowColumn = new QVBoxLayout;
    lowColumn->addWidget(lowSlider);
    lowColumn->setAlignment(lowSlider, Qt::AlignHCenter);
    lowColumn->addWidget(lowLabel);
    lowColumn->setAlignment(lowLabel, Qt::AlignHCenter);

    QVBoxLayout *highColumn = new QVBoxLayout;
    highColumn->addWidget(highSlider);
    highColumn->setAlignment(highSlider, Qt::AlignHCenter);
    highColumn->addWidget(highLabel);
    highColumn->setAlignment(highLabel, Qt::AlignHCenter);

    QVBoxLayout *variableColumn = new QVBoxLayout;
    variableColumn->addWidget(variableSlider);
    variableColumn->setAlignment(variableSlider, Qt::AlignHCenter);
    variableColumn->addWidget(variableLabel);
    variableColumn->setAlignment(variableLabel, Qt::AlignHCenter);

    QVBoxLayout *analogColumn = new QVBoxLayout;
    analogColumn->addWidget(analogSlider);
    analogColumn->setAlignment(analogSlider, Qt::AlignHCenter);
    analogColumn->addWidget(analogLabel);
    analogColumn->setAlignment(analogLabel, Qt::AlignHCenter);

    QGridLayout *displayGrid = new QGridLayout;
    displayGrid->setHorizontalSpacing(0);
    displayGrid->addLayout(wideColumn, 0, 0);
    displayGrid->addLayout(lowColumn, 0, 1);
    displayGrid->addLayout(highColumn, 0, 2);
    displayGrid->addLayout(variableColumn, 0, 3);
    displayGrid->addLayout(analogColumn, 0, 4);

    const int ColumnMinWidth = analogLabel->fontMetrics().horizontalAdvance(analogLabel->text()) + 5;
    displayGrid->setColumnMinimumWidth(0, ColumnMinWidth);
    displayGrid->setColumnMinimumWidth(1, ColumnMinWidth);
    displayGrid->setColumnMinimumWidth(2, ColumnMinWidth);
    displayGrid->setColumnMinimumWidth(3, ColumnMinWidth);
    displayGrid->setColumnMinimumWidth(4, ColumnMinWidth);

    QHBoxLayout* displayLayout = new QHBoxLayout;
    displayLayout->addLayout(displayGrid);
    displayLayout->addStretch(1);
    displayLayout->addLayout(timeScaleColumn);
    displayLayout->addStretch(1);
    return displayLayout;
}

void ControlPanel::openStimParametersDialog()
{
    // Single stim-capable channel selected; amp channels may apply live during Run (maintenance mode).
    Channel* selectedChannel = state->signalSources->selectedChannel();
    SignalType type = selectedChannel->getSignalType();

    if (type == AmplifierSignal) {
        const bool needNewDialog = !stimParamDialog || stimParamDialogChannel != selectedChannel;
        if (needNewDialog) {
            if (stimParamDialog) {
                disconnect(stimParamDialog, nullptr, this, nullptr);
                disconnect(this, nullptr, stimParamDialog, nullptr);
                disconnect(qApp, nullptr, stimParamDialog, nullptr);
                delete stimParamDialog;
                stimParamDialog = nullptr;
                stimParamDialogChannel = nullptr;
            }
            stimParamDialog = new StimParamDialog(state, selectedChannel, this);
            stimParamDialogChannel = selectedChannel;
            stimParamDialog->setWindowModality(Qt::NonModal);
            connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), stimParamDialog, SLOT(notifyFocusChanged(QWidget*, QWidget*)));
            connect(stimParamDialog, SIGNAL(accepted()), this, SLOT(onStimParamDialogAccepted()));
            connect(stimParamDialog, SIGNAL(rejected()), this, SLOT(onStimParamDialogRejected()));
        } else {
            stimParamDialog->updateFromState();
        }
        stimParamDialog->activate();
    } else if (type == BoardDigitalOutSignal) {
        if (digOutDialog && digOutDialogChannel != selectedChannel) {
            if (!promptDigOutDialogDirtyResolution(this, digOutDialog)) {
                return;
            }
        }
        const bool needNewDialog =
            !digOutDialog || digOutDialogChannel != selectedChannel;
        if (needNewDialog) {
            if (digOutDialog) {
                disconnect(digOutDialog, nullptr, this, nullptr);
                delete digOutDialog;
                digOutDialog = nullptr;
                digOutDialogChannel = nullptr;
            }
            digOutDialog = new DigOutDialog(
                state, controllerInterface, selectedChannel, this);
            digOutDialogChannel = selectedChannel;
            connect(digOutDialog, &DigOutDialog::configurationApplied, this,
                    &ControlPanel::onDigOutConfigurationApplied);
        }
        digOutDialog->activate();
    } else if (type == BoardDacSignal) {
        if (anOutDialog) {
            disconnect(this, nullptr, anOutDialog, nullptr);
            delete anOutDialog;
            anOutDialog = nullptr;
        }
        anOutDialog = new AnOutDialog(state, selectedChannel, this);
        connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), anOutDialog, SLOT(notifyFocusChanged(QWidget*, QWidget*)));

        if (anOutDialog->exec() == QDialog::Accepted) {
            state->stimParamsHaveChanged = true;
            controllerInterface->uploadStimParameters(selectedChannel);
        }
    }
    state->forceUpdate();
}

void ControlPanel::onStimParamDialogAccepted()
{
    if (!stimParamDialogChannel) {
        return;
    }
    state->stimParamsHaveChanged = true;
    if (state->running) {
        controllerInterface->uploadStimParametersDuringMaintenance(stimParamDialogChannel);
    } else {
        controllerInterface->uploadStimParameters(stimParamDialogChannel);
    }
    state->forceUpdate();
}

void ControlPanel::onStimParamDialogRejected()
{
    if (!stimParamDialog || !stimParamDialogChannel) {
        return;
    }
    stimParamDialog->updateFromState();
}

void ControlPanel::onDigOutConfigurationApplied(Channel* channel)
{
    state->stimParamsHaveChanged = true;
    if (selectionStimTriggerLabel) {
        updateStimTrigger();
    } else if (setStimButton) {
        updateStimParamDialogButton();
    }
    updateRhsDigOutLiveControls();
    if (channel == state->signalSources->selectedChannel()) {
        state->forceUpdate();
    }
}

void ControlPanel::updateRhsDigOutLiveControls()
{
    if (!rhsDigOutLiveControlsWidget) {
        return;
    }
    if (state->signalSources->numChannelsSelected() != 1) {
        rhsDigOutLiveControlsWidget->hide();
        return;
    }
    Channel* channel = state->signalSources->selectedChannel();
    if (channel->getSignalType() != BoardDigitalOutSignal) {
        rhsDigOutLiveControlsWidget->hide();
        return;
    }
    const RhsDigOutConfiguration configuration =
        rhsDigOutConfigurationFrom(*channel->stimParameters);
    const int channelIndex = channel->getNativeChannelNumber();
    const RhsDigOutRuntimeState runtime =
        controllerInterface->rhsDigOutRuntimeState(channelIndex);
    rhsDigOutLiveControlsWidget->updateState(
        configuration, runtime, state->running,
        controllerInterface->supportsRhsDigOutCommands(),
        rhsDigOutThresholdOwned(state, channel),
        state->uploadInProgress->getValue());
}

void ControlPanel::onRhsDigOutManualLevelRequested(bool high)
{
    if (state->signalSources->numChannelsSelected() != 1) {
        return;
    }
    Channel* channel = state->signalSources->selectedChannel();
    if (channel->getSignalType() != BoardDigitalOutSignal) {
        return;
    }
    controllerInterface->setRhsDigOutManualLevel(
        channel->getNativeChannelNumber(), high);
}

void ControlPanel::onRhsDigOutContinuousRequested(bool start)
{
    if (state->signalSources->numChannelsSelected() != 1) {
        return;
    }
    Channel* channel = state->signalSources->selectedChannel();
    if (channel->getSignalType() != BoardDigitalOutSignal) {
        return;
    }
    const int channelIndex = channel->getNativeChannelNumber();
    if (start) {
        // Push committed timing into stream 16 before Start; Start does not program events.
        controllerInterface->uploadStimParameters(channel);
        controllerInterface->startRhsDigOutContinuous(channelIndex);
    } else {
        controllerInterface->stopRhsDigOutSequencer(channelIndex);
    }
}

void ControlPanel::onRhsDigOutFireRequested()
{
    if (state->signalSources->numChannelsSelected() != 1) {
        return;
    }
    Channel* channel = state->signalSources->selectedChannel();
    if (channel->getSignalType() != BoardDigitalOutSignal) {
        return;
    }
    controllerInterface->fireRhsDigOut(channel);
}

void ControlPanel::updateFromState()
{
    state->signalSources->getSelectedSignals(selectedSignals);

    filterDisplaySelector->updateFromState();
    updateSelectionName();
    updateElectrodeImpedance();
    updateReferenceChannel();
    updateColor();
    updateEnableCheckBox();
    updateExternalAmplifierGain();
    updateYScales();

    clipWaveformsCheckBox->setChecked(state->clipWaveforms->getValue());
    timeScaleComboBox->setCurrentIndex(state->tScale->getIndex());

    bandwidthTab->updateFromState();
    impedanceTab->updateFromState();
    audioAnalogTab->updateFromState();
    configureTab->updateFromState();
    triggerTab->updateFromState();

    if (state->getControllerTypeEnum() == ControllerStimRecord) {
        updateStimTrigger();
        updateStimParamDialogButton();
        updateRhsDigOutLiveControls();
    } else if (controllerInterface->supportsRhsDigOutCommands()) {
        updateStimParamDialogButton();
        updateRhsDigOutLiveControls();
    }
}

void ControlPanel::updateYScales()
{
    wideSlider->setValue(state->yScaleWide->getIndex());
    lowSlider->setValue(state->yScaleLow->getIndex());
    highSlider->setValue(state->yScaleHigh->getIndex());

    if (state->getControllerTypeEnum() == ControllerStimRecord) {
        variableSlider->setValue(state->yScaleDC->getIndex());
    } else {
        variableSlider->setValue(state->yScaleAux->getIndex());
    }

    analogSlider->setValue(state->yScaleAnalog->getIndex());
}

void ControlPanel::updateStimParamDialogButton()
{
    if (!setStimButton) {
        return;
    }
    bool enableSetStim = false;
    if (state->signalSources->numChannelsSelected() == 1 && !state->sweeping) {
        Channel* channel = state->signalSources->selectedChannel();
        if (channel->isStimCapable()) {
            if (!state->running) {
                enableSetStim = true;
            } else if (channel->getSignalType() == AmplifierSignal ||
                       channel->getSignalType() == BoardDigitalOutSignal) {
                enableSetStim = true;
            }
        }
    }

    setStimButton->setEnabled(enableSetStim);
}

void ControlPanel::updateStimTrigger()
{
    if (!selectionStimTriggerLabel || !setStimButton) {
        return;
    }
    if (selectedSignals.isEmpty()) {
        selectionStimTriggerLabel->setText(tr("no selection"));
        setStimButton->setEnabled(false);
        return;
    }

    if (!selectedSignals.at(0)->isStimCapable()) {
        selectionStimTriggerLabel->setText(tr("n/a"));
        setStimButton->setEnabled(false);
        return;
    }

    bool sameStimTrigger = true;
    for (int i = 0; i < selectedSignals.size() - 1; i++) {
        if (selectedSignals.at(i)->getStimTrigger() != selectedSignals.at(i + 1)->getStimTrigger() ||
                !selectedSignals.at(i)->isStimEnabled() || !selectedSignals.at(i + 1)->isStimEnabled()) {
            sameStimTrigger = false;
            break;
        }
    }
    if (sameStimTrigger) {
        if (selectedSignals.at(0)->isStimEnabled()) {
            selectionStimTriggerLabel->setText(selectedSignals.at(0)->getStimTrigger());
        } else {
            selectionStimTriggerLabel->setText("");
        }
    } else {
        selectionStimTriggerLabel->setText("");
    }
    if (selectedSignals.size() == 1) {
        updateStimParamDialogButton();
    }
}

void ControlPanel::updateExternalAmplifierGain()
{
    const bool singleBoardAdcSelected =
        state->signalSources->numChannelsSelected() == 1 &&
        !selectedSignals.isEmpty() &&
        selectedSignals.at(0)->getSignalType() == BoardAdcSignal;

    externalAmplifierGainLabel->setVisible(singleBoardAdcSelected);
    externalAmplifierGainSpinBox->setVisible(singleBoardAdcSelected);

    if (!singleBoardAdcSelected) {
        return;
    }

    Channel* channel = selectedSignals.at(0);
    externalAmplifierGainSpinBox->blockSignals(true);
    channel->setupExternalAmplifierGainSpinBox(externalAmplifierGainSpinBox);
    externalAmplifierGainSpinBox->blockSignals(false);
    externalAmplifierGainSpinBox->setEnabled(!(state->running || state->sweeping));
}

void ControlPanel::setExternalAmplifierGainFromUi(double gain)
{
    if (state->signalSources->numChannelsSelected() != 1) {
        return;
    }
    Channel* channel = state->signalSources->selectedChannel();
    if (!channel || channel->getSignalType() != BoardAdcSignal) {
        return;
    }
    channel->setExternalAmplifierGain(gain);
}
