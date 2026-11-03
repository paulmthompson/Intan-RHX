//------------------------------------------------------------------------------
// Golden vectors for antidromic spike detection helpers.
//------------------------------------------------------------------------------

#include "antidromicspikedetection.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void testDetectLatencyNegativeToPositive()
{
    antidromicSpikeDetection::AntidromicDetectionSettings settings;
    settings.threshold = 0.0;
    settings.windowCenterMs = 2.0;
    settings.windowWidthMs = 4.0;
    settings.polarity = antidromicSpikeDetection::AntidromicCrossingPolarity::NegativeToPositive;
    settings.sampleRateHz = 1000.0;
    settings.totalTraceTimeMs = 10.0;

    std::vector<float> samples(11);
    for (int i = 0; i < 11; ++i) {
        const double timeMs = static_cast<double>(i - 5);
        samples[static_cast<std::size_t>(i)] = static_cast<float>(timeMs < 1.0 ? -1.0 : 1.0);
    }

    const std::optional<double> latency = antidromicSpikeDetection::detectLatencyMs(samples, settings);
    expect(latency.has_value(), "crossing detected");
    if (latency.has_value()) {
        expect(std::abs(*latency - 1.0) < 0.01, "latency near 1 ms");
    }
}

void testHeldTraceStats()
{
    const std::vector<std::optional<double>> latencies = {1.0, 2.0, std::nullopt};
    const antidromicSpikeDetection::AntidromicHeldTraceStats stats =
        antidromicSpikeDetection::computeHeldTraceStats(latencies);
    expect(stats.numTraces == 3, "trace count");
    expect(stats.numHits == 2, "hit count");
    expect(std::abs(stats.hitRatePercent - (200.0 / 3.0)) < 0.01, "hit rate");
    expect(stats.meanMs.has_value() && std::abs(*stats.meanMs - 1.5) < 1.0e-9, "mean latency");
    expect(stats.stdMs.has_value(), "std present with two hits");
}

} // namespace

int main()
{
    testDetectLatencyNegativeToPositive();
    testHeldTraceStats();
    if (failures == 0) {
        std::cout << "All antidromic spike detection golden tests passed.\n";
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
