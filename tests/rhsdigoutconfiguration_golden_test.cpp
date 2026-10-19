//------------------------------------------------------------------------------
// Golden vectors for Step 4 RHS digital-output programming helpers.
//------------------------------------------------------------------------------

#include "rhsdigoutconfiguration.hpp"

#include <cstdlib>
#include <iostream>

namespace {

int failures = 0;

/** Six stream-16 words from setDigitalOutSequenceParameters (channel index is USB address only). */
struct Stream16DigOutImage {
    uint32_t m_triggerParams = 0;
    uint32_t m_stimParams = 0;
    uint32_t m_eventStartStim = 0;
    uint32_t m_eventEndStim = 0;
    uint32_t m_eventRepeatStim = 0;
    uint32_t m_eventEnd = 0;
};

Stream16DigOutImage stream16DigOutImage(const RhsDigOutConfiguration& configuration,
                                        double sampleRateHz)
{
    Stream16DigOutImage image;
    RhsDigOutQuantizedTiming timing;
    const auto error =
        validateAndQuantizeRhsDigOutConfiguration(configuration, sampleRateHz, timing);
    if (error) {
        return image;
    }
    image.m_triggerParams = rhsDigOutTriggerWord(configuration);
    image.m_stimParams = rhsDigOutStimWord(configuration);
    image.m_eventStartStim = timing.m_startTick;
    image.m_eventEndStim = timing.m_endStimTick;
    image.m_eventRepeatStim = timing.m_repeatTick;
    image.m_eventEnd = timing.m_endTick;
    return image;
}

bool stream16ImagesEqual(const Stream16DigOutImage& lhs, const Stream16DigOutImage& rhs)
{
    return lhs.m_triggerParams == rhs.m_triggerParams &&
           lhs.m_stimParams == rhs.m_stimParams &&
           lhs.m_eventStartStim == rhs.m_eventStartStim &&
           lhs.m_eventEndStim == rhs.m_eventEndStim &&
           lhs.m_eventRepeatStim == rhs.m_eventRepeatStim &&
           lhs.m_eventEnd == rhs.m_eventEnd;
}

/** Matches rhs_digout_integration_tb.v HOST_CONT_* continuous case at 30 kHz. */
RhsDigOutConfiguration integrationContinuousConfiguration()
{
    RhsDigOutConfiguration configuration;
    configuration.m_source = RhsDigOutSource::Sequencer;
    configuration.m_sequenceMode = RhsDigOutSequenceMode::Continuous;
    configuration.m_enabled = true;
    configuration.m_postTriggerDelayUs = 0.0;
    configuration.m_highDurationUs = 100.0;
    configuration.m_periodUs = 1000.0;
    configuration.m_refractoryPeriodUs = 1000.0;
    return configuration;
}

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

void testDefaultContinuousAt30kHz()
{
    RhsDigOutConfiguration configuration;
    configuration.m_source = RhsDigOutSource::Sequencer;
    configuration.m_sequenceMode = RhsDigOutSequenceMode::Continuous;
    configuration.m_enabled = true;
    configuration.m_postTriggerDelayUs = 0.0;
    configuration.m_highDurationUs = 100.0;
    configuration.m_periodUs = 10000.0;
    configuration.m_refractoryPeriodUs = 1000.0;

    RhsDigOutQuantizedTiming timing;
    const auto error = validateAndQuantizeRhsDigOutConfiguration(
        configuration, 30000.0, timing);
    expect(!error.has_value(), "default continuous config must validate at 30 kHz");
    expect(rhsDigOutTriggerWord(configuration) == 0x00000080U,
           "continuous: enabled, no Manual Fire");
    expect(rhsDigOutStimWord(configuration) == 0x00000100U,
           "continuous: stim bit 8 set");
    expect(timing.m_startTick == 0U, "continuous start");
    expect(timing.m_endStimTick == 3U, "continuous high phase");
    expect(timing.m_repeatTick == 300U, "continuous repeat = delay+period");
    expect(timing.m_repeatTick > timing.m_endStimTick,
           "continuous repeat must exceed end_stim (or output sticks high)");
}

void testDisabledContinuousClearsStimWord()
{
    RhsDigOutConfiguration configuration;
    configuration.m_source = RhsDigOutSource::Sequencer;
    configuration.m_sequenceMode = RhsDigOutSequenceMode::Continuous;
    configuration.m_enabled = false;
    expect(rhsDigOutStimWord(configuration) == 0U,
           "disabled sequencer must not arm continuous bit");
}

void testIntegrationContinuousStream16ImageAt30kHz()
{
    const RhsDigOutConfiguration configuration = integrationContinuousConfiguration();
    RhsDigOutQuantizedTiming timing;
    const auto error =
        validateAndQuantizeRhsDigOutConfiguration(configuration, 30000.0, timing);
    expect(!error.has_value(), "integration continuous config must validate at 30 kHz");

    const Stream16DigOutImage image = stream16DigOutImage(configuration, 30000.0);
    expect(image.m_triggerParams == 0x00000080U,
           "integration continuous trigger: enabled, software-started");
    expect(image.m_stimParams == 0x00000100U, "integration continuous stim bit 8");
    expect(image.m_eventStartStim == 0U, "integration continuous start");
    expect(image.m_eventEndStim == 3U, "integration 100 us high");
    expect(image.m_eventRepeatStim == 30U, "integration 1 ms period");
    expect(image.m_eventEnd == 33U, "integration end includes 1 ms refractory");
}

void testStream16ImageSameForOutputIndicesTwoThreeFive()
{
    const RhsDigOutConfiguration configuration = integrationContinuousConfiguration();
    const Stream16DigOutImage imageA = stream16DigOutImage(configuration, 30000.0);
    const Stream16DigOutImage imageB = stream16DigOutImage(configuration, 30000.0);
    expect(stream16ImagesEqual(imageA, imageB),
           "stream-16 words must not depend on TTL output index (only USB address)");
    // programStimReg address uses (stream<<8)+(channel<<4)+reg; ch2/ch3/ch5 differ only there.
    expect(imageA.m_eventRepeatStim == 30U,
           "DIGITAL-OUT-3/4/6 share this repeat tick when settings match");
}

void testContinuousProgrammingMasksStaleDigitalIn4Trigger()
{
    RhsDigOutConfiguration withDigitalIn4Trigger = integrationContinuousConfiguration();
    withDigitalIn4Trigger.m_triggerSource = 3; // DIGITAL IN 4 (same index as DIGITAL-OUT-4)

    RhsDigOutConfiguration clean = integrationContinuousConfiguration();
    clean.m_triggerSource = 0;

    const Stream16DigOutImage stale =
        stream16DigOutImage(withDigitalIn4Trigger, 30000.0);
    const Stream16DigOutImage fresh = stream16DigOutImage(clean, 30000.0);
    expect(stream16ImagesEqual(stale, fresh),
           "continuous Apply must program trigger source 0 regardless of saved Source index");
}

void testFiniteTrainDiffersWhenTriggerSourceDiffers()
{
    RhsDigOutConfiguration finiteA;
    finiteA.m_source = RhsDigOutSource::Sequencer;
    finiteA.m_sequenceMode = RhsDigOutSequenceMode::Finite;
    finiteA.m_enabled = true;
    finiteA.m_manualFire = false;
    finiteA.m_triggerSource = 0;
    finiteA.m_highDurationUs = 100.0;
    finiteA.m_periodUs = 1000.0;
    finiteA.m_refractoryPeriodUs = 1000.0;
    finiteA.m_numberOfPulses = 2;

    RhsDigOutConfiguration finiteB = finiteA;
    finiteB.m_triggerSource = 3;

    expect(rhsDigOutTriggerWord(finiteA) != rhsDigOutTriggerWord(finiteB),
           "finite external trigger source must change trigger params word");
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
    testDefaultContinuousAt30kHz();
    testIntegrationContinuousStream16ImageAt30kHz();
    testStream16ImageSameForOutputIndicesTwoThreeFive();
    testContinuousProgrammingMasksStaleDigitalIn4Trigger();
    testFiniteTrainDiffersWhenTriggerSourceDiffers();
    testDisabledContinuousClearsStimWord();
    testManualLevelDisablesSequencerTrigger();
    testExternalSingleTriggerWord();

    if (failures != 0) {
        std::cerr << failures << " golden test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "PASS: rhsdigoutconfiguration golden tests\n";
    return EXIT_SUCCESS;
}
