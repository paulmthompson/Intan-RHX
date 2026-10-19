//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#include "antidromicchannelselection.hpp"

#include "channel.h"
#include "signalsources.h"

#include <algorithm>

namespace {

QString channelDisplayLabel(const Channel* channel)
{
    QString name = channel->getCustomName();
    if (channel->getCustomName() != channel->getNativeName()) {
        name += " (" + channel->getNativeName() + ")";
    }
    return name;
}

} // namespace

std::vector<AntidromicChannelEntry> rhsAmplifierChannelEntries(SignalSources* signalSources)
{
    std::vector<AntidromicChannelEntry> entries;
    if (!signalSources) {
        return entries;
    }

    for (int groupIndex = 0; groupIndex < signalSources->numPortGroups(); ++groupIndex) {
        const SignalGroup* group = signalSources->portGroupByIndex(groupIndex);
        for (int channelIndex = 0; channelIndex < group->numChannels(AmplifierSignal); ++channelIndex) {
            const Channel* channel = group->channelByUserOrder(AmplifierSignal, channelIndex);
            entries.push_back({channelDisplayLabel(channel), channel->getNativeName()});
        }
    }

    std::sort(entries.begin(), entries.end(),
              [](const AntidromicChannelEntry& a, const AntidromicChannelEntry& b) {
                  return a.displayLabel < b.displayLabel;
              });
    return entries;
}

QStringList boardAdcChannelNativeNames(SignalSources* signalSources)
{
    QStringList names;
    if (!signalSources) {
        return names;
    }

    for (int groupIndex = 0; groupIndex < signalSources->numGroups(); ++groupIndex) {
        const SignalGroup* group = signalSources->groupByIndex(groupIndex);
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

QString nativeNameFromDisplayLabel(const QString& displayLabel)
{
    if (!displayLabel.contains(" (")) {
        return displayLabel;
    }
    return displayLabel.section(" (", 1, 1).chopped(1);
}
