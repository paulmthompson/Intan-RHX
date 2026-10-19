//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#ifndef ANTIDROMICTRACEPLOT_H
#define ANTIDROMICTRACEPLOT_H

#define ANTIDROMICTRACEPLOT_X_SIZE 580
#define ANTIDROMICTRACEPLOT_Y_SIZE 360

#include <deque>
#include <optional>
#include <vector>

#include <QtWidgets>

#include "antidromictracecapture.hpp"
#include "plotutilities.h"
#include "systemstate.h"

class AntidromicTracePlot : public QWidget
{
    Q_OBJECT
public:
    explicit AntidromicTracePlot(SystemState* state, QWidget* parent = nullptr);

    /**
     * @brief Set whether Y axis uses board ADC volts (true) or RHS HIGH µV (false).
     */
    void setBoardAdcRecording(bool boardAdc);

    /**
     * @brief Replace baseline and collision held traces for comparison overlay.
     */
    void setComparisonHeldTraces(const std::deque<antidromicTraceCapture::AntidromicTraceSnippet>& baselineTraces,
                                 const std::deque<antidromicTraceCapture::AntidromicTraceSnippet>& collisionTraces);

    /**
     * @brief Replace the single trace shown when no held comparison data is displayed.
     * @param isCollisionCondition When true, uses the collision trace color.
     */
    void setLatestTrace(const antidromicTraceCapture::AntidromicTraceSnippet& snippet, bool isCollisionCondition);

    /**
     * @brief Clear all displayed traces.
     */
    void clearTraces();

    /**
     * @brief Draw threshold and spike search window overlays on the plot.
     */
    void setDetectionOverlay(double threshold, double windowCenterMs, double windowWidthMs);

    /**
     * @brief Horizontal inset from widget edge to the plotted trace area (pixels).
     */
    int plotAreaLeftInset() const;

    /**
     * @brief Horizontal inset from the plotted trace area to the widget edge (pixels).
     */
    int plotAreaRightInset() const;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

public slots:
    void updateFromState();

signals:
    void plotGeometryUpdated();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateCoordinateTranslator();
    void paintHeldTraceStack(QPainter& painter, const std::deque<antidromicTraceCapture::AntidromicTraceSnippet>& traces,
                             const QColor& baseColor);
    void paintTrace(QPainter& painter, const std::vector<float>& samples, const QColor& color);
    void paintAxes(QPainter& painter, PlotDecorator& plotDecorator, double yHalfRange, const QString& yUnitSuffix);
    void paintDetectionOverlay(QPainter& painter, PlotDecorator& plotDecorator);

    SystemState* _state;
    bool _boardAdcRecording;
    bool _showDetectionOverlay;
    double _overlayThreshold;
    double _overlayWindowCenterMs;
    double _overlayWindowWidthMs;
    std::optional<antidromicTraceCapture::AntidromicTraceSnippet> _latestTrace;
    bool _latestTraceIsCollision;
    std::deque<antidromicTraceCapture::AntidromicTraceSnippet> _baselineHeldTraces;
    std::deque<antidromicTraceCapture::AntidromicTraceSnippet> _collisionHeldTraces;
    double _tStepMsec;
    QRect _plotFrame;
    CoordinateTranslator _ct;
    QImage _image;
};

#endif // ANTIDROMICTRACEPLOT_H
