//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#include "boardadcspikedetection.hpp"

#include "boardadcscaling.hpp"

namespace boardAdcSpikeDetection {

std::optional<float> boardAdcFifoSampleToElectrodeMicrovolts(float adcVolts, double externalAmplifierGain)
{
    const std::optional<double> microvolts =
        boardAdcScaling::electrodeMicrovoltsFromBoardAdcVolts(static_cast<double>(adcVolts), externalAmplifierGain);
    if (!microvolts.has_value()) {
        return std::nullopt;
    }
    return static_cast<float>(*microvolts);
}

bool crossedSpikeThresholdNegative(float previousMicroVolts, float currentMicroVolts, int thresholdMicroVolts)
{
    const float threshold = static_cast<float>(thresholdMicroVolts);
    return previousMicroVolts > threshold && currentMicroVolts <= threshold;
}

} // namespace boardAdcSpikeDetection
