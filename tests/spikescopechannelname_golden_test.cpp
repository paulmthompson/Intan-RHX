//------------------------------------------------------------------------------
// Golden vectors for Spike Scope channel name canonicalization.
//------------------------------------------------------------------------------

#include "spikescopechannelname.hpp"

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

void testSpikeScopeNa()
{
    const auto canonical = canonicalSpikeScopeChannelValue(QStringLiteral("n/a"), std::nullopt, QString());
    expect(canonical.has_value(), "N/A accepted");
    if (canonical.has_value()) {
        expect(*canonical == QStringLiteral("N/A"), "N/A canonical form");
    }
}

void testSpikeScopeBoardAdc()
{
    const auto canonical = canonicalSpikeScopeChannelValue(QStringLiteral("ANALOG-IN-2"),
                                                           BoardAdcSignal,
                                                           QStringLiteral("ANALOG-IN-2"));
    expect(canonical.has_value(), "board ADC accepted");
    if (canonical.has_value()) {
        expect(*canonical == QStringLiteral("ANALOG-IN-2"), "board ADC native name");
    }
}

void testSpikeScopeAmplifier()
{
    const auto canonical = canonicalSpikeScopeChannelValue(QStringLiteral("A-001"),
                                                           AmplifierSignal,
                                                           QStringLiteral("A-001"));
    expect(canonical.has_value(), "amplifier accepted");
}

void testSpikeScopeRejectsDigital()
{
    const auto canonical = canonicalSpikeScopeChannelValue(QStringLiteral("DIGITAL-IN-0"),
                                                           BoardDigitalInSignal,
                                                           QStringLiteral("DIGITAL-IN-0"));
    expect(!canonical.has_value(), "digital input rejected");
}

void testSpikeScopeUnknownChannel()
{
    const auto canonical = canonicalSpikeScopeChannelValue(QStringLiteral("ANALOG-IN-99"), std::nullopt, QString());
    expect(!canonical.has_value(), "unknown channel rejected");
}

void testAmplifierOnlyRejectsBoardAdc()
{
    const auto canonical = canonicalAmplifierChannelNameValue(QStringLiteral("ANALOG-IN-1"),
                                                              BoardAdcSignal,
                                                              QStringLiteral("ANALOG-IN-1"));
    expect(!canonical.has_value(), "ChannelNameItem rejects board ADC");
}

void testAmplifierOnlyAcceptsAmp()
{
    const auto canonical = canonicalAmplifierChannelNameValue(QStringLiteral("B-010"),
                                                              AmplifierSignal,
                                                              QStringLiteral("B-010"));
    expect(canonical.has_value(), "ChannelNameItem accepts amplifier");
}

} // namespace

int main()
{
    testSpikeScopeNa();
    testSpikeScopeBoardAdc();
    testSpikeScopeAmplifier();
    testSpikeScopeRejectsDigital();
    testSpikeScopeUnknownChannel();
    testAmplifierOnlyRejectsBoardAdc();
    testAmplifierOnlyAcceptsAmp();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "spikescopechannelname_golden_test: OK\n";
    return EXIT_SUCCESS;
}
