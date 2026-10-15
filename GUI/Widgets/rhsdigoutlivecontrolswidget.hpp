//------------------------------------------------------------------------------
// Selected-channel RHS digital-output live controls.
//------------------------------------------------------------------------------

#ifndef RHSDIGOUTLIVECONTROLSWIDGET_HPP
#define RHSDIGOUTLIVECONTROLSWIDGET_HPP

#include "rhsdigoutconfiguration.hpp"

#include <QWidget>

namespace Ui {
class RhsDigOutLiveControlsWidget;
}

/**
 * @brief Presents the one live action applicable to the selected RHS TTL pin.
 * @ingroup RhsDigitalOut
 * @see ControllerInterface::setRhsDigOutManualLevel
 * @see ControllerInterface::startRhsDigOutContinuous
 * @see ControllerInterface::fireRhsDigOut
 */
class RhsDigOutLiveControlsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RhsDigOutLiveControlsWidget(QWidget* parent = nullptr);
    ~RhsDigOutLiveControlsWidget() override;

    /**
     * @brief Refresh mode, label, and eligibility from committed/live state.
     * @post The widget is hidden when the selected configuration has no live action.
     */
    void updateState(const RhsDigOutConfiguration& configuration,
                     const RhsDigOutRuntimeState& runtime,
                     bool acquisitionRunning,
                     bool commandAvailable,
                     bool thresholdOwned,
                     bool uploadInProgress);

signals:
    void manualLevelRequested(bool high);
    void continuousRequested(bool start);
    void fireRequested();

private slots:
    void _actionClicked();

private:
    Ui::RhsDigOutLiveControlsWidget* _ui;
    RhsDigOutConfiguration _configuration;
    RhsDigOutRuntimeState _runtime;
};

#endif // RHSDIGOUTLIVECONTROLSWIDGET_HPP
