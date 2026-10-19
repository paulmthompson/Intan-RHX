//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#ifndef ANTIDROMICSPIKEDETECTION_HPP
#define ANTIDROMICSPIKEDETECTION_HPP

#include <optional>
#include <string>
#include <vector>

namespace antidromicSpikeDetection {

enum class AntidromicCrossingPolarity {
    NegativeToPositive,
    PositiveToNegative,
};

struct AntidromicDetectionSettings
{
    double threshold = 0.0;
    double windowCenterMs = 0.0;
    double windowWidthMs = 1.0;
    AntidromicCrossingPolarity polarity = AntidromicCrossingPolarity::NegativeToPositive;
    double sampleRateHz = 0.0;
    double totalTraceTimeMs = 20.0;
};

struct AntidromicHeldTraceStats
{
    int numTraces = 0;
    int numHits = 0;
    double hitRatePercent = 0.0;
    std::optional<double> meanMs;
    std::optional<double> stdMs;
};

/**
 * @brief Find the first threshold crossing in the search window; latency is ms from stim onset (t = 0).
 * @pre samples is non-empty and matches totalTraceTimeMs at sampleRateHz.
 * @post Returns latency in ms when a crossing is found inside the window.
 */
std::optional<double> detectLatencyMs(const std::vector<float>& samples, const AntidromicDetectionSettings& settings);

/**
 * @brief Aggregate hit rate and latency statistics over held traces.
 */
AntidromicHeldTraceStats computeHeldTraceStats(const std::vector<std::optional<double>>& latenciesMs);

/**
 * @brief Parse crossing polarity from SystemState discrete item value name.
 */
AntidromicCrossingPolarity crossingPolarityFromValueName(const std::string& valueName);

} // namespace antidromicSpikeDetection

#endif // ANTIDROMICSPIKEDETECTION_HPP
