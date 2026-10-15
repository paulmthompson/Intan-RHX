//------------------------------------------------------------------------------
// RHS board digital-output configuration helpers.
//------------------------------------------------------------------------------

#include "rhsdigoutconfiguration.hpp"

#include "stimparameters.h"

#include <cmath>
#include <limits>

namespace {

constexpr uint32_t Never = 0xffffffffU;
constexpr uint32_t ManualFireBit = 1U << 8;
constexpr uint32_t ContinuousBit = 1U << 8;

std::optional<QString> quantizeMicroseconds(double microseconds,
                                            double sampleRateHz,
                                            uint32_t& ticks)
{
    if (!std::isfinite(microseconds) || microseconds < 0.0 ||
        !std::isfinite(sampleRateHz) || sampleRateHz <= 0.0) {
        return QString("Digital-output timing and sample rate must be finite and non-negative.");
    }
    const long double quantized = std::round(
        static_cast<long double>(microseconds) *
        static_cast<long double>(sampleRateHz) / 1.0e6L);
    if (quantized > static_cast<long double>(std::numeric_limits<uint32_t>::max())) {
        return QString("Digital-output timing exceeds the FPGA 32-bit event range.");
    }
    ticks = static_cast<uint32_t>(quantized);
    return std::nullopt;
}

} // namespace

RhsDigOutConfiguration rhsDigOutConfigurationFrom(const StimParameters& parameters)
{
    RhsDigOutConfiguration configuration;
    configuration.m_source = parameters.digOutSource &&
            parameters.digOutSource->getIndex() == static_cast<int>(RhsDigOutSource::ManualLevel) ?
        RhsDigOutSource::ManualLevel : RhsDigOutSource::Sequencer;
    configuration.m_sequenceMode =
        static_cast<RhsDigOutSequenceMode>(parameters.pulseOrTrain->getIndex());
    configuration.m_enabled = parameters.enabled->getValue();
    configuration.m_manualFire =
        parameters.digOutManualTrigger && parameters.digOutManualTrigger->getValue();
    configuration.m_triggerSource = parameters.triggerSource->getIndex();
    configuration.m_edgeTriggered =
        parameters.triggerEdgeOrLevel->getIndex() == TriggerEdge;
    configuration.m_triggerOnLow =
        parameters.triggerHighOrLow->getIndex() == TriggerLow;
    configuration.m_numberOfPulses = parameters.numberOfStimPulses->getValue();
    configuration.m_postTriggerDelayUs = parameters.postTriggerDelay->getValue();
    configuration.m_highDurationUs = parameters.firstPhaseDuration->getValue();
    configuration.m_periodUs = parameters.pulseTrainPeriod->getValue();
    configuration.m_refractoryPeriodUs = parameters.refractoryPeriod->getValue();
    return configuration;
}

void applyRhsDigOutConfiguration(const RhsDigOutConfiguration& configuration,
                                 StimParameters& parameters)
{
    parameters.digOutSource->setIndex(static_cast<int>(configuration.m_source));
    parameters.pulseOrTrain->setIndex(static_cast<int>(configuration.m_sequenceMode));
    parameters.enabled->setValue(configuration.m_enabled);
    parameters.digOutManualTrigger->setValue(configuration.m_manualFire);
    parameters.triggerSource->setIndex(configuration.m_triggerSource);
    parameters.triggerEdgeOrLevel->setIndex(
        configuration.m_edgeTriggered ? TriggerEdge : TriggerLevel);
    parameters.triggerHighOrLow->setIndex(
        configuration.m_triggerOnLow ? TriggerLow : TriggerHigh);
    parameters.numberOfStimPulses->setValue(configuration.m_numberOfPulses);
    parameters.postTriggerDelay->setValue(configuration.m_postTriggerDelayUs);
    parameters.firstPhaseDuration->setValue(configuration.m_highDurationUs);
    parameters.pulseTrainPeriod->setValue(configuration.m_periodUs);
    parameters.refractoryPeriod->setValue(configuration.m_refractoryPeriodUs);
}

