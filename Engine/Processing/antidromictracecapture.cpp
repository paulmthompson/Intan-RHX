//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#include "antidromictracecapture.hpp"

#include "pulseelectroderesistance.hpp"

#include <cmath>

namespace antidromicTraceCapture {

namespace {

constexpr uint16_t kStimOnFlag = pulseElectrodeResistance::StimOnFlag;

int halfWindowSamplesFromTotalTimeMs(double totalTimeMs, double sampleRateHz)
{
    if (totalTimeMs <= 0.0 || sampleRateHz <= 0.0) {
        return 0;
    }
    return static_cast<int>(std::lround((totalTimeMs * 0.5) * sampleRateHz / 1000.0));
}

std::optional<AntidromicTraceSnippet> extractSnippetAtOnset(int onsetIndex, int halfWindowSamples, int numWordsInMemory,
                                                            const std::function<float(int)>& getSample,
                                                            const std::function<uint32_t(int)>& getTimeStamp)
{
    const int windowStart = onsetIndex - halfWindowSamples;
    const int windowEnd = onsetIndex + halfWindowSamples;
    if (windowStart < -numWordsInMemory) {
        return std::nullopt;
    }

    const int snippetLength = 2 * halfWindowSamples;
    AntidromicTraceSnippet snippet;
    snippet.samples.resize(static_cast<std::size_t>(snippetLength));
    snippet.onsetIndex = onsetIndex;

    int outIndex = 0;
    for (int i = windowStart; i < windowEnd; ++i) {
        snippet.samples[static_cast<std::size_t>(outIndex)] = getSample(i);
        ++outIndex;
    }
    snippet.timestamp = getTimeStamp(onsetIndex);
    return snippet;
}

} // namespace

std::optional<AntidromicTraceSnippet> tryCaptureStimAlignedTrace(
    int numSamples,
    double sampleRateHz,
    double totalTimeMs,
    int numWordsInMemory,
    AntidromicTraceCaptureState& captureState,
    const std::function<uint16_t(int timeIndex)>& getStimFlags,
    const std::function<float(int timeIndex)>& getSample,
    const std::function<uint32_t(int timeIndex)>& getTimeStamp)
{
    if (numSamples <= 0 || sampleRateHz <= 0.0 || totalTimeMs <= 0.0) {
        return std::nullopt;
    }

    const int halfWindowSamples = halfWindowSamplesFromTotalTimeMs(totalTimeMs, sampleRateHz);
    if (halfWindowSamples <= 0) {
        return std::nullopt;
    }

    std::optional<AntidromicTraceSnippet> captured;

    for (int t = 0; t < numSamples; ++t) {
        const uint16_t stimFlags = getStimFlags(t);
        const bool stimOn = (stimFlags & kStimOnFlag) != 0;

        const bool stimOnset = stimOn && !captureState.lastStimOn;
        if (stimOnset) {
            if (halfWindowSamples <= 1) {
                captured = extractSnippetAtOnset(t, halfWindowSamples, numWordsInMemory, getSample, getTimeStamp);
                captureState.samplesUntilCapture = -1;
            } else {
                captureState.samplesUntilCapture = halfWindowSamples - 1;
            }
        } else if (captureState.samplesUntilCapture > 0) {
            --captureState.samplesUntilCapture;
            if (captureState.samplesUntilCapture == 0) {
                const int onsetIndex = t - (halfWindowSamples - 1);
                captured = extractSnippetAtOnset(onsetIndex, halfWindowSamples, numWordsInMemory, getSample,
                                                getTimeStamp);
                captureState.samplesUntilCapture = -1;
            }
        }

        captureState.lastStimOn = stimOn;
    }

    return captured;
}

} // namespace antidromicTraceCapture
