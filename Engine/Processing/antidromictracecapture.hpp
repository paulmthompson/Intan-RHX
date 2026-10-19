//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#ifndef ANTIDROMICTRACECAPTURE_HPP
#define ANTIDROMICTRACECAPTURE_HPP

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace antidromicTraceCapture {

struct AntidromicTraceSnippet
{
    std::vector<float> samples;
    uint32_t timestamp = 0;
    int onsetIndex = 0;
};

struct AntidromicTraceCaptureState
{
    bool lastStimOn = false;
    /** Samples remaining until capture after stim onset; -1 when idle. */
    int samplesUntilCapture = -1;
};

/**
 * @brief Capture a symmetric trace window centered on stim onset on another channel.
 *
 * @pre getStimFlags and getSample valid for timeIndex in [-numWordsInMemory, numSamples).
 * @post captureState updated; optional snippet when a complete window is available.
 */
std::optional<AntidromicTraceSnippet> tryCaptureStimAlignedTrace(
    int numSamples,
    double sampleRateHz,
    double totalTimeMs,
    int numWordsInMemory,
    AntidromicTraceCaptureState& captureState,
    const std::function<uint16_t(int timeIndex)>& getStimFlags,
    const std::function<float(int timeIndex)>& getSample,
    const std::function<uint32_t(int timeIndex)>& getTimeStamp);

} // namespace antidromicTraceCapture

#endif // ANTIDROMICTRACECAPTURE_HPP
