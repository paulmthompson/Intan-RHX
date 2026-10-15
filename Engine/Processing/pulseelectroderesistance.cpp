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
#include <limits>

namespace pulseElectrodeResistance {

double resistanceQuantizationUncertaintyPercent(double deltaVolts, double voltageLsbVolts)
{
    const double magnitude = std::abs(deltaVolts);
    if (magnitude <= 0.0 || voltageLsbVolts <= 0.0) {
        return std::numeric_limits<double>::infinity();
    }
    return 100.0 * (2.0 * voltageLsbVolts) / magnitude;
}

double boardAdcLsbVolts(ControllerType controllerType)
{
    if (controllerType == ControllerRecordUSB2) {
        return 50.354e-6;
    }
    return 312.5e-6;
}

uint16_t stimFlagsFromTtlWord(uint16_t digitalInWord, int ttlBitIndex, bool activeHigh)
{
    if (ttlBitIndex < 0) {
        return 0;
    }
    const uint16_t mask = static_cast<uint16_t>(0x01u << ttlBitIndex);
    const bool lineHigh = (digitalInWord & mask) != 0;
    const bool stimActive = activeHigh ? lineHigh : !lineHigh;
    return stimActive ? StimOnFlag : 0;
}

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

namespace {

int halfWindowSamplesFromTotalTimeMs(double totalTimeMs, double sampleRateHz)
{
    if (totalTimeMs <= 0.0 || sampleRateHz <= 0.0) {
        return 0;
    }
    return static_cast<int>(std::lround((totalTimeMs * 0.5) * sampleRateHz / 1000.0));
}

std::optional<PulseResponseSnippet> extractSnippetAtOnset(int onsetIndex, int halfWindowSamples, int numWordsInMemory,
                                                          bool acAvailable, bool traceAvailable,
                                                          const std::function<float(int)>& getDcVolts,
                                                          const std::function<uint32_t(int)>& getTimeStamp,
                                                          const std::function<float(int)>& getAcMicroVolts,
                                                          const std::function<float(int)>& getTraceVolts)
{
    const int windowStart = onsetIndex - halfWindowSamples;
    const int windowEnd = onsetIndex + halfWindowSamples;
    if (windowStart < -numWordsInMemory) {
        return std::nullopt;
    }

    const int snippetLength = 2 * halfWindowSamples;
    PulseResponseSnippet snippet;
    snippet.m_dcVolts.resize(static_cast<std::size_t>(snippetLength));
    snippet.m_acAvailable = acAvailable;
    snippet.m_traceAvailable = traceAvailable;
    if (acAvailable) {
        snippet.m_acMicroVolts.resize(static_cast<std::size_t>(snippetLength));
    }
    if (traceAvailable) {
        snippet.m_traceVolts.resize(static_cast<std::size_t>(snippetLength));
    }

    int outIndex = 0;
    for (int i = windowStart; i < windowEnd; ++i) {
        snippet.m_dcVolts[static_cast<std::size_t>(outIndex)] = getDcVolts(i);
        if (acAvailable) {
            snippet.m_acMicroVolts[static_cast<std::size_t>(outIndex)] = getAcMicroVolts(i);
        }
        if (traceAvailable) {
            snippet.m_traceVolts[static_cast<std::size_t>(outIndex)] = getTraceVolts(i);
        }
        ++outIndex;
    }
    snippet.m_timestamp = getTimeStamp(onsetIndex);
    return snippet;
}

} // namespace

std::optional<PulseResponseSnippet> tryCapturePulseResponseSnippet(
    int numSamples,
    double sampleRateHz,
    double totalTimeMs,
    int numWordsInMemory,
    bool acAvailable,
    PulseResponseCaptureState& captureState,
    const std::function<uint16_t(int timeIndex)>& getStimFlags,
    const std::function<float(int timeIndex)>& getDcVolts,
    const std::function<uint32_t(int timeIndex)>& getTimeStamp,
    const std::function<float(int timeIndex)>& getAcMicroVolts,
    bool traceAvailable,
    const std::function<float(int timeIndex)>& getTraceVolts)
{
    if (numSamples <= 0 || sampleRateHz <= 0.0 || totalTimeMs <= 0.0) {
        return std::nullopt;
    }

    const int halfWindowSamples = halfWindowSamplesFromTotalTimeMs(totalTimeMs, sampleRateHz);
    if (halfWindowSamples <= 0) {
        return std::nullopt;
    }

    std::optional<PulseResponseSnippet> captured;

    for (int t = 0; t < numSamples; ++t) {
        const uint16_t stimFlags = getStimFlags(t);
        const bool stimOn = (stimFlags & StimOnFlag) != 0;

        const bool stimOnset = stimOn && !captureState.m_lastStimOn;
        if (stimOnset) {
            if (halfWindowSamples <= 1) {
                captured = extractSnippetAtOnset(t, halfWindowSamples, numWordsInMemory, acAvailable, traceAvailable,
                                                 getDcVolts, getTimeStamp, getAcMicroVolts, getTraceVolts);
                captureState.m_samplesUntilCapture = -1;
            } else {
                captureState.m_samplesUntilCapture = halfWindowSamples - 1;
            }
        } else if (captureState.m_samplesUntilCapture > 0) {
            --captureState.m_samplesUntilCapture;
            if (captureState.m_samplesUntilCapture == 0) {
                const int onsetIndex = t - (halfWindowSamples - 1);
                captured = extractSnippetAtOnset(onsetIndex, halfWindowSamples, numWordsInMemory, acAvailable,
                                                 traceAvailable, getDcVolts, getTimeStamp, getAcMicroVolts,
                                                 getTraceVolts);
                captureState.m_samplesUntilCapture = -1;
            }
        }

        captureState.m_lastStimOn = stimOn;
    }

    return captured;
}

} // namespace pulseElectrodeResistance
