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

#ifndef PULSERESPONSEPLOT_H
#define PULSERESPONSEPLOT_H

#define PULSERESPONSEPLOT_X_SIZE 580
#define PULSERESPONSEPLOT_Y_SIZE 360

#include <optional>
#include <vector>

#include <QtWidgets>

#include "plotutilities.h"
#include "pulseelectroderesistance.hpp"
#include "systemstate.h"

class PulseResponsePlot : public QWidget
{
    Q_OBJECT
public:
    explicit PulseResponsePlot(SystemState* state_, QWidget* parent = nullptr);

    /**
     * @brief Replace the displayed peri-pulse snippet.
     * @post Widget repaints when snippet is valid.
     */
    void setLatestSnippet(const pulseElectrodeResistance::PulseResponseSnippet& snippet);

    /**
     * @brief Clear the trace display.
     */
    void clearSnippet();

    QSize minimumSizeHint() const override { return QSize(PULSERESPONSEPLOT_X_SIZE, PULSERESPONSEPLOT_Y_SIZE); }
    QSize sizeHint() const override { return QSize(PULSERESPONSEPLOT_X_SIZE, PULSERESPONSEPLOT_Y_SIZE); }

public slots:
    void updateFromState();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updatePanelFrames();
    void updateCoordinateTranslators();
    void paintPanel(QPainter& painter, const QRect& panelFrame, CoordinateTranslator& ct, const QString& title,
                    const std::vector<float>& samples, double yHalfRange, const QString& yUnitSuffix,
                    bool dataAvailable, const QString& unavailableMessage);

    SystemState* state;
    std::optional<pulseElectrodeResistance::PulseResponseSnippet> _snippet;
    double _tStepMsec;

    QRect _acPanelFrame;
    QRect _dcPanelFrame;
    QImage _image;

    CoordinateTranslator _acCt;
    CoordinateTranslator _dcCt;
};

#endif
