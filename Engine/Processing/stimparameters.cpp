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

/**
 * @file stimparameters.cpp
 * @brief Constructs StimParameters range items (microsecond maxima per channel type).
 * @ingroup RhxStimSequencer
 *
 * @details Maintenance: see docs/stim-sequencer-maintenance.md
 */

#include "stimparameters.h"

namespace {

QString stimXmlParameterName(const QString& baseName, int programIndex)
{
    if (programIndex == 0) {
        return baseName;
    }
    return QString("Program%1_%2").arg(programIndex).arg(baseName);
}

} // namespace

StimParameters::StimParameters(SingleItemList &hList_, SystemState *state_, SignalType signalType_, int programIndex_) :
    stimShape(nullptr),
    stimPolarity(nullptr),
    triggerSource(nullptr),
    triggerEdgeOrLevel(nullptr),
    triggerHighOrLow(nullptr),
    pulseOrTrain(nullptr),
    enabled(nullptr),
    maintainAmpSettle(nullptr),
    enableAmpSettle(nullptr),
    enableChargeRecovery(nullptr),
    firstPhaseDuration(nullptr),
    secondPhaseDuration(nullptr),
    interphaseDelay(nullptr),
    firstPhaseAmplitude(nullptr),
    secondPhaseAmplitude(nullptr),
    baselineVoltage(nullptr),
    postTriggerDelay(nullptr),
    pulseTrainPeriod(nullptr),
    refractoryPeriod(nullptr),
    preStimAmpSettle(nullptr),
    postStimAmpSettle(nullptr),
    postStimChargeRecovOn(nullptr),
    postStimChargeRecovOff(nullptr),
    signalType(signalType_),
    _programIndex(programIndex_),
    _state(state_)
{
    if (signalType == AmplifierSignal) {

        stimShape = new DiscreteItemList(stimXmlParameterName("Shape", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        stimShape->addItem("Biphasic", "Biphasic");
        stimShape->addItem("BiphasicWithInterphaseDelay", "BiphasicWithInterphaseDelay");
        stimShape->addItem("Triphasic", "Triphasic");
        stimShape->setValue("Biphasic");

        stimPolarity = new DiscreteItemList(stimXmlParameterName("Polarity", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        stimPolarity->addItem("NegativeFirst", "NegativeFirst");
        stimPolarity->addItem("PositiveFirst", "PositiveFirst");
        stimPolarity->setValue("NegativeFirst");

        triggerSource = new DiscreteItemList(stimXmlParameterName("Source", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        for (int i = 0; i < 16; ++i) {
            QString channelName = "DigitalIn" + QString("%1").arg(i + 1, 2, 10, QChar('0'));
            triggerSource->addItem(channelName, channelName);
        }
        for (int i = 0; i < 8; ++i) {
            QString channelName = "AnalogIn" + QString("%1").arg(i + 1, 2, 10, QChar('0'));
            triggerSource->addItem(channelName, channelName);
        }
        for (int i = 0; i < 8; ++i) {
            QString channelName = "KeyPressF" + QString::number(i + 1);
            triggerSource->addItem(channelName, channelName);
        }
        triggerSource->setValue("DigitalIn01");

        triggerEdgeOrLevel = new DiscreteItemList(stimXmlParameterName("TriggerEdgeOrLevel", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        triggerEdgeOrLevel->addItem("Edge", "Edge");
        triggerEdgeOrLevel->addItem("Level", "Level");
        triggerEdgeOrLevel->setValue("Edge");

        triggerHighOrLow = new DiscreteItemList(stimXmlParameterName("TriggerHighOrLow", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        triggerHighOrLow->addItem("High", "High");
        triggerHighOrLow->addItem("Low", "Low");
        triggerHighOrLow->setValue("High");

        pulseOrTrain = new DiscreteItemList(stimXmlParameterName("PulseOrTrain", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        pulseOrTrain->addItem("SinglePulse", "SinglePulse");
        pulseOrTrain->addItem("PulseTrain", "PulseTrain");
        pulseOrTrain->setValue("SinglePulse");

        enabled = new BooleanItem(stimXmlParameterName("StimEnabled", programIndex_), hList_, state_, false, XMLGroupStimParameters, TypeDependencyStim);
        maintainAmpSettle = new BooleanItem(stimXmlParameterName("MaintainAmpSettle", programIndex_), hList_, state_, false, XMLGroupStimParameters, TypeDependencyStim);
        enableAmpSettle = new BooleanItem(stimXmlParameterName("EnableAmpSettle", programIndex_), hList_, state_, true, XMLGroupStimParameters, TypeDependencyStim);
        enableChargeRecovery = new BooleanItem(stimXmlParameterName("EnableChargeRecovery", programIndex_), hList_, state_, false, XMLGroupStimParameters, TypeDependencyStim);

        double const MAXIMUM_DURATION = 1.0e7; // 10 s is 1e7 us

        firstPhaseDuration = new DoubleRangeItem(stimXmlParameterName("FirstPhaseDurationMicroseconds", programIndex_), hList_, state_, 0.0, MAXIMUM_DURATION, 100.0, XMLGroupStimParameters, TypeDependencyStim);
        secondPhaseDuration = new DoubleRangeItem(stimXmlParameterName("SecondPhaseDurationMicroseconds", programIndex_), hList_, state_, 0.0, MAXIMUM_DURATION, 100.0, XMLGroupStimParameters, TypeDependencyStim);
        interphaseDelay = new DoubleRangeItem(stimXmlParameterName("InterphaseDelayMicroseconds", programIndex_), hList_, state_, 0.0, MAXIMUM_DURATION, 100.0, XMLGroupStimParameters, TypeDependencyStim);
        firstPhaseAmplitude = new DoubleRangeItem(stimXmlParameterName("FirstPhaseAmplitudeMicroAmps", programIndex_), hList_, state_, 0.0, 2550.0, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        secondPhaseAmplitude = new DoubleRangeItem(stimXmlParameterName("SecondPhaseAmplitudeMicroAmps", programIndex_), hList_, state_, 0.0, 2550.0, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        postTriggerDelay = new DoubleRangeItem(stimXmlParameterName("PostTriggerDelayMicroseconds", programIndex_), hList_, state_, 0.0, MAXIMUM_DURATION, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        pulseTrainPeriod = new DoubleRangeItem(stimXmlParameterName("PulseTrainPeriodMicroseconds", programIndex_), hList_, state_, 0.0, 1000000.0, 10000.0, XMLGroupStimParameters, TypeDependencyStim);
        refractoryPeriod = new DoubleRangeItem(stimXmlParameterName("RefractoryPeriodMicroseconds", programIndex_), hList_, state_, 0.0, 1000000.0, 1000.0, XMLGroupStimParameters, TypeDependencyStim);
        preStimAmpSettle = new DoubleRangeItem(stimXmlParameterName("PreStimAmpSettleMicroseconds", programIndex_), hList_, state_, 0.0, 500000.0, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        postStimAmpSettle = new DoubleRangeItem(stimXmlParameterName("PostStimAmpSettleMicroseconds", programIndex_), hList_, state_, 0.0, 500000.0, 1000.0, XMLGroupStimParameters, TypeDependencyStim);
        postStimChargeRecovOn = new DoubleRangeItem(stimXmlParameterName("PostStimChargeRecovOnMicroseconds", programIndex_), hList_, state_, 0.0, 1000000.0, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        postStimChargeRecovOff = new DoubleRangeItem(stimXmlParameterName("PostStimChargeRecovOffMicroseconds", programIndex_), hList_, state_, 0.0, 1000000.0, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        numberOfStimPulses = new IntRangeItem(stimXmlParameterName("NumberOfStimPulses", programIndex_), hList_, state_, 0, 256, 2, XMLGroupStimParameters, TypeDependencyStim);

    } else if (signalType == BoardDacSignal) {

        stimShape = new DiscreteItemList(stimXmlParameterName("Shape", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        stimShape->addItem("Biphasic", "Biphasic");
        stimShape->addItem("BiphasicWithInterphaseDelay", "BiphasicWithInterphaseDelay");
        stimShape->addItem("Triphasic", "Triphasic");
        stimShape->addItem("Monophasic", "Monophasic");
        stimShape->setValue("Biphasic");

        stimPolarity = new DiscreteItemList(stimXmlParameterName("Polarity", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        stimPolarity->addItem("NegativeFirst", "NegativeFirst");
        stimPolarity->addItem("PositiveFirst", "PositiveFirst");
        stimPolarity->setValue("NegativeFirst");

        triggerSource = new DiscreteItemList(stimXmlParameterName("Source", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        for (int i = 0; i < 16; ++i) {
            QString channelName = "DigitalIn" + QString("%1").arg(i + 1, 2, 10, QChar('0'));
            triggerSource->addItem(channelName, channelName);
        }
        for (int i = 0; i < 8; ++i) {
            QString channelName = "AnalogIn" + QString("%1").arg(i + 1, 2, 10, QChar('0'));
            triggerSource->addItem(channelName, channelName);
        }
        for (int i = 0; i < 8; ++i) {
            QString channelName = "KeyPressF" + QString::number(i + 1);
            triggerSource->addItem(channelName, channelName);
        }
        triggerSource->setValue("DigitalIn01");

        triggerEdgeOrLevel = new DiscreteItemList(stimXmlParameterName("TriggerEdgeOrLevel", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        triggerEdgeOrLevel->addItem("Edge", "Edge");
        triggerEdgeOrLevel->addItem("Level", "Level");
        triggerEdgeOrLevel->setValue("Edge");

        triggerHighOrLow = new DiscreteItemList(stimXmlParameterName("TriggerHighOrLow", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        triggerHighOrLow->addItem("High", "High");
        triggerHighOrLow->addItem("Low", "Low");
        triggerHighOrLow->setValue("High");

        pulseOrTrain = new DiscreteItemList(stimXmlParameterName("PulseOrTrain", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        pulseOrTrain->addItem("SinglePulse", "SinglePulse");
        pulseOrTrain->addItem("PulseTrain", "PulseTrain");
        pulseOrTrain->setValue("SinglePulse");

        enabled = new BooleanItem(stimXmlParameterName("StimEnabled", programIndex_), hList_, state_, false, XMLGroupStimParameters, TypeDependencyStim);

        firstPhaseDuration = new DoubleRangeItem(stimXmlParameterName("FirstPhaseDurationMicroseconds", programIndex_), hList_, state_, 0.0, 50000.0, 100.0, XMLGroupStimParameters, TypeDependencyStim);
        secondPhaseDuration = new DoubleRangeItem(stimXmlParameterName("SecondPhaseDurationMicroseconds", programIndex_), hList_, state_, 0.0, 50000.0, 100.0, XMLGroupStimParameters, TypeDependencyStim);
        interphaseDelay = new DoubleRangeItem(stimXmlParameterName("InterphaseDelayMicroseconds", programIndex_), hList_, state_, 0.0, 5000.0, 100.0, XMLGroupStimParameters, TypeDependencyStim);
        firstPhaseAmplitude = new DoubleRangeItem(stimXmlParameterName("FirstPhaseAmplitudeVolts", programIndex_), hList_, state_, 0.0, 10.24, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        secondPhaseAmplitude = new DoubleRangeItem(stimXmlParameterName("SecondPhaseAmplitudeVolts", programIndex_), hList_, state_, 0.0, 10.24, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        baselineVoltage = new DoubleRangeItem(stimXmlParameterName("BaselineVoltageVolts", programIndex_), hList_, state_, -10.24, 10.24, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        postTriggerDelay = new DoubleRangeItem(stimXmlParameterName("PostTriggerDelayMicroseconds", programIndex_), hList_, state_, 0.0, 500000.0, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        pulseTrainPeriod = new DoubleRangeItem(stimXmlParameterName("PulseTrainPeriodMicroseconds", programIndex_), hList_, state_, 0.0, 1000000.0, 10000.0, XMLGroupStimParameters, TypeDependencyStim);
        refractoryPeriod = new DoubleRangeItem(stimXmlParameterName("RefractoryPeriodMicroseconds", programIndex_), hList_, state_, 0.0, 1000000.0, 1000.0, XMLGroupStimParameters, TypeDependencyStim);

        numberOfStimPulses = new IntRangeItem(stimXmlParameterName("NumberOfStimPulses", programIndex_), hList_, state_, 0, 256, 2, XMLGroupStimParameters, TypeDependencyStim);

    } else {

        triggerSource = new DiscreteItemList(stimXmlParameterName("Source", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        for (int i = 0; i < 16; ++i) {
            QString channelName = "DigitalIn" + QString("%1").arg(i + 1, 2, 10, QChar('0'));
            triggerSource->addItem(channelName, channelName);
        }
        for (int i = 0; i < 8; ++i) {
            QString channelName = "AnalogIn" + QString("%1").arg(i + 1, 2, 10, QChar('0'));
            triggerSource->addItem(channelName, channelName);
        }
        for (int i = 0; i < 8; ++i) {
            QString channelName = "KeyPressF" + QString::number(i + 1);
            triggerSource->addItem(channelName, channelName);
        }
        triggerSource->setValue("DigitalIn01");

        triggerEdgeOrLevel = new DiscreteItemList(stimXmlParameterName("TriggerEdgeOrLevel", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        triggerEdgeOrLevel->addItem("Edge", "Edge");
        triggerEdgeOrLevel->addItem("Level", "Level");
        triggerEdgeOrLevel->setValue("Edge");

        triggerHighOrLow = new DiscreteItemList(stimXmlParameterName("TriggerHighOrLow", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        triggerHighOrLow->addItem("High", "High");
        triggerHighOrLow->addItem("Low", "Low");
        triggerHighOrLow->setValue("High");

        pulseOrTrain = new DiscreteItemList(stimXmlParameterName("PulseOrTrain", programIndex_), hList_, state_, XMLGroupStimParameters, TypeDependencyStim);
        pulseOrTrain->addItem("SinglePulse", "SinglePulse");
        pulseOrTrain->addItem("PulseTrain", "PulseTrain");
        pulseOrTrain->setValue("SinglePulse");

        enabled = new BooleanItem(stimXmlParameterName("StimEnabled", programIndex_), hList_, state_, false, XMLGroupStimParameters, TypeDependencyStim);

        firstPhaseDuration = new DoubleRangeItem(stimXmlParameterName("FirstPhaseDurationMicroseconds", programIndex_), hList_, state_, 0.0, 1000000, 100.0, XMLGroupStimParameters, TypeDependencyStim);
        postTriggerDelay = new DoubleRangeItem(stimXmlParameterName("PostTriggerDelayMicroseconds", programIndex_), hList_, state_, 0.0, 500000.0, 0.0, XMLGroupStimParameters, TypeDependencyStim);
        pulseTrainPeriod = new DoubleRangeItem(stimXmlParameterName("PulseTrainPeriodMicroseconds", programIndex_), hList_, state_, 0.0, 1000000.0, 10000.0, XMLGroupStimParameters, TypeDependencyStim);
        refractoryPeriod = new DoubleRangeItem(stimXmlParameterName("RefractoryPeriodMicroseconds", programIndex_), hList_, state_, 0.0, 1000000.0, 1000.0, XMLGroupStimParameters, TypeDependencyStim);

        numberOfStimPulses = new IntRangeItem(stimXmlParameterName("NumberOfStimPulses", programIndex_), hList_, state_, 0, 256, 2, XMLGroupStimParameters, TypeDependencyStim);
    }
}

void StimParameters::populateParametersFrom(StimParameters *originalStimParameters)
{
    if (stimShape)
        stimShape->setIndex(originalStimParameters->stimShape->getIndex());

    if (stimPolarity)
        stimPolarity->setIndex(originalStimParameters->stimPolarity->getIndex());

    if (triggerSource)
        triggerSource->setIndex(originalStimParameters->triggerSource->getIndex());

    if (triggerEdgeOrLevel)
        triggerEdgeOrLevel->setIndex(originalStimParameters->triggerEdgeOrLevel->getIndex());

    if (triggerHighOrLow)
        triggerHighOrLow->setIndex(originalStimParameters->triggerHighOrLow->getIndex());

    if (pulseOrTrain)
        pulseOrTrain->setIndex(originalStimParameters->pulseOrTrain->getIndex());

    if (enabled)
        enabled->setValue(originalStimParameters->enabled->getValue());

    if (maintainAmpSettle)
        maintainAmpSettle->setValue(originalStimParameters->maintainAmpSettle->getValue());

    if (enableAmpSettle)
        enableAmpSettle->setValue(originalStimParameters->enableAmpSettle->getValue());

    if (enableChargeRecovery)
        enableChargeRecovery->setValue(originalStimParameters->enableChargeRecovery->getValue());


    if (firstPhaseDuration)
        firstPhaseDuration->setValue(originalStimParameters->firstPhaseDuration->getValue());

    if (secondPhaseDuration)
        secondPhaseDuration->setValue(originalStimParameters->secondPhaseDuration->getValue());

    if (interphaseDelay)
        interphaseDelay->setValue(originalStimParameters->interphaseDelay->getValue());

    if (firstPhaseAmplitude)
        firstPhaseAmplitude->setValue(originalStimParameters->firstPhaseAmplitude->getValue());

    if (secondPhaseAmplitude)
        secondPhaseAmplitude->setValue(originalStimParameters->secondPhaseAmplitude->getValue());

    if (baselineVoltage)
        baselineVoltage->setValue(originalStimParameters->baselineVoltage->getValue());

    if (postTriggerDelay)
        postTriggerDelay->setValue(originalStimParameters->postTriggerDelay->getValue());

    if (pulseTrainPeriod)
        pulseTrainPeriod->setValue(originalStimParameters->pulseTrainPeriod->getValue());

    if (refractoryPeriod)
        refractoryPeriod->setValue(originalStimParameters->refractoryPeriod->getValue());

    if (preStimAmpSettle)
        preStimAmpSettle->setValue(originalStimParameters->preStimAmpSettle->getValue());

    if (postStimAmpSettle)
        postStimAmpSettle->setValue(originalStimParameters->postStimAmpSettle->getValue());

    if (postStimChargeRecovOn)
        postStimChargeRecovOn->setValue(originalStimParameters->postStimChargeRecovOn->getValue());

    if (postStimChargeRecovOff)
        postStimChargeRecovOff->setValue(originalStimParameters->postStimChargeRecovOff->getValue());

    if (numberOfStimPulses)
        numberOfStimPulses->setValue(originalStimParameters->numberOfStimPulses->getValue());
}

namespace {

bool itemMatchesProgram(const StateSingleItem* item, const StateSingleItem* candidate)
{
    return item != nullptr && item == candidate;
}

void appendItemAttribute(const StateSingleItem* item, XMLGroup xmlGroup, SystemState* state, QStringList& attributeList)
{
    if (!item || item->getXMLGroup() != xmlGroup) {
        return;
    }
    bool addAttribute = false;
    switch (item->getTypeDependency()) {
    case TypeDependencyNone:
        addAttribute = true;
        break;
    case TypeDependencyNonStim:
        if (state->getControllerTypeEnum() != ControllerStimRecord) {
            addAttribute = true;
        }
        break;
    case TypeDependencyStim:
        if (state->getControllerTypeEnum() == ControllerStimRecord) {
            addAttribute = true;
        }
        break;
    }
    if (addAttribute) {
        attributeList.append(item->getParameterName() + ":_:" + item->getValueString());
    }
}

} // namespace

bool StimParameters::ownsStateItem(const StateSingleItem* item) const
{
    if (!item) {
        return false;
    }
    return itemMatchesProgram(item, stimShape) ||
           itemMatchesProgram(item, stimPolarity) ||
           itemMatchesProgram(item, triggerSource) ||
           itemMatchesProgram(item, triggerEdgeOrLevel) ||
           itemMatchesProgram(item, triggerHighOrLow) ||
           itemMatchesProgram(item, pulseOrTrain) ||
           itemMatchesProgram(item, enabled) ||
           itemMatchesProgram(item, maintainAmpSettle) ||
           itemMatchesProgram(item, enableAmpSettle) ||
           itemMatchesProgram(item, enableChargeRecovery) ||
           itemMatchesProgram(item, firstPhaseDuration) ||
           itemMatchesProgram(item, secondPhaseDuration) ||
           itemMatchesProgram(item, interphaseDelay) ||
           itemMatchesProgram(item, firstPhaseAmplitude) ||
           itemMatchesProgram(item, secondPhaseAmplitude) ||
           itemMatchesProgram(item, baselineVoltage) ||
           itemMatchesProgram(item, postTriggerDelay) ||
           itemMatchesProgram(item, pulseTrainPeriod) ||
           itemMatchesProgram(item, refractoryPeriod) ||
           itemMatchesProgram(item, preStimAmpSettle) ||
           itemMatchesProgram(item, postStimAmpSettle) ||
           itemMatchesProgram(item, postStimChargeRecovOn) ||
           itemMatchesProgram(item, postStimChargeRecovOff) ||
           itemMatchesProgram(item, numberOfStimPulses);
}

QStringList StimParameters::getAttributesForXml(XMLGroup xmlGroup) const
{
    QStringList attributeList;
    appendItemAttribute(stimShape, xmlGroup, _state, attributeList);
    appendItemAttribute(stimPolarity, xmlGroup, _state, attributeList);
    appendItemAttribute(triggerSource, xmlGroup, _state, attributeList);
    appendItemAttribute(triggerEdgeOrLevel, xmlGroup, _state, attributeList);
    appendItemAttribute(triggerHighOrLow, xmlGroup, _state, attributeList);
    appendItemAttribute(pulseOrTrain, xmlGroup, _state, attributeList);
    appendItemAttribute(enabled, xmlGroup, _state, attributeList);
    appendItemAttribute(maintainAmpSettle, xmlGroup, _state, attributeList);
    appendItemAttribute(enableAmpSettle, xmlGroup, _state, attributeList);
    appendItemAttribute(enableChargeRecovery, xmlGroup, _state, attributeList);
    appendItemAttribute(firstPhaseDuration, xmlGroup, _state, attributeList);
    appendItemAttribute(secondPhaseDuration, xmlGroup, _state, attributeList);
    appendItemAttribute(interphaseDelay, xmlGroup, _state, attributeList);
    appendItemAttribute(firstPhaseAmplitude, xmlGroup, _state, attributeList);
    appendItemAttribute(secondPhaseAmplitude, xmlGroup, _state, attributeList);
    appendItemAttribute(baselineVoltage, xmlGroup, _state, attributeList);
    appendItemAttribute(postTriggerDelay, xmlGroup, _state, attributeList);
    appendItemAttribute(pulseTrainPeriod, xmlGroup, _state, attributeList);
    appendItemAttribute(refractoryPeriod, xmlGroup, _state, attributeList);
    appendItemAttribute(preStimAmpSettle, xmlGroup, _state, attributeList);
    appendItemAttribute(postStimAmpSettle, xmlGroup, _state, attributeList);
    appendItemAttribute(postStimChargeRecovOn, xmlGroup, _state, attributeList);
    appendItemAttribute(postStimChargeRecovOff, xmlGroup, _state, attributeList);
    appendItemAttribute(numberOfStimPulses, xmlGroup, _state, attributeList);
    return attributeList;
}
