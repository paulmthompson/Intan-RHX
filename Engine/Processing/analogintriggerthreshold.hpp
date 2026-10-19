//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#ifndef ANALOGINTRIGGERTHRESHOLD_HPP
#define ANALOGINTRIGGERTHRESHOLD_HPP

class QDoubleSpinBox;
class QString;

/**
 * @brief Nominal full-scale voltage for board analog-input ADC registers (±10.24 V).
 */
constexpr double kAnalogInTriggerFullScaleVolts = 10.24;

/**
 * @brief Voltage change per one ADC-threshold count (WireIn 0x0f LSB).
 */
constexpr double kAnalogInTriggerThresholdVoltsPerCount =
    kAnalogInTriggerFullScaleVolts / 32768.0;

/**
 * @brief Preferred QDoubleSpinBox step matching one hardware count.
 */
constexpr double kAnalogInTriggerThresholdSpinStepVolts = 1.0 / 3200.0;

/**
 * @brief True when a stim/digout trigger index selects ANALOG IN 1–8.
 * @param triggerSourceIndex StimParameters trigger source index (16–23).
 */
bool isAnalogInStimTriggerSource(int triggerSourceIndex);

/**
 * @brief Encode volts to the 16-bit ADC threshold written to the FPGA.
 * @pre Uses the same truncation as RHXController::setAnalogInTriggerThreshold.
 */
int analogInTriggerThresholdVoltsToCounts(double voltageVolts);

/**
 * @brief Decode a 16-bit ADC threshold count to volts.
 */
double analogInTriggerThresholdCountsToVolts(int counts);

/**
 * @brief Round-trip volts through the hardware count encoding.
 */
double quantizeAnalogInTriggerThresholdVolts(double voltageVolts);

/**
 * @brief Configure a spin box for editable FPGA analog trigger thresholds.
 */
void configureAnalogInTriggerThresholdSpinBox(QDoubleSpinBox* spinBox);

/**
 * @brief One-line UI summary for the shared FPGA analog comparator threshold.
 */
QString analogInTriggerThresholdShortSummary();

/**
 * @brief Full explanation for tooltips (not translated here; wrap with tr() in GUI).
 */
QString analogInTriggerThresholdDetailsText();

#endif // ANALOGINTRIGGERTHRESHOLD_HPP
