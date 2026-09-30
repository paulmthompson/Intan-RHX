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

#include "pulseelectroderesistance.hpp"

#include <algorithm>
#include <cmath>

namespace pulseElectrodeResistance {

namespace {

constexpr double kPrePulseWindowSeconds = 0.010;
constexpr double kMinimumPlateauSeconds = 0.001;
constexpr int kMinimumBaselineSamples = 10;
constexpr int kMinimumPlateauSamples = 10;

bool isSampleUsableForBaseline(uint16_t stimFlags)
{
    return (stimFlags & StimOnFlag) == 0 && (stimFlags & AmpSettleFlag) == 0;
}

bool isSampleUsableForPlateau(uint16_t stimFlags)
{
    return (stimFlags & StimOnFlag) != 0 && (stimFlags & ChargeRecoveryFlag) == 0;
}

int prePulseWindowSamples(double sampleRateHz)
{
    const int fromRate = static_cast<int>(std::lround(sampleRateHz * kPrePulseWindowSeconds));
    return std::max(kMinimumBaselineSamples, fromRate);
}

int minimumPlateauSamples(double sampleRateHz)
{
    const int fromRate = static_cast<int>(std::lround(sampleRateHz * kMinimumPlateauSeconds));
    return std::max(kMinimumPlateauSamples, fromRate);
}

std::optional<float> computeBaselineMedian(int risingEdgeIndex, int preWindowSamples,
                                           const std::function<float(int)>& getDcVolts,
                                           const std::function<uint16_t(int)>& getStimFlags)
{
    std::vector<float> baselineSamples;
    baselineSamples.reserve(static_cast<std::size_t>(preWindowSamples));
    for (int t = risingEdgeIndex - preWindowSamples; t < risingEdgeIndex; ++t) {
        const uint16_t stimFlags = getStimFlags(t);
        if (isSampleUsableForBaseline(stimFlags)) {
            baselineSamples.push_back(getDcVolts(t));
        }
    }
    if (static_cast<int>(baselineSamples.size()) < kMinimumBaselineSamples) {
        return std::nullopt;
    }
    return medianOfSamples(std::move(baselineSamples));
}

void resetPulseTracking(PulseResistanceTrackerState& trackerState)
{
    trackerState.m_pulseActive = false;
    trackerState.m_complianceDuringPulse = false;
    trackerState.m_baselineVolts.reset();
    trackerState.m_plateauSamples.clear();
}

} // namespace

float medianOfSamples(std::vector<float> samples)
{
    const std::size_t mid = samples.size() / 2;
    std::nth_element(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(mid), samples.end());
    if (samples.size() % 2 == 1) {
        return samples[mid];
    }
    const float upper = samples[mid];
    std::nth_element(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(mid - 1), samples.end());
    return 0.5F * (samples[mid - 1] + upper);
}

PulseResistanceResult estimateFromBaselineAndPlateau(float baselineVolts, const std::vector<float>& plateauSamples,
                                                     double currentAmps, uint32_t timestamp)
{
    PulseResistanceResult result;
    result.m_timestamp = timestamp;
    result.m_currentAmps = currentAmps;

    if (currentAmps <= 0.0) {
        result.m_failure = PulseResistanceFailure::CurrentTooSmall;  // zero or negative configured amplitude
        return result;
    }
    if (static_cast<int>(plateauSamples.size()) < kMinimumPlateauSamples) {
        result.m_failure = PulseResistanceFailure::InsufficientPlateau;
        return result;
    }

    const float plateauVolts = medianOfSamples(std::vector<float>(plateauSamples.begin(), plateauSamples.end()));
    result.m_deltaVolts = static_cast<double>(plateauVolts) - static_cast<double>(baselineVolts);
    result.m_resistanceOhms = std::abs(result.m_deltaVolts) / currentAmps;
    result.m_valid = std::isfinite(result.m_resistanceOhms) && result.m_resistanceOhms > 0.0;
    return result;
}

std::optional<PulseResistanceResult> processStimPulseSamples(
    int numSamples,
    double sampleRateHz,
    double firstPhaseAmplitudeMicroAmps,
    PulseResistanceTrackerState& trackerState,
    const std::function<float(int timeIndex)>& getDcVolts,
    const std::function<uint16_t(int timeIndex)>& getStimFlags,
    const std::function<uint32_t(int timeIndex)>& getTimeStamp)
{
    if (numSamples <= 0 || sampleRateHz <= 0.0) {
        return std::nullopt;
    }

    const int preWindowSamples = prePulseWindowSamples(sampleRateHz);
    const int minPlateauSamples = minimumPlateauSamples(sampleRateHz);
    std::optional<PulseResistanceResult> completedPulse;

    for (int t = 0; t < numSamples; ++t) {
        const uint16_t stimFlags = getStimFlags(t);
        const bool stimOn = (stimFlags & StimOnFlag) != 0;

        if (stimOn && (stimFlags & ComplianceFlag) != 0) {
            trackerState.m_complianceDuringPulse = true;
        }

        if (stimOn && !trackerState.m_lastStimOn) {
            resetPulseTracking(trackerState);
            trackerState.m_pulseActive = true;
            trackerState.m_baselineVolts = computeBaselineMedian(t, preWindowSamples, getDcVolts, getStimFlags);
        }

        if (trackerState.m_pulseActive && stimOn && isSampleUsableForPlateau(stimFlags)) {
            trackerState.m_plateauSamples.push_back(getDcVolts(t));
        }

        if (!stimOn && trackerState.m_lastStimOn && trackerState.m_pulseActive) {
            PulseResistanceResult result;
            result.m_timestamp = getTimeStamp(t > 0 ? t - 1 : 0);

            if (!trackerState.m_baselineVolts.has_value()) {
                result.m_failure = PulseResistanceFailure::InsufficientBaseline;
                completedPulse = result;
            } else if (static_cast<int>(trackerState.m_plateauSamples.size()) < minPlateauSamples) {
                result.m_failure = PulseResistanceFailure::InsufficientPlateau;
                completedPulse = result;
            } else {
                const double currentAmps = firstPhaseAmplitudeMicroAmps * 1.0e-6;
                result = estimateFromBaselineAndPlateau(*trackerState.m_baselineVolts, trackerState.m_plateauSamples,
                                                          currentAmps, result.m_timestamp);
                if (result.m_valid && trackerState.m_complianceDuringPulse) {
                    result.m_complianceLimited = true;
                }
                completedPulse = result;
            }
            resetPulseTracking(trackerState);
        }

        trackerState.m_lastStimOn = stimOn;
    }

    return completedPulse;
}

} // namespace pulseElectrodeResistance
