//------------------------------------------------------------------------------
// Golden vectors for board ADC external-amplifier gain scaling.
//------------------------------------------------------------------------------

#include "boardadcscaling.hpp"

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

void testIdentityGain()
{
    const auto physical = boardAdcScaling::physicalVoltsFromBoardAdcVolts(1.0, 1.0);
    const auto microvolts = boardAdcScaling::electrodeMicrovoltsFromBoardAdcVolts(1.0, 1.0);
    expect(physical.has_value(), "identity physical");
    expect(microvolts.has_value(), "identity microvolts");
    if (physical.has_value()) {
        expect(std::abs(*physical - 1.0) < 1.0e-12, "identity physical value");
    }
    if (microvolts.has_value()) {
        expect(std::abs(*microvolts - 1.0e6) < 1.0, "identity microvolts value");
    }
}

void testGain1000()
{
    const auto physical = boardAdcScaling::physicalVoltsFromBoardAdcVolts(1.0, 1000.0);
    const auto microvolts = boardAdcScaling::electrodeMicrovoltsFromBoardAdcVolts(1.0, 1000.0);
    expect(physical.has_value() && std::abs(*physical - 0.001) < 1.0e-15, "G=1000 physical");
    expect(microvolts.has_value() && std::abs(*microvolts - 1000.0) < 1.0e-9, "G=1000 microvolts");
}

void testGain10000NegativeAdc()
{
    const auto physical = boardAdcScaling::physicalVoltsFromBoardAdcVolts(-0.5, 10000.0);
    const auto microvolts = boardAdcScaling::electrodeMicrovoltsFromBoardAdcVolts(-0.5, 10000.0);
    expect(physical.has_value() && std::abs(*physical + 5.0e-5) < 1.0e-15, "G=10000 negative physical");
    expect(microvolts.has_value() && std::abs(*microvolts + 50.0) < 1.0e-9, "G=10000 negative microvolts");
}

void testInvalidGain()
{
    expect(!boardAdcScaling::physicalVoltsFromBoardAdcVolts(1.0, 0.0).has_value(), "zero gain");
    expect(!boardAdcScaling::electrodeMicrovoltsFromBoardAdcVolts(1.0, -10.0).has_value(), "negative gain");
}

} // namespace

int main()
{
    testIdentityGain();
    testGain1000();
    testGain10000NegativeAdc();
    testInvalidGain();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "boardadcscaling_golden_test: all passed.\n";
    return EXIT_SUCCESS;
}
