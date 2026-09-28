// Copyright (c) 2026 P. M. Thompson.
// Licensed under GPL-3.0-or-later (same as the parent Intan-RHX fork).
// This file was written for this fork and is not authored by Intan Technologies.

#include "ampstimprograms.hpp"

#include "systemstate.h"

AmpStimPrograms::AmpStimPrograms(SingleItemList& channelItems, SystemState* state, SignalType signalType)
{
    for (std::size_t bank = 0; bank < kActiveProgramCount; ++bank) {
        _programs[bank] = std::make_unique<StimParameters>(channelItems, state, signalType, static_cast<int>(bank));
    }
}

StimParameters* AmpStimPrograms::program(std::size_t bank)
{
    return _programs.at(bank).get();
}

const StimParameters* AmpStimPrograms::program(std::size_t bank) const
{
    return _programs.at(bank).get();
}

void AmpStimPrograms::populateProgramFrom(std::size_t destBank, const StimParameters* source)
{
    program(destBank)->populateParametersFrom(source);
}

QStringList AmpStimPrograms::getAttributes(XMLGroup xmlGroup, std::size_t bank) const
{
    return program(bank)->getAttributesForXml(xmlGroup);
}

bool AmpStimPrograms::applyXmlAttribute(std::size_t bank, const QString& attributeName, const QString& attributeValue,
                                        SingleItemList& channelItems, SystemState* state) const
{
    StateSingleItem* singleItem = state->locateStateSingleItem(channelItems, attributeName);
    if (!singleItem) {
        return false;
    }
    if (singleItem->getXMLGroup() != XMLGroupStimParameters) {
        return false;
    }
    if (!program(bank)->ownsStateItem(singleItem)) {
        return false;
    }
    return singleItem->setValue(attributeValue);
}
