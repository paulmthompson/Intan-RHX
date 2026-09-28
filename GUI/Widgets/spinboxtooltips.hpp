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

#ifndef SPINBOXTOOLTIPS_HPP
#define SPINBOXTOOLTIPS_HPP

#include <QString>

class QLabel;
class QSpinBox;
class CurrentSpinBox;
class TimeSpinBox;
class VoltageSpinBox;

/**
 * @brief Sets TimeSpinBox range and matching range tooltips on the label and spin box.
 * @param spin Time spin box to configure.
 * @param label Companion label (may be null).
 * @param minUs Minimum allowed value in microseconds.
 * @param maxUs Maximum allowed value in microseconds.
 * @param timestepUs Sample period in microseconds (quantization step).
 * @pre spin is non-null.
 * @post spin range and tooltips reflect minUs, maxUs, and timestepUs.
 */
void configureMicrosecondSpinLimits(TimeSpinBox* spin, QLabel* label,
                                    double minUs, double maxUs, double timestepUs);

/**
 * @brief Sets CurrentSpinBox range and matching range tooltips.
 * @param spin Current spin box to configure.
 * @param label Companion label (may be null).
 * @param minMicroAmps Minimum allowed current in microamps.
 * @param maxMicroAmps Maximum allowed current in microamps.
 * @param stepMicroAmps Current quantisation step in microamps.
 * @pre spin is non-null.
 * @post spin range and tooltips reflect the given limits.
 */
void configureCurrentSpinLimits(CurrentSpinBox* spin, QLabel* label,
                                double minMicroAmps, double maxMicroAmps, double stepMicroAmps);

/**
 * @brief Sets QSpinBox range and matching range tooltips.
 * @param spin Integer spin box to configure.
 * @param label Companion label (may be null).
 * @param min Minimum allowed value.
 * @param max Maximum allowed value.
 * @param unitDescription Short unit phrase for the tooltip (e.g. tr("pulses")).
 * @pre spin is non-null.
 * @post spin range and tooltips reflect min and max.
 */
void configurePlainSpinLimits(QSpinBox* spin, QLabel* label,
                              int min, int max, const QString& unitDescription);

/**
 * @brief Sets VoltageSpinBox range and matching range tooltips.
 * @param spin Voltage spin box to configure.
 * @param label Companion label (may be null).
 * @param minVolts Minimum allowed voltage in volts.
 * @param maxVolts Maximum allowed voltage in volts.
 * @pre spin is non-null.
 * @post spin range and tooltips reflect the given limits.
 */
void configureVoltageSpinLimits(VoltageSpinBox* spin, QLabel* label,
                                double minVolts, double maxVolts);

#endif // SPINBOXTOOLTIPS_HPP
