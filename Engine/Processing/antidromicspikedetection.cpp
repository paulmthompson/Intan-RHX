//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#include "antidromicspikedetection.hpp"

#include <cmath>

namespace antidromicSpikeDetection {

namespace {

bool crossesThreshold(float previousSample, float currentSample, double threshold, AntidromicCrossingPolarity polarity)
{
    switch (polarity) {
    case AntidromicCrossingPolarity::NegativeToPositive:
        return previousSample < threshold && currentSample >= threshold;
    case AntidromicCrossingPolarity::PositiveToNegative:
        return previousSample > threshold && currentSample <= threshold;
    }
    return false;
}

double interpolateCrossingTimeMs(double previousTimeMs, double currentTimeMs, float previousSample, float currentSample,
                                 double threshold)
{
    const double denominator = static_cast<double>(currentSample) - static_cast<double>(previousSample);
    if (std::abs(denominator) < 1.0e-12) {
        return currentTimeMs;
    }
    const double fraction = (threshold - static_cast<double>(previousSample)) / denominator;
    return previousTimeMs + fraction * (currentTimeMs - previousTimeMs);
}

} // namespace

std::optional<double> detectLatencyMs(const std::vector<float>& samples, const AntidromicDetectionSettings& settings)
{
    if (samples.size() < 2 || settings.sampleRateHz <= 0.0 || settings.totalTraceTimeMs <= 0.0) {
        return std::nullopt;
    }

    const double dtMs = 1000.0 / settings.sampleRateHz;
    const int halfSamples = static_cast<int>(samples.size()) / 2;

    const double windowHalfWidth = settings.windowWidthMs * 0.5;
    double windowStartMs = settings.windowCenterMs - windowHalfWidth;
    double windowEndMs = settings.windowCenterMs + windowHalfWidth;

    const double traceMinMs = -settings.totalTraceTimeMs * 0.5;
    const double traceMaxMs = settings.totalTraceTimeMs * 0.5;
    windowStartMs = std::max(windowStartMs, traceMinMs);
    windowEndMs = std::min(windowEndMs, traceMaxMs);
    if (windowEndMs <= windowStartMs) {
        return std::nullopt;
    }

    for (std::size_t i = 1; i < samples.size(); ++i) {
        const double previousTimeMs = (static_cast<int>(i) - 1 - halfSamples) * dtMs;
        const double currentTimeMs = (static_cast<int>(i) - halfSamples) * dtMs;

        if (!crossesThreshold(samples[i - 1], samples[i], settings.threshold, settings.polarity)) {
            continue;
        }

        const double crossingTimeMs =
            interpolateCrossingTimeMs(previousTimeMs, currentTimeMs, samples[i - 1], samples[i], settings.threshold);
        if (crossingTimeMs >= windowStartMs && crossingTimeMs <= windowEndMs) {
            return crossingTimeMs;
        }
    }

    return std::nullopt;
}

AntidromicHeldTraceStats computeHeldTraceStats(const std::vector<std::optional<double>>& latenciesMs)
{
    AntidromicHeldTraceStats stats;
    stats.numTraces = static_cast<int>(latenciesMs.size());

    std::vector<double> hits;
    hits.reserve(latenciesMs.size());
    for (const std::optional<double>& latency : latenciesMs) {
        if (latency.has_value()) {
            hits.push_back(*latency);
        }
    }

    stats.numHits = static_cast<int>(hits.size());
    if (stats.numTraces > 0) {
        stats.hitRatePercent = 100.0 * static_cast<double>(stats.numHits) / static_cast<double>(stats.numTraces);
    }

    if (hits.empty()) {
        return stats;
    }

    double sum = 0.0;
    for (double value : hits) {
        sum += value;
    }
    stats.meanMs = sum / static_cast<double>(hits.size());

    if (hits.size() >= 2) {
        double sumSquaredError = 0.0;
        for (double value : hits) {
            const double delta = value - *stats.meanMs;
            sumSquaredError += delta * delta;
        }
        stats.stdMs = std::sqrt(sumSquaredError / static_cast<double>(hits.size() - 1));
    }

    return stats;
}

AntidromicCrossingPolarity crossingPolarityFromValueName(const std::string& valueName)
{
    if (valueName == "PositiveToNegative") {
        return AntidromicCrossingPolarity::PositiveToNegative;
    }
    return AntidromicCrossingPolarity::NegativeToPositive;
}

} // namespace antidromicSpikeDetection
