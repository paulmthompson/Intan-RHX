//------------------------------------------------------------------------------
// Golden vectors for board ADC spike threshold crossing helpers.
//------------------------------------------------------------------------------

#include "boardadcspikedetection.hpp"

#include <cmath>
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

void testFifoSampleToMicrovolts()
{
    const auto microvolts = boardAdcSpikeDetection::boardAdcFifoSampleToElectrodeMicrovolts(1.0f, 1000.0);
    expect(microvolts.has_value(), "conversion succeeds");
    if (microvolts.has_value()) {
        expect(std::abs(*microvolts - 1000.0f) < 1.0e-3f, "G=1000 uV");
    }
    expect(!boardAdcSpikeDetection::boardAdcFifoSampleToElectrodeMicrovolts(1.0f, 0.0).has_value(), "zero gain");
}

void testNegativeCrossing()
{
    expect(boardAdcSpikeDetection::crossedSpikeThresholdNegative(-50.0f, -80.0f, -70), "cross down through -70");
    expect(!boardAdcSpikeDetection::crossedSpikeThresholdNegative(-80.0f, -90.0f, -70), "already below");
    expect(!boardAdcSpikeDetection::crossedSpikeThresholdNegative(-60.0f, -50.0f, -70), "moving up");
    expect(!boardAdcSpikeDetection::crossedSpikeThresholdNegative(-70.0f, -70.0f, -70), "flat at threshold");
}

void testGainScalesEffectiveThreshold()
{
    // electrode µV = adcVolts * 1e6 / gain; at gain 1000, -0.08 V ADC → -80 µV
    const float adcVolts = -0.08f;
    const auto uV = boardAdcSpikeDetection::boardAdcFifoSampleToElectrodeMicrovolts(adcVolts, 1000.0);
    expect(uV.has_value() && *uV < -70.0f, "ADC voltage maps below -70 uV at gain 1000");
}

} // namespace

int main()
{
    testFifoSampleToMicrovolts();
    testNegativeCrossing();
    testGainScalesEffectiveThreshold();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "boardadcspikedetection_golden_test: all passed.\n";
    return EXIT_SUCCESS;
}
