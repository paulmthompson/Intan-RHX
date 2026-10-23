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
//------------------------------------------------------------------------------

#ifndef SPIKESCOPECHANNELNAME_HPP
#define SPIKESCOPECHANNELNAME_HPP

#include <optional>

#include <QString>

#include "rhxglobals.h"

/**
 * @brief Canonical Spike Scope channel name from user input and resolved channel metadata.
 * @param value User-supplied channel name (may include a waveform filter suffix; not stripped here).
 * @param resolvedType Signal type when channelByName succeeds; empty when lookup failed.
 * @param resolvedNativeName Native name when lookup succeeded.
 * @return Canonical stored name, or empty when the value is not allowed for Spike Scope.
 */
std::optional<QString> canonicalSpikeScopeChannelValue(const QString& value,
                                                      std::optional<SignalType> resolvedType,
                                                      const QString& resolvedNativeName);

/**
 * @brief Whether a channel name is valid for amplifier-only ChannelNameItem state (ISI, PSTH, etc.).
 */
std::optional<QString> canonicalAmplifierChannelNameValue(const QString& value,
                                                          std::optional<SignalType> resolvedType,
                                                          const QString& resolvedNativeName);

#endif // SPIKESCOPECHANNELNAME_HPP
