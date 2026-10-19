//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#include "antidromictraceplot.h"

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

AntidromicTracePlot::AntidromicTracePlot(SystemState* state, QWidget* parent)
    : QWidget(parent)
    , _state(state)
    , _boardAdcRecording(false)
    , _tStepMsec(0.0)
{
    connect(_state, SIGNAL(stateChanged()), this, SLOT(updateFromState()));

    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    _image = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    _image.fill(Qt::black);
    updateFromState();
}

void AntidromicTracePlot::setBoardAdcRecording(bool boardAdc)
{
    _boardAdcRecording = boardAdc;
    update();
}

void AntidromicTracePlot::setLatestTrace(const antidromicTraceCapture::AntidromicTraceSnippet& snippet)
{
    _latestTrace = snippet;
    update();
}

void AntidromicTracePlot::setHeldTraces(const std::deque<antidromicTraceCapture::AntidromicTraceSnippet>& traces)
{
    _heldTraces = traces;
    update();
}

void AntidromicTracePlot::clearTraces()
{
    _latestTrace.reset();
    _heldTraces.clear();
    update();
}

QSize AntidromicTracePlot::minimumSizeHint() const
{
    return QSize(ANTIDROMICTRACEPLOT_X_SIZE, ANTIDROMICTRACEPLOT_Y_SIZE);
}

QSize AntidromicTracePlot::sizeHint() const
{
    return QSize(ANTIDROMICTRACEPLOT_X_SIZE, ANTIDROMICTRACEPLOT_Y_SIZE);
}

void AntidromicTracePlot::updateFromState()
{
    const double sampleRateHz = _state->sampleRate->getNumericValue();
    if (sampleRateHz > 0.0) {
        _tStepMsec = 1000.0 / sampleRateHz;
    }
    update();
}

void AntidromicTracePlot::resizeEvent(QResizeEvent*)
{
    _image = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    _image.fill(Qt::black);
    update();
}

void AntidromicTracePlot::updateCoordinateTranslator()
{
    const QRect imageFrame(rect());
    const double totalTimeMs = _state->antidromicTraceTimeMs->getNumericValue();
    const double tMin = -totalTimeMs * 0.5;
    const double tMax = totalTimeMs * 0.5;
    const double yHalfRange = _boardAdcRecording ? _state->antidromicTraceYScaleVolts->getNumericValue()
                                                 : _state->antidromicTraceYScaleMicroVolts->getNumericValue();
    const QString yUnit = _boardAdcRecording ? QStringLiteral("V") : MicroVoltsSymbol;

    const QFontMetrics fm = fontMetrics();
    const int leftMargin = leftMarginForYLabels(fm, yHalfRange, yUnit);
    const int textBoxHeight = fm.height();
    const int bottomMargin = textBoxHeight + 8;
    _plotFrame = imageFrame.adjusted(leftMargin, textBoxHeight + 8, -8, -bottomMargin);
    _ct.set(_plotFrame, tMin, tMax, -yHalfRange, yHalfRange);
}

void AntidromicTracePlot::paintTrace(QPainter& painter, const std::vector<float>& samples, const QColor& color)
{
    if (samples.empty() || _tStepMsec <= 0.0) {
        return;
    }

    const int halfSamples = static_cast<int>(samples.size()) / 2;
    const double timeOriginMsec = -halfSamples * _tStepMsec;

    std::vector<QPointF> polyline(samples.size());
    double timeMsec = timeOriginMsec;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        polyline[i] = QPointF(_ct.screenXFromRealX(timeMsec), _ct.screenYFromRealY(samples[i]));
        timeMsec += _tStepMsec;
    }
    painter.setPen(color);
    painter.drawPolyline(polyline.data(), static_cast<int>(polyline.size()));
}

void AntidromicTracePlot::paintAxes(QPainter& painter, PlotDecorator& plotDecorator, double yHalfRange,
                                    const QString& yUnitSuffix)
{
    painter.setPen(Qt::white);
    plotDecorator.drawHorizontalAxisLine(_ct, 0.0);
    plotDecorator.drawVerticalAxisLine(_ct, 0.0);

    plotDecorator.drawLabeledTickMarkLeft("+" + formatAxisValue(yHalfRange) + " " + yUnitSuffix, _ct, yHalfRange, 0);
    plotDecorator.drawLabeledTickMarkLeft(formatAxisValue(-yHalfRange) + " " + yUnitSuffix, _ct, -yHalfRange, 0);

    const double tMin = _ct.xMinReal();
    const double tMax = _ct.xMaxReal();
    plotDecorator.drawLabeledTickMarkBottom(formatAxisValue(tMin), _ct, tMin, 0, false);
    if (tMin < 0.0 && tMax > 0.0) {
        plotDecorator.drawLabeledTickMarkBottom(QStringLiteral("0"), _ct, 0.0, 0, false);
    }
    plotDecorator.drawLabeledTickMarkBottom(formatAxisValue(tMax) + QStringLiteral(" ms"), _ct, tMax, 0, true);
}

void AntidromicTracePlot::paintEvent(QPaintEvent*)
{
    QPainter painter(&_image);
    painter.fillRect(rect(), Qt::black);

    updateCoordinateTranslator();

    const double yHalfRange = _boardAdcRecording ? _state->antidromicTraceYScaleVolts->getNumericValue()
                                                 : _state->antidromicTraceYScaleMicroVolts->getNumericValue();
    const QString yUnit = _boardAdcRecording ? QStringLiteral("V") : MicroVoltsSymbol;

    painter.setPen(Qt::darkGray);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    PlotDecorator plotDecorator(painter);
    painter.setPen(Qt::white);
    const QString title =
        _boardAdcRecording ? tr("Recording (board ADC)") : tr("Recording (HIGH band)");
    plotDecorator.writeLabel(title, rect().left() + 4, rect().top() + 2, Qt::AlignLeft | Qt::AlignTop);

    painter.drawRect(_ct.borderRect());
    painter.setClipRect(_ct.clippingRect());

    const bool holdEnabled = _state->antidromicTraceHoldEnabled->getValue();
    if (holdEnabled) {
        const int length = static_cast<int>(_heldTraces.size());
        for (int i = 0; i < length; ++i) {
            const int value = 255 - static_cast<int>(200.0 * (static_cast<double>(length - i - 1))
                                                     / static_cast<double>(std::max(length, 1)));
            paintTrace(painter, _heldTraces[static_cast<std::size_t>(i)].samples, QColor(value, 80, 80));
        }
    } else if (_latestTrace.has_value()) {
        paintTrace(painter, _latestTrace->samples, QColor(0, 200, 255));
    }

    painter.setClipping(false);

    if ((holdEnabled && _heldTraces.empty()) || (!holdEnabled && !_latestTrace.has_value())) {
        painter.setPen(Qt::gray);
        plotDecorator.writeLabel(tr("Waiting for stim pulse on stimulation channel..."),
                                 _ct.xLeft() + 4, (_ct.yTop() + _ct.yBottom()) / 2, Qt::AlignLeft | Qt::AlignVCenter);
    }

    paintAxes(painter, plotDecorator, yHalfRange, yUnit);

    QPainter widgetPainter(this);
    widgetPainter.drawImage(0, 0, _image);
}
