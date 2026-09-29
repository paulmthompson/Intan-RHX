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

/**
 * @file controllerinterface.h
 * @ingroup RhxStimSequencer
 * @brief High-level controller facade; declares stim sequence upload entry points.
 *
 * @details Maintenance: see docs/stim-sequencer-maintenance.md
 */

#ifndef CONTROLLERINTERFACE_H
#define CONTROLLERINTERFACE_H

#include <QObject>
#include <QString>

#include <vector>
#include <string>

#include "rhxcontroller.h"
#include "datafilereader.h"
#include "rhxglobals.h"
#include "datastreamfifo.h"
#include "usbdatathread.h"
#include "waveformprocessorthread.h"
#include "rhxregisters.h"
#include "rhxdatareader.h"
#include "multicolumndisplay.h"
#include "savetodiskthread.h"
#include "audiothread.h"
#include "tcpdataoutputthread.h"
#include "systemstate.h"
#include "signalsources.h"
#include "xpucontroller.h"
#include "isidialog.h"
#include "psthdialog.h"
#include "spectrogramdialog.h"
#include "spikesortingdialog.h"

class ControlPanel;

class ControllerInterface : public QObject
{
    Q_OBJECT
public:
    ControllerInterface(SystemState* state_, AbstractRHXController* rhxController_, const QString& boardSerialNumber, bool useOpenCL,
                        DataFileReader* dataFileReader_ = nullptr, QObject* parent = nullptr, bool is7310_ = false);
    ~ControllerInterface();

    void rescanPorts(bool updateDisplay = false);

    void updateChipCommandLists(bool updateStimParams = false);

    /**
     * @brief Enter amplifier maintenance mode during continuous acquisition (custom bitfile).
     * @pre ControllerStimRecord, continuous run active; see docs/amp-maintenance-mode.md.
     * @post Amp FIFO zeros; aux_execute off; stim sequencers held.
     */
    void beginAmpMaintenance();

    /**
     * @brief Upload RHS register-config aux lists and execute them inside maintenance.
     * @pre beginAmpMaintenance() already called; do not call during PipeIn with aux_execute on.
     */
    bool uploadRhsRegisterConfigDuringMaintenance(bool updateStimParams = false);

    /**
     * @brief Leave maintenance mode (brief DSP settle, then clear WireIn bits).
     */
    void endAmpMaintenance();

    /**
     * @brief Apply @c desired* bandwidth/DSP settings while acquisition is running (custom bitfile).
     * @pre ControllerStimRecord, continuous run; uses amp maintenance + RHS register aux upload.
     */
    void uploadBandwidthDuringMaintenance();

    /**
     * @brief Apply amp-channel stim sequencer program and magnitudes while acquisition is running (custom bitfile).
     * @pre ControllerStimRecord, continuous run, @p channel is AmplifierSignal; see docs/amp-maintenance-mode.md.
     * @post FPGA program bank 0 and RHS2116 DACs match channel stim parameters (active program banks).
     */
    void uploadStimParametersDuringMaintenance(Channel* channel);

    /**
     * @brief Clear amp-maintenance WireIns and upload state (e.g. on Stop or interrupted live upload).
     * @post aux_execute and amp_maintenance deasserted; DSP settle off; UploadInProgress false.
     */
    void abortAmpMaintenanceIfAny();

    void getCableDelay(std::vector<int> &delays) const { rhxController->getCableDelay(delays); }
    void setCableDelay(BoardPort port, int delay) { rhxController->setCableDelay(port, delay); }
    void enableExternalDigOut(BoardPort port, bool enable) { rhxController->enableExternalDigOut(port, enable); }
    void setExternalDigOutChannel(BoardPort port, int channel) { rhxController->setExternalDigOutChannel(port, channel); }
    void setManualCableDelays();

    void toggleAudioThread(bool enabled);
    void runTCPDataOutputThread();

    void runController();
    void runControllerSilently(double nSeconds, QProgressDialog* progress = nullptr);
    float measureRmsLevel(std::string waveName, double timeSec) const;
    void setAllSpikeDetectionThresholds();
    void sweepDisplay(double speed);
    bool rewindPossible() const { return waveformFifo->numWordsInMemory(WaveformFifo::ReaderDisplay) > 0; }
    bool fastForwardPossible() const { return currentSweepPosition < 0; }

