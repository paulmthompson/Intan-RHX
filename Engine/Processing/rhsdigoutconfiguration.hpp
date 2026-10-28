//------------------------------------------------------------------------------
// RHS board digital-output configuration helpers.
//------------------------------------------------------------------------------

#ifndef RHSDIGOUTCONFIGURATION_HPP
#define RHSDIGOUTCONFIGURATION_HPP

#include <QString>

#include <cstdint>
#include <optional>

class StimParameters;

enum class RhsDigOutSource {
    ManualLevel = 0,
    Sequencer = 1
};

enum class RhsDigOutSequenceMode {
    Single = 0,
    Finite = 1,
    Continuous = 2
};

/**
 * @brief Persisted configuration for one RHS board digital output.
 * @ingroup RhsDigitalOut
 *
 * Live Manual High, Continuous Running, and Fire activity are intentionally
 * absent so XML/clipboard operations cannot energize a pin.
 */
struct RhsDigOutConfiguration
{
    RhsDigOutSource m_source = RhsDigOutSource::Sequencer;
    RhsDigOutSequenceMode m_sequenceMode = RhsDigOutSequenceMode::Single;
    bool m_enabled = false;
    bool m_manualFire = true;
    int m_triggerSource = 0;
    bool m_edgeTriggered = true;
    bool m_triggerOnLow = false;
    int m_numberOfPulses = 2;
    double m_postTriggerDelayUs = 0.0;
    double m_highDurationUs = 100.0;
    double m_periodUs = 10000.0;
    double m_refractoryPeriodUs = 1000.0;
};

/**
 * @brief Quantized timing shown by the GUI and programmed into stream 16.
 * @ingroup RhsDigitalOut
 */
struct RhsDigOutQuantizedTiming
{
    uint32_t m_startTick = 0;
    uint32_t m_endStimTick = 0;
    uint32_t m_repeatTick = 0xffffffffU;
    uint32_t m_endTick = 0;
    double m_achievedFrequencyHz = 0.0;
    double m_achievedDutyPercent = 0.0;
};

/**
 * @brief Non-persisted host view of one RHS output's live activity.
 * @ingroup RhsDigitalOut
 */
struct RhsDigOutRuntimeState
{
    bool m_manualLevelHigh = false;
    bool m_continuousRunning = false;
    bool m_manualFireBusy = false;
};

/**
 * @brief Read persisted RHS digital-output fields from StimParameters.
 * @pre @p parameters belongs to a BoardDigitalOutSignal channel.
 * @post The returned value contains configuration only, never live state.
 */
RhsDigOutConfiguration rhsDigOutConfigurationFrom(const StimParameters& parameters);

/**
 * @brief Commit a configuration to BoardDigitalOut StimParameters state items.
 * @pre @p parameters belongs to a BoardDigitalOutSignal channel.
 * @post XML/clipboard-backed state items match @p configuration.
 */
void applyRhsDigOutConfiguration(const RhsDigOutConfiguration& configuration,
                                 StimParameters& parameters);

/**
 * @brief Validate and quantize one RHS digital-output configuration.
 * @param sampleRateHz Current acquisition sample rate in samples/second.
 * @param timing Receives sample-tick values and achieved frequency/duty.
 * @return An informative error, or std::nullopt when valid.
 * @pre @p sampleRateHz is the rate that will clock the FPGA sequencer.
 * @post On success, @p timing is safe to program into 32-bit event registers.
 */
std::optional<QString> validateAndQuantizeRhsDigOutConfiguration(
    const RhsDigOutConfiguration& configuration,
    double sampleRateHz,
    RhsDigOutQuantizedTiming& timing);

/**
 * @brief Build stream-16 TriggerParams without amplifier/DAC shape fields.
 * @pre Trigger source is in [0, 31].
 * @post Bit 8 represents Manual Fire and the low five bits remain FPGA-safe.
 */
uint32_t rhsDigOutTriggerWord(const RhsDigOutConfiguration& configuration);

/**
 * @brief Build stream-16 StimParams without amplifier/DAC shape fields.
 * @pre Pulse count is in [1, 256] for finite modes.
 * @post Bit 8 represents continuous repeat and bits 7:0 hold extra pulses.
 */
uint32_t rhsDigOutStimWord(const RhsDigOutConfiguration& configuration);

/**
 * @brief Compare source and sequence mode for Apply auto-restart eligibility.
 */
bool rhsDigOutSameSourceAndMode(const RhsDigOutConfiguration& lhs,
                                const RhsDigOutConfiguration& rhs);

/**
 * @brief Compare every persisted RHS digital-output configuration field.
 */
bool rhsDigOutConfigurationsEqual(const RhsDigOutConfiguration& lhs,
                                  const RhsDigOutConfiguration& rhs);

#endif // RHSDIGOUTCONFIGURATION_HPP