std::optional<QString> validateAndQuantizeRhsDigOutConfiguration(
    const RhsDigOutConfiguration& configuration,
    double sampleRateHz,
    RhsDigOutQuantizedTiming& timing)
{
    if (configuration.m_triggerSource < 0 || configuration.m_triggerSource > 31) {
        return QString("Digital-output trigger source is outside the FPGA range.");
    }
    if (configuration.m_manualFire &&
        configuration.m_sequenceMode == RhsDigOutSequenceMode::Continuous) {
        return QString("Continuous mode cannot use the Manual Fire trigger.");
    }
    if (configuration.m_sequenceMode == RhsDigOutSequenceMode::Finite &&
        (configuration.m_numberOfPulses < 2 || configuration.m_numberOfPulses > 256)) {
        return QString("A finite train must contain between 2 and 256 pulses.");
    }
    if (configuration.m_source == RhsDigOutSource::ManualLevel ||
        !configuration.m_enabled) {
        timing = RhsDigOutQuantizedTiming();
        return std::nullopt;
    }

    uint32_t delayTicks = 0;
    uint32_t highTicks = 0;
    uint32_t periodTicks = 0;
    uint32_t refractoryTicks = 0;
    if (auto error = quantizeMicroseconds(
            configuration.m_postTriggerDelayUs, sampleRateHz, delayTicks)) {
        return error;
    }
    if (auto error = quantizeMicroseconds(
            configuration.m_highDurationUs, sampleRateHz, highTicks)) {
        return error;
    }
    if (auto error = quantizeMicroseconds(
            configuration.m_periodUs, sampleRateHz, periodTicks)) {
        return error;
    }
    if (auto error = quantizeMicroseconds(
            configuration.m_refractoryPeriodUs, sampleRateHz, refractoryTicks)) {
        return error;
    }
    if (highTicks == 0) {
        return QString("Pulse duration rounds to zero sample ticks.");
    }
    if (configuration.m_sequenceMode != RhsDigOutSequenceMode::Single &&
        highTicks >= periodTicks) {
        return QString("Pulse duration must be shorter than the period after sample-rate rounding.");
    }

    const uint64_t endStim = static_cast<uint64_t>(delayTicks) + highTicks;
    const uint64_t end = endStim + refractoryTicks;
    const uint64_t repeat = static_cast<uint64_t>(delayTicks) + periodTicks;
    const uint64_t maxTick = std::numeric_limits<uint32_t>::max();
    if (endStim > maxTick || end > maxTick ||
        (configuration.m_sequenceMode != RhsDigOutSequenceMode::Single &&
         repeat > maxTick)) {
        return QString("Digital-output event ordering exceeds the FPGA 32-bit range.");
    }

    timing.m_startTick = delayTicks;
    timing.m_endStimTick = static_cast<uint32_t>(endStim);
    timing.m_repeatTick =
        configuration.m_sequenceMode == RhsDigOutSequenceMode::Single ?
        Never : static_cast<uint32_t>(repeat);
    timing.m_endTick = static_cast<uint32_t>(end);
    if (configuration.m_sequenceMode != RhsDigOutSequenceMode::Single) {
        timing.m_achievedFrequencyHz = sampleRateHz / static_cast<double>(periodTicks);
        timing.m_achievedDutyPercent =
            100.0 * static_cast<double>(highTicks) / static_cast<double>(periodTicks);
    } else {
        timing.m_achievedFrequencyHz = 0.0;
        timing.m_achievedDutyPercent = 0.0;
    }
    return std::nullopt;
}

uint32_t rhsDigOutTriggerWord(const RhsDigOutConfiguration& configuration)
{
    const bool enabled = configuration.m_enabled &&
        configuration.m_source == RhsDigOutSource::Sequencer;
    const bool softwareTriggered =
        configuration.m_manualFire ||
        configuration.m_sequenceMode == RhsDigOutSequenceMode::Continuous;
    const uint32_t source = softwareTriggered ?
        0U : static_cast<uint32_t>(configuration.m_triggerSource);
    return source |
           (!softwareTriggered && configuration.m_edgeTriggered ? (1U << 5) : 0U) |
           (!softwareTriggered && configuration.m_triggerOnLow ? (1U << 6) : 0U) |
           (enabled ? (1U << 7) : 0U) |
           (configuration.m_manualFire ? ManualFireBit : 0U);
}

uint32_t rhsDigOutStimWord(const RhsDigOutConfiguration& configuration)
{
    const int pulses = configuration.m_sequenceMode == RhsDigOutSequenceMode::Finite ?
        configuration.m_numberOfPulses : 1;
    return static_cast<uint32_t>(pulses - 1) |
           (configuration.m_sequenceMode == RhsDigOutSequenceMode::Continuous ?
            ContinuousBit : 0U);
}

bool rhsDigOutSameSourceAndMode(const RhsDigOutConfiguration& lhs,
                                const RhsDigOutConfiguration& rhs)
{
    return lhs.m_source == rhs.m_source &&
           lhs.m_sequenceMode == rhs.m_sequenceMode;
}

bool rhsDigOutConfigurationsEqual(const RhsDigOutConfiguration& lhs,
                                  const RhsDigOutConfiguration& rhs)
{
    return rhsDigOutSameSourceAndMode(lhs, rhs) &&
           lhs.m_enabled == rhs.m_enabled &&
           lhs.m_manualFire == rhs.m_manualFire &&
           lhs.m_triggerSource == rhs.m_triggerSource &&
           lhs.m_edgeTriggered == rhs.m_edgeTriggered &&
           lhs.m_triggerOnLow == rhs.m_triggerOnLow &&
           lhs.m_numberOfPulses == rhs.m_numberOfPulses &&
           lhs.m_postTriggerDelayUs == rhs.m_postTriggerDelayUs &&
           lhs.m_highDurationUs == rhs.m_highDurationUs &&
           lhs.m_periodUs == rhs.m_periodUs &&
           lhs.m_refractoryPeriodUs == rhs.m_refractoryPeriodUs;
}
