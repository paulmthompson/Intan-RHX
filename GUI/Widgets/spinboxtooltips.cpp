//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//  Version 3.2.0
//
//  Copyright (c) 2020-2023 Intan Technologies
//
//  This file is part of the Intan Technologies RHX Data Acquisition Software.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published
//  by the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//  This software is provided 'as-is', without any express or implied warranty.
//  In no event will the authors be held liable for any damages arising from
//  the use of this software.
//
//  See <http://www.intantech.com> for documentation and product information.
//
//------------------------------------------------------------------------------

#include "spinboxtooltips.hpp"

#include "rhxglobals.h"
#include "smartspinbox.h"
#include "voltagespinbox.h"

#include <QLabel>
#include <QObject>
#include <QSpinBox>
#include <QWidget>

namespace {

/**
 * @brief Formats a duration in microseconds for tooltip display.
 */
QString formatMicrosecondsForTooltip(double microseconds)
{
    if (microseconds >= 1.0e6) {
        return QObject::tr("%1 s").arg(microseconds / 1.0e6, 0, 'g', 6);
    }
    if (microseconds >= 1000.0) {
        return QObject::tr("%1 ms").arg(microseconds / 1000.0, 0, 'g', 6);
    }
    return QObject::tr("%1 µs").arg(microseconds, 0, 'f', 0);
}

/**
 * @brief Applies the same range hint to a label and spin box widgets.
 */
void applyRangeToolTip(QLabel* label, QWidget* spinWrapper, QWidget* spinEditor, const QString& toolTipText)
{
    if (label != nullptr) {
        label->setToolTip(toolTipText);
        label->setAccessibleDescription(toolTipText);
    }
    if (spinWrapper != nullptr) {
        spinWrapper->setToolTip(toolTipText);
        spinWrapper->setAccessibleDescription(toolTipText);
    }
    if (spinEditor != nullptr && spinEditor != spinWrapper) {
        spinEditor->setToolTip(toolTipText);
        spinEditor->setAccessibleDescription(toolTipText);
    }
}

} // namespace

void configureMicrosecondSpinLimits(TimeSpinBox* spin, QLabel* label,
                                    double minUs, double maxUs, double timestepUs)
{
    spin->setRange(minUs, maxUs);

    const QString minFormatted = formatMicrosecondsForTooltip(minUs);
    const QString maxFormatted = formatMicrosecondsForTooltip(maxUs);
    const QString timestepFormatted = formatMicrosecondsForTooltip(timestepUs);

    const QString toolTipText = QObject::tr(
        "Allowed range: %1 to %2 (%3 to %4 µs). "
        "Values are rounded to the sample period (%5).")
                                    .arg(minFormatted)
                                    .arg(maxFormatted)
                                    .arg(minUs, 0, 'f', 0)
                                    .arg(maxUs, 0, 'f', 0)
                                    .arg(timestepFormatted);

    applyRangeToolTip(label, spin, spin->pointer(), toolTipText);
}

void configureCurrentSpinLimits(CurrentSpinBox* spin, QLabel* label,
                                double minMicroAmps, double maxMicroAmps, double stepMicroAmps)
{
    spin->setRange(minMicroAmps, maxMicroAmps);

    const QString toolTipText = QObject::tr(
        "Allowed range: %1 to %2 %3. "
        "Values are rounded to the current step size (%4 %3).")
                                    .arg(minMicroAmps, 0, 'g', 6)
                                    .arg(maxMicroAmps, 0, 'g', 6)
                                    .arg(MicroAmpsSymbol)
                                    .arg(stepMicroAmps, 0, 'g', 6);

    applyRangeToolTip(label, spin, spin->pointer(), toolTipText);
}

void configurePlainSpinLimits(QSpinBox* spin, QLabel* label,
                              int min, int max, const QString& unitDescription)
{
    spin->setRange(min, max);

    const QString toolTipText = QObject::tr("Allowed range: %1 to %2 %3.")
                                    .arg(min)
                                    .arg(max)
                                    .arg(unitDescription);

    applyRangeToolTip(label, spin, spin, toolTipText);
}

void configureVoltageSpinLimits(VoltageSpinBox* spin, QLabel* label,
                                double minVolts, double maxVolts)
{
    spin->setRange(minVolts, maxVolts);

    const QString toolTipText = QObject::tr("Allowed range: %1 to %2 V.")
                                    .arg(minVolts, 0, 'g', 6)
                                    .arg(maxVolts, 0, 'g', 6);

    applyRangeToolTip(label, spin, spin->pointer(), toolTipText);
}
