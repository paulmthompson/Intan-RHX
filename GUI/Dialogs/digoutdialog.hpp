//------------------------------------------------------------------------------
// Modeless RHS digital-output configuration dialog.
//------------------------------------------------------------------------------

#ifndef DIGOUTDIALOG_HPP
#define DIGOUTDIALOG_HPP

#include "rhsdigoutconfiguration.hpp"

#include <QDialog>

class Channel;
class ControllerInterface;
class DigFigure;
class SystemState;

namespace Ui {
class DigOutDialog;
}

/**
 * @brief Modeless draft editor for one RHS board digital output.
 * @ingroup RhsDigitalOut
 *
 * Widget edits remain local until Apply or OK. Cancel discards only edits made
 * after the most recent successful Apply.
 */
class DigOutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DigOutDialog(SystemState* state,
                          ControllerInterface* controllerInterface,
                          Channel* channel,
                          QWidget* parent = nullptr);
    ~DigOutDialog() override;

    /**
     * @brief Show and raise the modeless editor without replacing its draft.
     * @post The dialog is visible and active.
     */
    void activate();

    /**
     * @brief Whether widget values differ from the last successful Apply.
     */
    bool hasUnappliedChanges() const;

    /**
     * @brief Apply the current draft and keep the dialog open.
     * @return true after a successful controller commit.
     * @post On success, Cancel's baseline becomes the applied configuration.
     */
    bool applyDraft();

    /**
     * @brief Discard edits made since the most recent successful Apply.
     */
    void discardDraft();

    /**
     * @brief Channel currently owned by this dialog.
     */
    Channel* channel() const { return _channel; }

public slots:
    void accept() override;
    void reject() override;

signals:
    void configurationApplied(Channel* channel);

private slots:
    void _readWidgetsIntoDraft();
    void _updateWidgetsForMode();
    void _updateAnalogInTriggerThresholdVisibility();
    void _commitAnalogInTriggerThresholdFromSpinBox();

private:
    /**
     * @brief Populate all widgets from the local draft.
     */
    void _loadDraftIntoWidgets();

    /**
     * @brief Refresh achieved values and the waveform preview.
     */
    void _updatePreview();

    /**
     * @brief Whether the analog tab owns this dialog's TTL pin.
     */
    bool _thresholdOwned() const;

    Ui::DigOutDialog* _ui;
    SystemState* _state;
    ControllerInterface* _controllerInterface;
    Channel* _channel;
    DigFigure* _digFigure;
    RhsDigOutConfiguration _draft;
    RhsDigOutConfiguration _appliedBaseline;
    bool _loading;
};

#endif // DIGOUTDIALOG_HPP
