//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#ifndef ANTIDROMICSTIMULATIONDIALOG_HPP
#define ANTIDROMICSTIMULATIONDIALOG_HPP

#include <deque>
#include <optional>
#include <vector>

#include <QDialog>

#include "antidromicspikedetection.hpp"
#include "antidromictracecapture.hpp"

class AntidromicTracePlot;
class SystemState;
class WaveformFifo;

namespace Ui {
class AntidromicStimulationDialog;
}

/**
 * @brief Session buffers for one antidromic comparison condition (baseline or collision).
 */
struct AntidromicConditionBuffers
{
    std::deque<antidromicTraceCapture::AntidromicTraceSnippet> traces;
    std::deque<std::optional<double>> latenciesMs;
};

/**
 * @brief Antidromic stimulation channel selection and stim-aligned recording trace display.
 */
class AntidromicStimulationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AntidromicStimulationDialog(SystemState* state, QWidget* parent = nullptr);
    ~AntidromicStimulationDialog() override;

    /**
     * @brief Show the dialog and sync widgets from state.
     * @post Dialog is visible and reflects current SystemState channel selections.
     */
    void activate();

    /**
     * @brief Refresh channel lists after headstage topology changes.
     */
    void updateForChangeHeadstages();

    /**
     * @brief Process new display FIFO data for stim-aligned traces.
     * @pre Acquisition running for meaningful updates.
     */
    void updateAntidromicStimulation(WaveformFifo* waveformFifo, int numSamples);

    void updateForRun();
    void updateForStop();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void updateFromState();
    void stimChannelIndexChanged(int index);
    void recordingSourceIndexChanged(int index);
    void recordingChannelIndexChanged(int index);
    void setTraceTimeSpan(int index);
    void setTraceYScaleMicroVolts(int index);
    void setTraceYScaleVolts(int index);
    void setHoldEnabled(bool enabled);
    void setHoldMaxTraces(int index);
    void comparisonConditionIndexChanged(int index);
    void clearActiveConditionTraces();
    void clearAllConditionTraces();
    void thresholdSliderChanged(int value);
    void windowCenterSliderChanged(int value);
    void windowWidthChanged(double value);
    void crossingIndexChanged(int index);
    void syncWindowCenterSliderMargins();

private:
    enum class AntidromicComparisonCondition { Baseline, Collision };

    void refreshChannelComboBoxes();
    void populateStimChannelComboBox();
    void populateRecordingChannelComboBox();
    int stimComboIndexForNativeName(const QString& nativeName) const;
    int recordingComboIndexForNativeName(const QString& nativeName) const;
    bool recordingSourceIsBoardAdc() const;
    bool comparisonConditionIsCollision() const;
    AntidromicComparisonCondition activeComparisonCondition() const;
    AntidromicConditionBuffers& activeConditionBuffers();
    const AntidromicConditionBuffers& activeConditionBuffers() const;
    void trimConditionBuffers(AntidromicConditionBuffers& buffers);
    void clearSessionBuffers();
    void clearConditionBuffers(AntidromicComparisonCondition which);
    void updateYScaleUi();
    void syncConfigFingerprint();
    QString buildConfigFingerprint() const;
    void resetCaptureForConfigChange();
    void refreshPlotFromBuffers();
    void applyCapturedSnippet(const antidromicTraceCapture::AntidromicTraceSnippet& snippet);
    antidromicSpikeDetection::AntidromicDetectionSettings buildDetectionSettings() const;
    void clampDetectionWindowToTrace();
    void syncWindowCenterSliderRange();
    void syncWindowWidthSpinBoxLimits();
    void syncThresholdSliderRange();
    void syncDetectionWidgetsFromState();
    void updateDetectionOverlay();
    void recomputeConditionDetections(AntidromicConditionBuffers& buffers);
    void recomputeAllDetections();
    void updateStatisticsLabels();
    void updateStatusHint();
    QString formatConditionHitRateLabel(const QString& prefix,
                                        const antidromicSpikeDetection::AntidromicHeldTraceStats& stats) const;
    QString formatConditionLatencyLabel(const QString& prefix,
                                        const antidromicSpikeDetection::AntidromicHeldTraceStats& stats) const;

    SystemState* _state;
    Ui::AntidromicStimulationDialog* _ui;
    AntidromicTracePlot* _tracePlot;
    bool _loading;
    antidromicTraceCapture::AntidromicTraceCaptureState _captureState;
    AntidromicConditionBuffers _baselineBuffers;
    AntidromicConditionBuffers _collisionBuffers;
    antidromicTraceCapture::AntidromicTraceSnippet _latestTrace;
    bool _hasLatestTrace;
    QString _configFingerprint;
};

#endif // ANTIDROMICSTIMULATIONDIALOG_HPP
