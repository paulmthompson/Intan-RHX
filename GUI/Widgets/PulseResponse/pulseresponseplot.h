/**
 * @file pulseresponseplot.h
 * @brief Pulse response plot widget.
 * @ingroup PulseResponse
*/


#ifndef PULSERESPONSEPLOT_H
#define PULSERESPONSEPLOT_H

#define PULSERESPONSEPLOT_X_SIZE 580
#define PULSERESPONSEPLOT_Y_SIZE 360

#include <optional>
#include <vector>

#include <QtWidgets>

#include "plotutilities.h"
#include "PulseResponse/pulseelectroderesistance.hpp"
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
