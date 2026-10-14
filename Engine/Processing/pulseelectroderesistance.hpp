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

#ifndef PULSEELECTRODERESISTANCE_HPP
#define PULSEELECTRODERESISTANCE_HPP

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace pulseElectrodeResistance {

constexpr uint16_t StimOnFlag = 0x0001u;
constexpr uint16_t StimPolFlag = 0x0100u;
constexpr uint16_t AmpSettleFlag = 0x2000u;
constexpr uint16_t ChargeRecoveryFlag = 0x4000u;
constexpr uint16_t ComplianceFlag = 0x8000u;

/** DC amplifier voltage resolution (10-bit, ±10 V) in volts; matches RHXDataReader scaling. */
constexpr double DcAmplifierLsbVolts = 0.01923;

/**
 * @brief Relative resistance uncertainty (percent) from 10-bit DC quantization on baseline and plateau.
 * @pre deltaVolts is the estimated pulse amplitude in volts.
 * @post Returns 100 * (2 * DcAmplifierLsbVolts) / |deltaVolts|, or infinity if deltaVolts is zero.
 */
double resistanceQuantizationUncertaintyPercent(double deltaVolts);

enum class PulseResistanceFailure {
    None,
    Compliance,
    InsufficientBaseline,
    InsufficientPlateau,
    CurrentTooSmall,
};

struct PulseResistanceResult {
    bool m_valid = false;
    bool m_complianceLimited = false;
    PulseResistanceFailure m_failure = PulseResistanceFailure::None;
    double m_resistanceOhms = 0.0;
    double m_deltaVolts = 0.0;
    double m_currentAmps = 0.0;
    uint32_t m_timestamp = 0;
};

struct PulseResistanceTrackerState {
    bool m_lastStimOn = false;
    bool m_pulseActive = false;
    bool m_complianceDuringPulse = false;
    std::optional<float> m_baselineVolts;
    std::vector<float> m_plateauSamples;
};

struct PulseResponseSnippet {
    std::vector<float> m_acMicroVolts;
    std::vector<float> m_dcVolts;
    uint32_t m_timestamp = 0;
    bool m_acAvailable = false;
};

struct PulseResponseCaptureState {
    bool m_lastStimOn = false;
    /** Samples remaining until capture after stim onset; -1 when idle. */
    int m_samplesUntilCapture = -1;
};

/**
 * @brief Compute the median of a non-empty sample vector.
 * @pre samples is not empty.
 * @post Return value is the median element.
 */
float medianOfSamples(std::vector<float> samples);

/**
 * @brief Estimate electrode DC resistance from one completed stim-on interval.
 * @pre baselineVolts and plateauSamples hold enough valid samples; currentAmps > 0.
 * @post Returns PulseResistanceResult with m_valid true when estimation succeeds.
 */
PulseResistanceResult estimateFromBaselineAndPlateau(float baselineVolts, const std::vector<float>& plateauSamples,
                                                     double currentAmps, uint32_t timestamp);

/**
 * @brief Scan new FIFO samples for stim-on edges and update resistance estimates.
 *
 * Uses per-sample STIM flags and DC volts. Maintains tracker state across display refresh chunks.
 *
 * @pre getDcVolts and getStimFlags valid for timeIndex in [-historySamples, numSamples).
 * @post trackerState updated; optional result when a stim pulse completes (falling edge).
 */
std::optional<PulseResistanceResult> processStimPulseSamples(
    int numSamples,
    double sampleRateHz,
    double firstPhaseAmplitudeMicroAmps,
    PulseResistanceTrackerState& trackerState,
    const std::function<float(int timeIndex)>& getDcVolts,
    const std::function<uint16_t(int timeIndex)>& getStimFlags,
    const std::function<uint32_t(int timeIndex)>& getTimeStamp);

/**
 * @brief Capture a symmetric AC/DC snippet centered on stim onset when the full window is in FIFO memory.
 *
 * @pre getDcVolts and getStimFlags valid for timeIndex in [-numWordsInMemory, numSamples).
 * @post captureState updated; optional snippet when a pulse onset is centered in a complete window.
 */
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
    const std::function<float(int timeIndex)>& getAcMicroVolts);

} // namespace pulseElectrodeResistance

#endif