    void setDisplay(MultiColumnDisplay* display_) { display = display_; }
    void setControlPanel(ControlPanel* controlPanel_) { controlPanel = controlPanel_; }
    void setISIDialog(ISIDialog* isiDialog_) { isiDialog = isiDialog_; }
    void setPSTHDialog(PSTHDialog* psthDialog_) { psthDialog = psthDialog_; }
    void setSpectrogramDialog(SpectrogramDialog* spectrogramDialog_) { spectrogramDialog = spectrogramDialog_; }
    void setSpikeSortingDialog(SpikeSortingDialog* spikeSortingDialog_) { spikeSortingDialog = spikeSortingDialog_; }

    QString getCurrentAudioChannel() const { return currentAudioChannel; }

    /**
     * @brief Upload amp-channel stim sequencer program from channel StimParameters.
     * @ingroup RhxStimSequencer
     *
     * Converts timing fields from microseconds to ticks, then calls programStimReg and magnitude aux commands.
     *
     * @pre rhxController is live hardware (not synthetic or playback).
     * @post Sequencer registers and stim magnitudes match ampChannel->stimParameters.
     *
     * @see setAnalogOutSequenceParameters
     * @see setDigitalOutSequenceParameters
     * @see AbstractRHXController::programStimReg
     */
    void setStimSequenceParameters(Channel* ampChannel, StimParameters* parameters, int stimProgramBank = 0);

    /**
     * @brief Upload board-DAC stim sequencer program from channel StimParameters.
     * @ingroup RhxStimSequencer
     *
     * @pre rhxController is live hardware (not synthetic or playback).
     * @post DAC sequencer registers match anOutChannel->stimParameters.
     *
     * @see setStimSequenceParameters
     * @see AbstractRHXController::programStimReg
     */
    void setAnalogOutSequenceParameters(Channel* anOutChannel);

    /**
     * @brief Upload digital-out stim sequencer program from channel StimParameters.
     * @ingroup RhxStimSequencer
     *
     * @pre rhxController is live hardware (not synthetic or playback).
     * @post Digout sequencer registers match digOutChannel->stimParameters.
     *
     * @see setStimSequenceParameters
     * @see AbstractRHXController::programStimReg
     */
    void setDigitalOutSequenceParameters(Channel* digOutChannel);

    void setManualStimTrigger(int trigger, bool triggerOn);
    void setManualStimTrigger(QString keyName, bool triggerOn);

    void setChargeRecoveryParameters(bool mode, RHXRegisters::ChargeRecoveryCurrentLimit currentLimit,
                                     double targetVoltage);

    void setAmpSettleMode(bool useFastSettle) { rhxController->setAmpSettleMode(useFastSettle); }
    void setGlobalSettlePolicy(bool settleWholeHeadstageA, bool settleWholeHeadstageB, bool settleWholeHeadstageC,
                               bool settleWholeHeadstageD, bool settleAllHeadstages)
        { rhxController->setGlobalSettlePolicy( settleWholeHeadstageA, settleWholeHeadstageB, settleWholeHeadstageC,
                                                settleWholeHeadstageD, settleAllHeadstages); }

    void setDacGain(int dacGainIndex);
    void setAudioNoiseSuppress(int noiseSuppressIndex);
    void setDacHighpassFilterEnabled(bool enabled);
    void setDacHighpassFilterFrequency(double frequency);
    void setDacChannel(int dac, const QString& channelName);
    void setDacRefChannel(const QString& channelName);
    void setDacThreshold(int dac, int threshold);
    void setDacEnabled(int dac, bool enabled);
    void setTtlOutMode(bool mode1, bool mode2, bool mode3, bool mode4, bool mode5, bool mode6, bool mode7, bool mode8);

    void enableFastSettle(bool enabled);
    void enableExternalFastSettle(bool enabled);
    void setExternalFastSettleChannel(int channel);

    SaveToDiskThread* saveThread() const { return saveToDiskThread; }

    QString playbackFileName() const;
    QString currentTimePlaybackFile() const;
    QString startTimePlaybackFile() const;
    QString endTimePlaybackFile() const;

    void resetWaveformFifo();

    bool measureImpedances();
    bool saveImpedances();

