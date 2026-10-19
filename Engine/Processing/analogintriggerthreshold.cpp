//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#include "analogintriggerthreshold.hpp"

#include <algorithm>

#include <QDoubleSpinBox>
#include <QString>

bool isAnalogInStimTriggerSource(int triggerSourceIndex)
{
    return triggerSourceIndex >= 16 && triggerSourceIndex < 24;
}

int analogInTriggerThresholdVoltsToCounts(double voltageVolts)
{
    const double scaled =
        32768.0 * (voltageVolts / kAnalogInTriggerFullScaleVolts) + 32768.0;
    int value = static_cast<int>(scaled);
    if (value < 0) {
        value = 0;
    } else if (value > 65535) {
        value = 65535;
    }
    return value;
}

double analogInTriggerThresholdCountsToVolts(int counts)
{
    const int clamped = std::clamp(counts, 0, 65535);
    return (static_cast<double>(clamped) - 32768.0) * kAnalogInTriggerFullScaleVolts /
           32768.0;
}

double quantizeAnalogInTriggerThresholdVolts(double voltageVolts)
{
    return analogInTriggerThresholdCountsToVolts(
        analogInTriggerThresholdVoltsToCounts(voltageVolts));
}

void configureAnalogInTriggerThresholdSpinBox(QDoubleSpinBox* spinBox)
{
    if (!spinBox) {
        return;
    }
    spinBox->setMinimum(-kAnalogInTriggerFullScaleVolts);
    spinBox->setMaximum(kAnalogInTriggerFullScaleVolts);
    spinBox->setDecimals(4);
    spinBox->setSingleStep(kAnalogInTriggerThresholdSpinStepVolts);
    spinBox->setSuffix(QStringLiteral(" V"));
    spinBox->setMinimumWidth(
        spinBox->fontMetrics().horizontalAdvance(QStringLiteral("-10.2400 V")) + 28);
}

QString analogInTriggerThresholdShortSummary()
{
    return QStringLiteral(
        "Shared by all analog inputs (0.3125 mV steps). Not DAC threshold on TTL 1–8.");
}

QString analogInTriggerThresholdDetailsText()
{
    return QStringLiteral(
        "This threshold applies to every board analog input and to all amplifier "
        "stimulation and digital-output programs that use any ANALOG IN trigger. "
        "It is not the DAC-threshold mode on TTL outputs 1–8 in the Analog tab. "
        "The FPGA fires when voltage is above this level; use Trigger on Low to "
        "respond to the inverted line. Host recording, episodic triggers, and PSTH "
        "use the same TriggerAnalogThresholdVolts value when synced to hardware.");
}
