//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#ifndef BOARDADCSCALING_HPP
#define BOARDADCSCALING_HPP

#include <optional>

namespace boardAdcScaling {

/**
 * @brief Physical voltage at the electrode from board ADC volts and external amplifier gain.
 * @pre externalAmplifierGain > 0
 * @post Returns adcVolts / externalAmplifierGain when gain is valid.
 */
std::optional<double> physicalVoltsFromBoardAdcVolts(double adcVolts, double externalAmplifierGain);

/**
 * @brief Electrode-referenced amplitude in microvolts from board ADC volts and gain.
 * @pre externalAmplifierGain > 0
 * @post Returns adcVolts * 1e6 / externalAmplifierGain when gain is valid.
 */
std::optional<double> electrodeMicrovoltsFromBoardAdcVolts(double adcVolts, double externalAmplifierGain);

} // namespace boardAdcScaling

#endif // BOARDADCSCALING_HPP
