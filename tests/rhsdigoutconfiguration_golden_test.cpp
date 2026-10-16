//------------------------------------------------------------------------------
// Golden vectors for Step 4 RHS digital-output programming helpers.
//------------------------------------------------------------------------------

#include "rhsdigoutconfiguration.hpp"

#include <cstdlib>
#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void testDefaultSingleManualFireAt30kHz()
{
    RhsDigOutConfiguration configuration;
    configuration.m_source = RhsDigOutSource::Sequencer;
    configuration.m_sequenceMode = RhsDigOutSequenceMode::Single;
    configuration.m_enabled = true;
    configuration.m_manualFire = true;

    RhsDigOutQuantizedTiming timing;
    const auto error = validateAndQuantizeRhsDigOutConfiguration(
        configuration, 30000.0, timing);
    expect(!error.has_value(), "default GUI config must validate at 30 kHz");

    expect(rhsDigOutTriggerWord(configuration) == 0x00000180U,
           "trigger word: enabled + Manual Fire");
    expect(rhsDigOutStimWord(configuration) == 0U,
           "stim word: single pulse (N-1 = 0)");

    expect(timing.m_startTick == 0U, "start tick");
    expect(timing.m_endStimTick == 3U, "100 us high at 30 kHz");
    expect(timing.m_repeatTick == 0xffffffffU, "single mode repeat Never");
    expect(timing.m_endTick == 33U, "end tick includes 1000 us refractory");
}

void testManualLevelDisablesSequencerTrigger()
{
    RhsDigOutConfiguration configuration;
    configuration.m_source = RhsDigOutSource::ManualLevel;
    configuration.m_enabled = true;
    configuration.m_manualFire = false;

    RhsDigOutQuantizedTiming timing;
    const auto error = validateAndQuantizeRhsDigOutConfiguration(
        configuration, 30000.0, timing);
    expect(!error.has_value(), "manual level config validates");
    expect(rhsDigOutTriggerWord(configuration) == 0U,
           "manual level must not enable sequencer trigger");
}

void testExternalSingleTriggerWord()
{
    RhsDigOutConfiguration configuration;
    configuration.m_source = RhsDigOutSource::Sequencer;
    configuration.m_sequenceMode = RhsDigOutSequenceMode::Single;
    configuration.m_enabled = true;
    configuration.m_manualFire = false;
    configuration.m_triggerSource = 5;
    configuration.m_edgeTriggered = true;
    configuration.m_triggerOnLow = false;

    expect(rhsDigOutTriggerWord(configuration) == 0x000000a5U,
           "external edge-triggered source 5");
}

} // namespace

int main()
{
    testDefaultSingleManualFireAt30kHz();
    testManualLevelDisablesSequencerTrigger();
    testExternalSingleTriggerWord();

    if (failures != 0) {
        std::cerr << failures << " golden test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "PASS: rhsdigoutconfiguration golden tests\n";
    return EXIT_SUCCESS;
}
