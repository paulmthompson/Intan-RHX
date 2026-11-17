/**
 * @file pulseresistancedialog.h
 * @brief Pulse resistance dialog.
 * @ingroup PulseResponse
*/

#ifndef PULSERESISTANCEDIALOG_H
#define PULSERESISTANCEDIALOG_H

#include "PulseResponse/pulseelectroderesistance.hpp"
#include "systemstate.h"

#include <QColor>
#include <QDialog>

#include <optional>

class QLabel;
class QPushButton;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QStackedWidget;
class QTableWidget;
class PulseResponsePlot;
class WaveformFifo;

class PulseResistanceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PulseResistanceDialog(SystemState* state_, QWidget* parent = nullptr);

    void updateForRun();
    void updateForLoad();
    void updateForStop();
    void updateForChangeHeadstages();

    /**
     * @brief Process new display FIFO data for the watched channel or external ADC inputs.
     * @pre Acquisition running and dialog visible for meaningful updates.
     * @post UI updated when a stim pulse completes.
     */
    void updatePulseResistance(WaveformFifo* waveformFifo, int numSamples);

    /**
     * @brief Show dialog and refresh from state.
     */
    void activate();

private slots:
    void updateFromState();
    void changeCurrentChannel(const QString& nativeChannelName)
    {
        state->pulseResistanceChannel->setValue(nativeChannelName);
    }
    void toggleLock() { updateFromState(); }
    void setToSelected();
    void captureAcsfReference();
    void captureBrainReference();
    void clearReferences();
    void clearHistory();
    void setWarningThresholdPercent(double value);
    void chooseWarningColor();
    void setMode(int index);
    void setExternalDcAdc(int index);
    void setExternalTraceAdc(int index);
    void setExternalCurrentNanoAmps(double value);
    void setExternalTtlMarker(int index);
    void setExternalTtlActiveHigh(bool activeHigh);
    void setTraceTimeSpan(int index) { state->pulseResistanceTraceTimeMs->setIndex(index); }
    void setTraceYScaleUpper(int index);
    void setTraceYScaleDc(int index) { state->pulseResistanceTraceYScaleDc->setIndex(index); }

private:
    void updateTitle();
    void resetTrackerForChannelChange();
    void syncConfigFingerprint();
    QString buildConfigFingerprint() const;
    QStringList boardAdcChannelNames() const;
    void refreshExternalAdcComboBoxes();
    void updateHelpText();
    void updateModeUi();
    void applyResult(const pulseElectrodeResistance::PulseResistanceResult& result);
    void updatePulseResistanceRhs(WaveformFifo* waveformFifo, int numSamples);
    void updatePulseResistanceExternal(WaveformFifo* waveformFifo, int numSamples);
    QColor warningColor() const;
    void updateWarningColorButtonStyle();
    void refreshReferenceDisplays();
    void updateHistoryRowReferenceColumns(int row);
    static QString formatResistanceOhms(double ohms);
    static QString formatPercentDifference(double percentDiff);
    QString formatResistanceOhmsWithUncertainty(double ohms, double deltaVolts) const;
    static QString failureReasonText(pulseElectrodeResistance::PulseResistanceFailure failure);

    SystemState* state;

    QLabel* modeLabel;
    QComboBox* modeComboBox;
    QStackedWidget* modeStack;

    QWidget* rhsPage;
    QLabel* channelName;
    QCheckBox* lockScopeCheckbox;
    QPushButton* setToSelectedButton;

    QWidget* externalPage;
    QComboBox* externalDcAdcComboBox;
    QComboBox* externalTraceAdcComboBox;
    QDoubleSpinBox* externalCurrentSpinBox;
    QComboBox* externalTtlMarkerComboBox;
    QCheckBox* externalTtlActiveHighCheckbox;

    QPushButton* captureAcsfButton;
    QPushButton* captureBrainButton;
    QPushButton* clearReferencesButton;
    QPushButton* clearHistoryButton;
    QDoubleSpinBox* warningThresholdSpinBox;
    QPushButton* warningColorButton;

    QLabel* resistanceValueLabel;
    QLabel* acsfReferenceLabel;
    QLabel* acsfDiffLabel;
    QLabel* brainReferenceLabel;
    QLabel* brainDiffLabel;
    QLabel* deltaVoltageLabel;
    QLabel* currentLabel;
    QLabel* statusLabel;
    QLabel* helpLabel;

    QTableWidget* historyTable;

    QComboBox* traceTimeSpanComboBox;
    QComboBox* traceYScaleAcComboBox;
    QLabel* traceYScaleAcLabel;
    QLabel* traceYScaleDcLabel;
    QComboBox* traceYScaleDcComboBox;
    PulseResponsePlot* responsePlot;

    pulseElectrodeResistance::PulseResistanceTrackerState _trackerState;
    pulseElectrodeResistance::PulseResponseCaptureState _responseCaptureState;
    QString _watchedChannelNativeName;
    QString _configFingerprint;
    double _voltageQuantizationLsbVolts;
    std::optional<double> _latestValidResistanceOhms;
    std::optional<double> _acsfReferenceOhms;
    std::optional<double> _brainReferenceOhms;
    bool _captureAcsfOnNextPulse;
    bool _captureBrainOnNextPulse;

    static constexpr int kMaxHistoryRows = 50;
};

#endif