    double swBufferPercentFull() const;
    double latestWaveformProcessorCpuLoad() const { return waveformProcessorCpuLoad; }

    void uploadAmpSettleSettings();
    void uploadChargeRecoverySettings();
    void uploadBandwidthSettings();
    void uploadStimParameters(Channel* channel);
    void uploadStimParameters();

signals:
    void setTimeLabel(QString text);
    void setTopStatusLabel(QString text);
    void haveStopped();
    void setHardwareFifoStatus(double percentFull);
    void cpuLoadPercent(double percent);
    void TCPErrorMessage(QString errorMessage);

public slots:
    void updateFromState();
    void updateCurrentAudioChannel(QString name);
    void manualStimTriggerOn(QString keyName);
    void manualStimTriggerOff(QString keyName);
    void manualStimTriggerPulse(QString keyName);

private slots:
    void updateHardwareFifo(double percentFull) { emit setHardwareFifoStatus(percentFull); }
    void updateWaveformProcessorCpuLoad(double percentLoad) { waveformProcessorCpuLoad = percentLoad; }

private:
    void openController(const QString& boardSerialNumber);
    void initializeController();
    int scanPorts(std::vector<ChipType> &chipType, std::vector<int> &portIndex, std::vector<int> &commandStream,
                  std::vector<int> &numChannelsOnPort);
    void addAmplifierChannels(const std::vector<ChipType> &chipType, const std::vector<int> &portIndex,
                              const std::vector<int> &commandStream, const std::vector<int> &numChannelsOnPort);
    void enablePlaybackChannels();
    void addPlaybackHeadstageChannels();

    void sendTCPError(QString errorMessage);
    void pipeReadErrorMessage(int errorID);

    /**
     * @brief Sleep in short slices, processing Qt events so Stop can run.
     * @return false if @c state->running became false (caller should abort maintenance).
     */
    bool sleepMsInterruptible(int totalMs);

    void logTeardownStage(const char* message);

    /**
     * @brief Program FPGA stim sequencer registers (WireIn / BRAM) for one amp program bank.
     * @pre Live hardware; @p parameters non-null.
     * @post Sequencer program words for @p stimProgramBank match @p parameters (no MOSI magnitude upload).
     */
    void programAmpStimSequencerRegs(Channel* ampChannel, StimParameters* parameters, int stimProgramBank);

    /**
     * @brief Upload stim magnitudes on the headstage using a finite aux run (controller stopped).
     * @pre Bank 0 program; not used during continuous acquisition.
     * @post RHS2116 magnitude registers updated; aux enabled on all streams.
     */
    void uploadAmpStimMagnitudesStopped(Channel* ampChannel, StimParameters* parameters);

    /**
     * @brief Execute createCommandListSetStimMagnitudes aux list inside an active maintenance window.
     * @pre beginAmpMaintenance() called; aux_execute off.
     * @return false if wait aborted (e.g. Stop pressed).
     * @post aux_execute off; aux routing restored to all streams.
     */
    bool uploadAmpStimMagnitudesDuringMaintenance(int stream, int chipChannel, StimParameters* parameters);

    SystemState* state;
    AbstractRHXController* rhxController;
    DataFileReader* dataFileReader;
    TCPDataOutputThread* tcpDataOutputThread;

    XPUController* xpuController;

    DataStreamFifo* usbStreamFifo;
    USBDataThread* usbDataThread;
    WaveformFifo* waveformFifo;
    WaveformProcessorThread* waveformProcessorThread;

    MultiColumnDisplay* display;
    ControlPanel* controlPanel;
    ISIDialog* isiDialog;
    PSTHDialog* psthDialog;
    SpectrogramDialog* spectrogramDialog;
    SpikeSortingDialog* spikeSortingDialog;

    AudioThread* audioThread;
    SaveToDiskThread* saveToDiskThread;

    int currentSweepPosition;

    bool audioEnabled;
    bool tcpDataOutputEnabled;

    QString currentAudioChannel;

    double hardwareFifoPercentFull;
    double waveformProcessorCpuLoad;
    std::vector<double> cpuLoadHistory;

    bool is7310;

    bool _runControllerActive;
    bool _ampMaintenanceEntered;

    void outOfMemoryError(double memRequiredGB);
};

#endif // CONTROLLERINTERFACE_H
