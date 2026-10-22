//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#ifndef BOARDADCSPIKEDETECTION_HPP
#define BOARDADCSPIKEDETECTION_HPP

#include <optional>

namespace boardAdcSpikeDetection {

/**
 * @brief Convert a board ADC FIFO sample (volts at connector) to electrode µV.
 * @pre externalAmplifierGain > 0
 * @post Returns scaled microvolts when gain is valid.
 */
std::optional<float> boardAdcFifoSampleToElectrodeMicrovolts(float adcVolts, double externalAmplifierGain);

/**
 * @brief True when the waveform crosses downward through a negative spike threshold (µV at electrode).
 * @pre previousMicroVolts and currentMicroVolts are in electrode µV.
 */
bool crossedSpikeThresholdNegative(float previousMicroVolts, float currentMicroVolts, int thresholdMicroVolts);

} // namespace boardAdcSpikeDetection

#endif // BOARDADCSPIKEDETECTION_HPP
