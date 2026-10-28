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

#include "pulseresponseplot.h"

#include <algorithm>
#include <cmath>

#include "rhxglobals.h"

namespace {

QString formatAxisValue(double value)
{
    if (std::abs(value - std::round(value)) < 1.0e-6) {
        return QString::number(static_cast<qlonglong>(std::llround(value)));
    }
    return QString::number(value, 'f', 1);
}

int leftMarginForYLabels(const QFontMetrics& fontMetrics, double yHalfRange, const QString& unitSuffix)
{
    const QString positiveLabel = "+" + formatAxisValue(yHalfRange) + " " + unitSuffix;
    const QString negativeLabel = formatAxisValue(-yHalfRange) + " " + unitSuffix;
    const int labelWidth = std::max(fontMetrics.horizontalAdvance(positiveLabel),
                                    fontMetrics.horizontalAdvance(negativeLabel));
    return labelWidth + 10;
}

} // namespace

PulseResponsePlot::PulseResponsePlot(SystemState* state_, QWidget* parent) :
    QWidget(parent),
    state(state_),
    _tStepMsec(0.0)
{
    connect(state, SIGNAL(stateChanged()), this, SLOT(updateFromState()));

    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    _image = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    _image.fill(Qt::black);
    updateFromState();
}

void PulseResponsePlot::setLatestSnippet(const pulseElectrodeResistance::PulseResponseSnippet& snippet)
{
    _snippet = snippet;
    update();
}

void PulseResponsePlot::clearSnippet()
{
    _snippet.reset();
    update();
}

void PulseResponsePlot::updateFromState()
{
    const double sampleRateHz = state->sampleRate->getNumericValue();
    if (sampleRateHz > 0.0) {
        _tStepMsec = 1000.0 / sampleRateHz;
    }
    update();
}

void PulseResponsePlot::resizeEvent(QResizeEvent*)
{
    _image = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    _image.fill(Qt::black);
    update();
}

void PulseResponsePlot::updatePanelFrames()
{
    const QRect imageFrame(rect());
    const int panelGap = 6;
    const int halfHeight = (imageFrame.height() - panelGap) / 2;
    _acPanelFrame = QRect(imageFrame.left(), imageFrame.top(), imageFrame.width(), halfHeight);
    _dcPanelFrame = QRect(imageFrame.left(), imageFrame.top() + halfHeight + panelGap, imageFrame.width(),
                          imageFrame.bottom() - (_acPanelFrame.bottom() + panelGap) + 1);
}

void PulseResponsePlot::updateCoordinateTranslators()
{
    const double totalTimeMs = state->pulseResistanceTraceTimeMs->getNumericValue();
    const double tMin = -totalTimeMs * 0.5;
    const double tMax = totalTimeMs * 0.5;
    const bool externalMode = state->pulseResistanceUsesExternalAdcTtl();
    const double acY = state->pulseResistanceTraceYScaleAc->getNumericValue();
    const double externalTraceY = state->pulseResistanceExternalTraceYScaleVolts->getNumericValue();
    const double dcY = state->pulseResistanceTraceYScaleDc->getNumericValue();
    const double upperY = externalMode ? externalTraceY : acY;
    const QString upperUnit = externalMode ? QStringLiteral("V") : MicroVoltsSymbol;

    const QFontMetrics fm = fontMetrics();
    const int acLeftMargin = leftMarginForYLabels(fm, upperY, upperUnit);
    const int dcLeftMargin = leftMarginForYLabels(fm, dcY, QStringLiteral("V"));
    const int textBoxHeight = fm.height();
    const int bottomMargin = textBoxHeight + 8;
    const QRect acPlot = _acPanelFrame.adjusted(acLeftMargin, textBoxHeight + 8, -8, -bottomMargin);
    const QRect dcPlot = _dcPanelFrame.adjusted(dcLeftMargin, textBoxHeight + 8, -8, -bottomMargin);

    _acCt.set(acPlot, tMin, tMax, -upperY, upperY);
    _dcCt.set(dcPlot, tMin, tMax, -dcY, dcY);
}

