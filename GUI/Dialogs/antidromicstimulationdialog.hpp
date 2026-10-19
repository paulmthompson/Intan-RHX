//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#ifndef ANTIDROMICSTIMULATIONDIALOG_HPP
#define ANTIDROMICSTIMULATIONDIALOG_HPP

#include <deque>

#include <QDialog>

#include "antidromictracecapture.hpp"

class AntidromicTracePlot;
class SystemState;
class WaveformFifo;

namespace Ui {
class AntidromicStimulationDialog;
}

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
    void clearHeldTraces();

private:
    void refreshChannelComboBoxes();
    void populateStimChannelComboBox();
    void populateRecordingChannelComboBox();
    int stimComboIndexForNativeName(const QString& nativeName) const;
    int recordingComboIndexForNativeName(const QString& nativeName) const;
    bool recordingSourceIsBoardAdc() const;
    void updateYScaleUi();
    void syncConfigFingerprint();
    QString buildConfigFingerprint() const;
    void resetCaptureForConfigChange();
    void refreshPlotFromBuffers();
    void applyCapturedSnippet(const antidromicTraceCapture::AntidromicTraceSnippet& snippet);

    SystemState* _state;
    Ui::AntidromicStimulationDialog* _ui;
    AntidromicTracePlot* _tracePlot;
    bool _loading;
    antidromicTraceCapture::AntidromicTraceCaptureState _captureState;
    std::deque<antidromicTraceCapture::AntidromicTraceSnippet> _heldTraces;
    antidromicTraceCapture::AntidromicTraceSnippet _latestTrace;
    bool _hasLatestTrace;
    QString _configFingerprint;
};

#endif // ANTIDROMICSTIMULATIONDIALOG_HPP
