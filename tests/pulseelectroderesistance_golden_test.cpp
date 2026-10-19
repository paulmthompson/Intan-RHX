//------------------------------------------------------------------------------
// Golden vectors for external ADC + TTL pulse resistance helpers.
//------------------------------------------------------------------------------

#include "pulseelectroderesistance.hpp"

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

} // namespace

int main()
{
    testBoardAdcLsbVolts();
    testStimFlagsFromTtlWord();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "pulseelectroderesistance_golden_test: OK\n";
    return EXIT_SUCCESS;
}
