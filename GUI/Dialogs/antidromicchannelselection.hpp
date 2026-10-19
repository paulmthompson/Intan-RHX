//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#ifndef ANTIDROMICCHANNELSELECTION_HPP
#define ANTIDROMICCHANNELSELECTION_HPP

#include <QString>
#include <QStringList>
#include <vector>

class SignalSources;

/**
 * @brief Display label and native channel name for antidromic channel pickers.
 */
struct AntidromicChannelEntry
{
    QString displayLabel;
    QString nativeName;
};

/**
 * @brief All RHS amplifier channels across populated headstage ports.
 * @param signalSources Signal source tree for the active controller.
 * @return Entries sorted by display label.
 */
std::vector<AntidromicChannelEntry> rhsAmplifierChannelEntries(SignalSources* signalSources);

/**
 * @brief Native names of board analog input (ADC) channels.
 * @param signalSources Signal source tree for the active controller.
 * @return Sorted native channel names.
 */
QStringList boardAdcChannelNativeNames(SignalSources* signalSources);

/**
 * @brief Extract native channel name from a combo display string.
 * @param displayLabel Label shown in the UI (may include custom name and native name in parentheses).
 * @return Native channel name.
 */
QString nativeNameFromDisplayLabel(const QString& displayLabel);

#endif // ANTIDROMICCHANNELSELECTION_HPP
