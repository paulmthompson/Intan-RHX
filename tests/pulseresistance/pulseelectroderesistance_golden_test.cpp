//------------------------------------------------------------------------------
// Golden vectors for external ADC + TTL pulse resistance helpers.
//------------------------------------------------------------------------------

#include "PulseResponse/pulseelectroderesistance.hpp"

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

void testBoardAdcLsbVolts()
{
    expect(std::abs(pulseElectrodeResistance::boardAdcLsbVolts(ControllerRecordUSB2) - 50.354e-6) < 1.0e-12,
           "RHD USB2 board ADC LSB");
    expect(std::abs(pulseElectrodeResistance::boardAdcLsbVolts(ControllerRecordUSB3) - 312.5e-6) < 1.0e-12,
           "RHD USB3 board ADC LSB");
    expect(std::abs(pulseElectrodeResistance::boardAdcLsbVolts(ControllerStimRecord) - 312.5e-6) < 1.0e-12,
           "RHS board ADC LSB");
}

void testStimFlagsFromTtlWord()
{
    using pulseElectrodeResistance::StimOnFlag;
    using pulseElectrodeResistance::stimFlagsFromTtlWord;

    expect(stimFlagsFromTtlWord(0x0004, 2, true) == StimOnFlag, "TTL bit 2 high, active high");
    expect(stimFlagsFromTtlWord(0x0000, 2, true) == 0, "TTL bit 2 low, active high");
    expect(stimFlagsFromTtlWord(0x0004, 2, false) == 0, "TTL bit 2 high, active low");
    expect(stimFlagsFromTtlWord(0x0000, 2, false) == StimOnFlag, "TTL bit 2 low, active low");
    expect(stimFlagsFromTtlWord(0xffff, -1, true) == 0, "invalid TTL index");
}

void testRelativePercentDifferenceAndThreshold()
{
    using pulseElectrodeResistance::DefaultWarningThresholdPercent;
    using pulseElectrodeResistance::isRelativeIncreaseAboveThreshold;
    using pulseElectrodeResistance::relativePercentDifference;

    const auto diffZero = relativePercentDifference(5.0e6, 5.0e6);
    expect(diffZero.has_value() && std::abs(*diffZero) < 1.0e-9, "0% difference when equal");

    const auto diffPlus30 = relativePercentDifference(6.5e6, 5.0e6);
    expect(diffPlus30.has_value() && std::abs(*diffPlus30 - 30.0) < 1.0e-9, "+30% increase");
    expect(!isRelativeIncreaseAboveThreshold(*diffPlus30, DefaultWarningThresholdPercent),
           "exactly +30% is not strictly above +30% threshold");

    const auto diffPlus35 = relativePercentDifference(6.75e6, 5.0e6);
    expect(diffPlus35.has_value() && std::abs(*diffPlus35 - 35.0) < 1.0e-9, "+35% increase");
    expect(isRelativeIncreaseAboveThreshold(*diffPlus35, DefaultWarningThresholdPercent),
           "+35% exceeds +30% threshold");

    const auto diffMinus20 = relativePercentDifference(4.0e6, 5.0e6);
    expect(diffMinus20.has_value() && std::abs(*diffMinus20 - (-20.0)) < 1.0e-9, "-20% decrease");
    expect(!isRelativeIncreaseAboveThreshold(*diffMinus20, DefaultWarningThresholdPercent),
           "decrease does not trigger positive increase alert");

    expect(!relativePercentDifference(0.0, 5.0e6).has_value(), "zero measured resistance invalid");
    expect(!relativePercentDifference(5.0e6, 0.0).has_value(), "zero reference resistance invalid");
    expect(!relativePercentDifference(-1.0, 5.0e6).has_value(), "negative resistance invalid");
}

} // namespace

int main()
{
    testBoardAdcLsbVolts();
    testStimFlagsFromTtlWord();
    testRelativePercentDifferenceAndThreshold();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "pulseelectroderesistance_golden_test: OK\n";
    return EXIT_SUCCESS;
}
