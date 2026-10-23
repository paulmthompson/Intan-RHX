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

#include "spikescopechannelname.hpp"

std::optional<QString> canonicalSpikeScopeChannelValue(const QString& value,
                                                       std::optional<SignalType> resolvedType,
                                                       const QString& resolvedNativeName)
{
    if (value.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("N/A");
    }
    if (!resolvedType.has_value()) {
        return std::nullopt;
    }
    const SignalType type = *resolvedType;
    if (type != AmplifierSignal && type != BoardAdcSignal) {
        return std::nullopt;
    }
    if (resolvedNativeName.isEmpty()) {
        return std::nullopt;
    }
    return resolvedNativeName;
}

std::optional<QString> canonicalAmplifierChannelNameValue(const QString& value,
                                                          std::optional<SignalType> resolvedType,
                                                          const QString& resolvedNativeName)
{
    if (value.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("N/A");
    }
    if (!resolvedType.has_value() || *resolvedType != AmplifierSignal) {
        return std::nullopt;
    }
    if (resolvedNativeName.isEmpty()) {
        return std::nullopt;
    }
    return resolvedNativeName;
}