void PulseResponsePlot::paintPanel(QPainter& painter, const QRect& panelFrame, CoordinateTranslator& ct,
                                   const QString& title, const std::vector<float>& samples, double yHalfRange,
                                   const QString& yUnitSuffix, bool dataAvailable, const QString& unavailableMessage)
{
    painter.setPen(Qt::darkGray);
    painter.drawRect(panelFrame.adjusted(0, 0, -1, -1));

    PlotDecorator plotDecorator(painter);
    painter.setPen(Qt::white);
    plotDecorator.writeLabel(title, panelFrame.left() + 4, panelFrame.top() + 2, Qt::AlignLeft | Qt::AlignTop);

    painter.drawRect(ct.borderRect());
    painter.setClipRect(ct.clippingRect());

    if (dataAvailable && !samples.empty()) {
        const int halfSamples = static_cast<int>(samples.size()) / 2;
        const double timeOriginMsec = -halfSamples * _tStepMsec;

        QPointF* polyline = new QPointF[static_cast<int>(samples.size())];
        double timeMsec = timeOriginMsec;
        for (std::size_t i = 0; i < samples.size(); ++i) {
            polyline[static_cast<int>(i)] = QPointF(ct.screenXFromRealX(timeMsec), ct.screenYFromRealY(samples[i]));
            timeMsec += _tStepMsec;
        }
        painter.setPen(QColor(0, 200, 255));
        painter.drawPolyline(polyline, static_cast<int>(samples.size()));
        delete[] polyline;
    }

    painter.setClipping(false);

    if (!dataAvailable || samples.empty()) {
        painter.setPen(Qt::gray);
        plotDecorator.writeLabel(unavailableMessage, ct.xLeft() + 4, (ct.yTop() + ct.yBottom()) / 2,
                                 Qt::AlignLeft | Qt::AlignVCenter);
    }

    painter.setPen(Qt::white);
    plotDecorator.drawHorizontalAxisLine(ct, 0.0);
    plotDecorator.drawVerticalAxisLine(ct, 0.0);

    plotDecorator.drawLabeledTickMarkLeft("+" + formatAxisValue(yHalfRange) + " " + yUnitSuffix, ct, yHalfRange, 0);
    plotDecorator.drawLabeledTickMarkLeft(formatAxisValue(-yHalfRange) + " " + yUnitSuffix, ct, -yHalfRange, 0);

    const double tMin = ct.xMinReal();
    const double tMax = ct.xMaxReal();
    plotDecorator.drawLabeledTickMarkBottom(formatAxisValue(tMin), ct, tMin, 0, false);
    if (tMin < 0.0 && tMax > 0.0) {
        plotDecorator.drawLabeledTickMarkBottom(QStringLiteral("0"), ct, 0.0, 0, false);
    }
    plotDecorator.drawLabeledTickMarkBottom(formatAxisValue(tMax) + QStringLiteral(" ms"), ct, tMax, 0, true);
}

void PulseResponsePlot::paintEvent(QPaintEvent*)
{
    QPainter painter(&_image);
    painter.fillRect(rect(), Qt::black);

    updatePanelFrames();
    updateCoordinateTranslators();

    const bool externalMode = state->pulseResistanceUsesExternalAdcTtl();
    const double acY = state->pulseResistanceTraceYScaleAc->getNumericValue();
    const double externalTraceY = state->pulseResistanceExternalTraceYScaleVolts->getNumericValue();
    const double dcY = state->pulseResistanceTraceYScaleDc->getNumericValue();
    const double upperY = externalMode ? externalTraceY : acY;

    if (_snippet.has_value()) {
        const auto& snippet = *_snippet;
        if (externalMode) {
            paintPanel(painter, _acPanelFrame, _acCt, tr("Trace ADC (V)"), snippet.m_traceVolts, upperY, tr("V"),
                       snippet.m_traceAvailable, tr("No trace ADC"));
            paintPanel(painter, _dcPanelFrame, _dcCt, tr("Resistance ADC (V)"), snippet.m_dcVolts, dcY, tr("V"),
                       !snippet.m_dcVolts.empty(), tr("No resistance trace"));
        } else {
            paintPanel(painter, _acPanelFrame, _acCt, tr("AC (high-pass)"), snippet.m_acMicroVolts, acY,
                       MicroVoltsSymbol, snippet.m_acAvailable, tr("AC unavailable"));
            paintPanel(painter, _dcPanelFrame, _dcCt, tr("DC"), snippet.m_dcVolts, dcY, tr("V"),
                       !snippet.m_dcVolts.empty(), tr("No DC trace"));
        }
    } else {
        if (externalMode) {
            paintPanel(painter, _acPanelFrame, _acCt, tr("Trace ADC (V)"), {}, upperY, tr("V"), false,
                       tr("Waiting for stim pulse..."));
            paintPanel(painter, _dcPanelFrame, _dcCt, tr("Resistance ADC (V)"), {}, dcY, tr("V"), false,
                       tr("Waiting for stim pulse..."));
        } else {
            paintPanel(painter, _acPanelFrame, _acCt, tr("AC (high-pass)"), {}, acY, MicroVoltsSymbol, false,
                       tr("Waiting for stim pulse..."));
            paintPanel(painter, _dcPanelFrame, _dcCt, tr("DC"), {}, dcY, tr("V"), false, tr("Waiting for stim pulse..."));
        }
    }

    QPainter widgetPainter(this);
    widgetPainter.drawImage(0, 0, _image);
}
