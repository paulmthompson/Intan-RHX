//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#include "boardadcscaling.hpp"

namespace boardAdcScaling {

std::optional<double> physicalVoltsFromBoardAdcVolts(double adcVolts, double externalAmplifierGain)
{
    if (externalAmplifierGain <= 0.0) {
        return std::nullopt;
    }
    return adcVolts / externalAmplifierGain;
}

std::optional<double> electrodeMicrovoltsFromBoardAdcVolts(double adcVolts, double externalAmplifierGain)
{
    if (externalAmplifierGain <= 0.0) {
        return std::nullopt;
    }
    return adcVolts * 1.0e6 / externalAmplifierGain;
}

} // namespace boardAdcScaling
