//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#include "antidromicstimulationdialog.hpp"

#include "antidromicchannelselection.hpp"
#include "antidromictraceplot.h"
#include "channel.h"
#include "signalsources.h"
#include "systemstate.h"
#include "ui_antidromicstimulationdialog.h"
#include "waveformfifo.h"

#include <cmath>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QSpacerItem>
#include <QVBoxLayout>

namespace {

constexpr int kNativeNameRole = Qt::UserRole;

} // namespace

AntidromicStimulationDialog::AntidromicStimulationDialog(SystemState* state, QWidget* parent)
    : QDialog(parent)
    , _state(state)
    , _ui(new Ui::AntidromicStimulationDialog)
    , _tracePlot(nullptr)
    , _loading(false)
    , _hasLatestTrace(false)
{
    _ui->setupUi(this);
    setWindowModality(Qt::NonModal);
    _ui->plotRowLayout->setStretch(0, 1);
    _ui->plotRowLayout->setStretch(1, 0);
    _ui->windowCenterSliderRow->setStretch(1, 1);

    _tracePlot = new AntidromicTracePlot(_state, this);
    auto* plotLayout = new QVBoxLayout(_ui->plotPlaceholder);
    plotLayout->setContentsMargins(0, 0, 0, 0);
    plotLayout->addWidget(_tracePlot);

    _state->antidromicRecordingSource->setupComboBox(_ui->recordingSourceComboBox);
    _state->antidromicTraceTimeMs->setupComboBox(_ui->traceTimeComboBox);
    _state->antidromicTraceYScaleMicroVolts->setupComboBox(_ui->traceYScaleMicroVoltsComboBox);
    _state->antidromicTraceYScaleVolts->setupComboBox(_ui->traceYScaleVoltsComboBox);
    _state->antidromicHoldMaxTraces->setupComboBox(_ui->holdMaxTracesComboBox);

    connect(_state, SIGNAL(stateChanged()), this, SLOT(updateFromState()));
    connect(_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(_ui->stimChannelComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(stimChannelIndexChanged(int)));
    connect(_ui->recordingSourceComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(recordingSourceIndexChanged(int)));
    connect(_ui->recordingChannelComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(recordingChannelIndexChanged(int)));
    connect(_ui->traceTimeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setTraceTimeSpan(int)));
    connect(_ui->traceYScaleMicroVoltsComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setTraceYScaleMicroVolts(int)));
    connect(_ui->traceYScaleVoltsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setTraceYScaleVolts(int)));
    connect(_ui->holdTracesCheckBox, SIGNAL(toggled(bool)), this, SLOT(setHoldEnabled(bool)));
    connect(_ui->holdMaxTracesComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setHoldMaxTraces(int)));
    connect(_ui->clearTracesButton, SIGNAL(clicked()), this, SLOT(clearHeldTraces()));

    _state->antidromicDetectionWindowWidthMs->setupSpinBox(_ui->windowWidthSpinBox);
    _state->antidromicDetectionCrossing->setupComboBox(_ui->crossingComboBox);
    connect(_tracePlot, SIGNAL(plotGeometryUpdated()), this, SLOT(syncWindowCenterSliderMargins()));
    connect(_ui->thresholdSlider, SIGNAL(valueChanged(int)), this, SLOT(thresholdSliderChanged(int)));
    connect(_ui->windowCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(windowCenterSliderChanged(int)));
    connect(_ui->windowWidthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(windowWidthChanged(double)));
    connect(_ui->crossingComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(crossingIndexChanged(int)));

    refreshChannelComboBoxes();
    updateYScaleUi();
    syncWindowWidthSpinBoxLimits();
    clampDetectionWindowToTrace();
    syncWindowCenterSliderRange();
    syncDetectionWidgetsFromState();
    updateDetectionOverlay();
    updateStatisticsLabels();
    updateFromState();
    syncWindowCenterSliderMargins();
}

AntidromicStimulationDialog::~AntidromicStimulationDialog()
{
    delete _ui;
}

void AntidromicStimulationDialog::activate()
{
    syncConfigFingerprint();
    refreshChannelComboBoxes();
    updateFromState();
    show();
    raise();
    activateWindow();
}

void AntidromicStimulationDialog::updateForChangeHeadstages()
{
    resetCaptureForConfigChange();
    refreshChannelComboBoxes();
    updateFromState();
}

void AntidromicStimulationDialog::updateForRun()
{
    resetCaptureForConfigChange();
}

void AntidromicStimulationDialog::updateForStop()
{
    resetCaptureForConfigChange();
}

bool AntidromicStimulationDialog::recordingSourceIsBoardAdc() const
{
    return _state->antidromicRecordingSource->getValue() == QLatin1String("BoardAdc");
}

void AntidromicStimulationDialog::refreshChannelComboBoxes()
{
    populateStimChannelComboBox();
    populateRecordingChannelComboBox();
    syncConfigFingerprint();
}

void AntidromicStimulationDialog::populateStimChannelComboBox()
{
    const QString currentNative = _state->antidromicStimChannel->getValue();
    const std::vector<AntidromicChannelEntry> entries = rhsAmplifierChannelEntries(_state->signalSources);

    _ui->stimChannelComboBox->blockSignals(true);
    _ui->stimChannelComboBox->clear();
    for (const AntidromicChannelEntry& entry : entries) {
        _ui->stimChannelComboBox->addItem(entry.displayLabel, entry.nativeName);
    }

    int index = stimComboIndexForNativeName(currentNative);
    if (index < 0 && _ui->stimChannelComboBox->count() > 0) {
        index = 0;
        _state->antidromicStimChannel->setValue(
            _ui->stimChannelComboBox->itemData(0, kNativeNameRole).toString());
    }
    if (index >= 0) {
        _ui->stimChannelComboBox->setCurrentIndex(index);
    }
    _ui->stimChannelComboBox->blockSignals(false);
}

void AntidromicStimulationDialog::populateRecordingChannelComboBox()
{
    const QString currentNative = _state->antidromicRecordingChannel->getValue();

    _ui->recordingChannelComboBox->blockSignals(true);
    _ui->recordingChannelComboBox->clear();

    if (recordingSourceIsBoardAdc()) {
        const QStringList adcNames = boardAdcChannelNativeNames(_state->signalSources);
        for (const QString& nativeName : adcNames) {
            const QString displayLabel = _state->signalSources->getNativeAndCustomNames(nativeName);
            _ui->recordingChannelComboBox->addItem(displayLabel, nativeName);
        }
    } else {
        const std::vector<AntidromicChannelEntry> entries = rhsAmplifierChannelEntries(_state->signalSources);
        for (const AntidromicChannelEntry& entry : entries) {
            _ui->recordingChannelComboBox->addItem(entry.displayLabel, entry.nativeName);
        }
    }

    int index = recordingComboIndexForNativeName(currentNative);
    if (index < 0 && _ui->recordingChannelComboBox->count() > 0) {
        index = 0;
        _state->antidromicRecordingChannel->setValue(
            _ui->recordingChannelComboBox->itemData(0, kNativeNameRole).toString());
    }
    if (index >= 0) {
        _ui->recordingChannelComboBox->setCurrentIndex(index);
    }
    _ui->recordingChannelComboBox->blockSignals(false);
}

int AntidromicStimulationDialog::stimComboIndexForNativeName(const QString& nativeName) const
{
    for (int i = 0; i < _ui->stimChannelComboBox->count(); ++i) {
        if (_ui->stimChannelComboBox->itemData(i, kNativeNameRole).toString() == nativeName) {
            return i;
        }
    }
    return -1;
}

int AntidromicStimulationDialog::recordingComboIndexForNativeName(const QString& nativeName) const
{
    for (int i = 0; i < _ui->recordingChannelComboBox->count(); ++i) {
        if (_ui->recordingChannelComboBox->itemData(i, kNativeNameRole).toString() == nativeName) {
            return i;
        }
    }
    return -1;
}

void AntidromicStimulationDialog::updateYScaleUi()
{
    const bool boardAdc = recordingSourceIsBoardAdc();
    _ui->traceYScaleMicroVoltsLabel->setVisible(!boardAdc);
    _ui->traceYScaleMicroVoltsComboBox->setVisible(!boardAdc);
    _ui->traceYScaleVoltsLabel->setVisible(boardAdc);
    _ui->traceYScaleVoltsComboBox->setVisible(boardAdc);
    if (_tracePlot) {
        _tracePlot->setBoardAdcRecording(boardAdc);
    }
}

QString AntidromicStimulationDialog::buildConfigFingerprint() const
{
    return _state->antidromicStimChannel->getValue() + QLatin1Char('|')
           + _state->antidromicRecordingSource->getValue() + QLatin1Char('|')
           + _state->antidromicRecordingChannel->getValue();
}

void AntidromicStimulationDialog::syncConfigFingerprint()
{
    const QString fingerprint = buildConfigFingerprint();
    if (fingerprint != _configFingerprint) {
        _configFingerprint = fingerprint;
        resetCaptureForConfigChange();
    }
}

void AntidromicStimulationDialog::resetCaptureForConfigChange()
{
    _captureState = antidromicTraceCapture::AntidromicTraceCaptureState();
    _heldTraces.clear();
    _heldLatenciesMs.clear();
    _hasLatestTrace = false;
    if (_tracePlot) {
        _tracePlot->clearTraces();
    }
    updateStatisticsLabels();
}

void AntidromicStimulationDialog::refreshPlotFromBuffers()
{
    if (!_tracePlot) {
        return;
    }
    if (_state->antidromicTraceHoldEnabled->getValue()) {
        _tracePlot->setHeldTraces(_heldTraces);
    } else if (_hasLatestTrace) {
        _tracePlot->setLatestTrace(_latestTrace);
    } else {
        _tracePlot->clearTraces();
    }
}

void AntidromicStimulationDialog::applyCapturedSnippet(const antidromicTraceCapture::AntidromicTraceSnippet& snippet)
{
    _latestTrace = snippet;
    _hasLatestTrace = true;

    if (_state->antidromicTraceHoldEnabled->getValue()) {
        _heldTraces.push_back(snippet);
        const int maxTraces = static_cast<int>(_state->antidromicHoldMaxTraces->getNumericValue());
        while (static_cast<int>(_heldTraces.size()) > maxTraces) {
            _heldTraces.pop_front();
            if (!_heldLatenciesMs.empty()) {
                _heldLatenciesMs.pop_front();
            }
        }
        recomputeAllDetections();
    }
    refreshPlotFromBuffers();
}

void AntidromicStimulationDialog::updateAntidromicStimulation(WaveformFifo* waveformFifo, int numSamples)
{
    if (isHidden() || !waveformFifo || numSamples <= 0) {
        return;
    }

    if (_state->getControllerTypeEnum() != ControllerStimRecord) {
        return;
    }

    syncConfigFingerprint();

    const QString stimNative = _state->antidromicStimChannel->getValue();
    const QString recordNative = _state->antidromicRecordingChannel->getValue();
    if (stimNative.isEmpty() || stimNative == QStringLiteral("N/A") || recordNative.isEmpty()
        || recordNative == QStringLiteral("N/A")) {
        _ui->statusLabel->setText(tr("Select stimulation and recording channels."));
        return;
    }

    Channel* stimChannel = _state->signalSources->channelByName(stimNative);
    if (!stimChannel || !stimChannel->stimParameters) {
        _ui->statusLabel->setText(tr("Invalid stimulation channel."));
        return;
    }
    if (!stimChannel->stimParameters->enabled->getValue()) {
        _ui->statusLabel->setText(tr("Stimulation disabled on stimulation channel."));
        return;
    }

    const std::string stimWaveName = stimNative.toStdString() + "|STIM";
    const uint16_t* stimWaveform = waveformFifo->getDigitalWaveformPointer(stimWaveName);
    if (!stimWaveform) {
        _ui->statusLabel->setText(tr("STIM waveform unavailable for stimulation channel."));
        return;
    }

    const double sampleRateHz = _state->sampleRate->getNumericValue();
    const double totalTimeMs = _state->antidromicTraceTimeMs->getNumericValue();
    const WaveformFifo::Reader reader = WaveformFifo::ReaderDisplay;
    const int numWordsInMemory = waveformFifo->numWordsInMemory(reader);

    std::function<float(int)> getSample;
    if (recordingSourceIsBoardAdc()) {
        const float* analogWaveform = waveformFifo->getAnalogWaveformPointer(recordNative.toStdString());
        if (!analogWaveform) {
            _ui->statusLabel->setText(tr("Board ADC waveform unavailable — enable analog inputs."));
            return;
        }
        getSample = [waveformFifo, reader, analogWaveform](int timeIndex) {
            return waveformFifo->getAnalogData(reader, analogWaveform, timeIndex);
        };
    } else {
        const std::string highBandName = recordNative.toStdString() + "|HIGH";
        if (!waveformFifo->gpuWaveformPresent(highBandName)) {
            _ui->statusLabel->setText(tr("HIGH band not in FIFO — enable high-pass filter display."));
            return;
        }
        const GpuWaveformAddress highAddress = waveformFifo->getGpuWaveformAddress(highBandName);
        getSample = [waveformFifo, reader, highAddress](int timeIndex) {
            return waveformFifo->getGpuAmplifierData(reader, highAddress, timeIndex);
        };
    }

    auto getStimFlags = [waveformFifo, reader, stimWaveform](int timeIndex) {
        return waveformFifo->getDigitalData(reader, stimWaveform, timeIndex);
    };
    auto getTimeStamp = [waveformFifo, reader](int timeIndex) {
        return waveformFifo->getTimeStamp(reader, timeIndex);
    };

    const std::optional<antidromicTraceCapture::AntidromicTraceSnippet> snippet =
        antidromicTraceCapture::tryCaptureStimAlignedTrace(numSamples, sampleRateHz, totalTimeMs, numWordsInMemory,
                                                           _captureState, getStimFlags, getSample, getTimeStamp);

    if (snippet.has_value()) {
        applyCapturedSnippet(*snippet);
        _ui->statusLabel->setText(tr("Captured trace at stim onset (t = 0)."));
    } else if (!_ui->statusLabel->text().contains(tr("disabled"))) {
        _ui->statusLabel->setText(
        tr("Waiting for stim pulse on stimulation channel. Detection uses the first threshold crossing in the "
           "search window (enable Hold for hit-rate and latency statistics)."));
    }

    setWindowTitle(tr("Antidromic Stimulation") + " (" + recordNative + ")");
}

void AntidromicStimulationDialog::updateFromState()
{
    _loading = true;

    const int sourceIndex = _state->antidromicRecordingSource->getIndex();
    if (_ui->recordingSourceComboBox->currentIndex() != sourceIndex) {
        _ui->recordingSourceComboBox->blockSignals(true);
        _ui->recordingSourceComboBox->setCurrentIndex(sourceIndex);
        _ui->recordingSourceComboBox->blockSignals(false);
        populateRecordingChannelComboBox();
    }
    updateYScaleUi();

    const int stimIndex = stimComboIndexForNativeName(_state->antidromicStimChannel->getValue());
    if (stimIndex >= 0 && _ui->stimChannelComboBox->currentIndex() != stimIndex) {
        _ui->stimChannelComboBox->blockSignals(true);
        _ui->stimChannelComboBox->setCurrentIndex(stimIndex);
        _ui->stimChannelComboBox->blockSignals(false);
    }

    const int recordingIndex = recordingComboIndexForNativeName(_state->antidromicRecordingChannel->getValue());
    if (recordingIndex >= 0 && _ui->recordingChannelComboBox->currentIndex() != recordingIndex) {
        _ui->recordingChannelComboBox->blockSignals(true);
        _ui->recordingChannelComboBox->setCurrentIndex(recordingIndex);
        _ui->recordingChannelComboBox->blockSignals(false);
    }

    if (_ui->traceTimeComboBox->currentIndex() != _state->antidromicTraceTimeMs->getIndex()) {
        _ui->traceTimeComboBox->setCurrentIndex(_state->antidromicTraceTimeMs->getIndex());
    }
    if (_ui->traceYScaleMicroVoltsComboBox->currentIndex() != _state->antidromicTraceYScaleMicroVolts->getIndex()) {
        _ui->traceYScaleMicroVoltsComboBox->setCurrentIndex(_state->antidromicTraceYScaleMicroVolts->getIndex());
    }
    if (_ui->traceYScaleVoltsComboBox->currentIndex() != _state->antidromicTraceYScaleVolts->getIndex()) {
        _ui->traceYScaleVoltsComboBox->setCurrentIndex(_state->antidromicTraceYScaleVolts->getIndex());
    }
    if (_ui->holdTracesCheckBox->isChecked() != _state->antidromicTraceHoldEnabled->getValue()) {
        _ui->holdTracesCheckBox->setChecked(_state->antidromicTraceHoldEnabled->getValue());
    }
    if (_ui->holdMaxTracesComboBox->currentIndex() != _state->antidromicHoldMaxTraces->getIndex()) {
        _ui->holdMaxTracesComboBox->setCurrentIndex(_state->antidromicHoldMaxTraces->getIndex());
    }

    syncWindowWidthSpinBoxLimits();
    clampDetectionWindowToTrace();
    syncWindowCenterSliderRange();
    syncThresholdSliderRange();
    syncDetectionWidgetsFromState();
    syncWindowCenterSliderMargins();
    updateDetectionOverlay();
    refreshPlotFromBuffers();
    updateStatisticsLabels();

    _loading = false;
}

void AntidromicStimulationDialog::stimChannelIndexChanged(int index)
{
    if (_loading || index < 0) {
        return;
    }
    const QString nativeName = _ui->stimChannelComboBox->itemData(index, kNativeNameRole).toString();
    if (nativeName != _state->antidromicStimChannel->getValue()) {
        _state->antidromicStimChannel->setValue(nativeName);
    }
    syncConfigFingerprint();
}

void AntidromicStimulationDialog::recordingSourceIndexChanged(int index)
{
    if (_loading || index < 0) {
        return;
    }
    _state->antidromicRecordingSource->setIndex(index);
    populateRecordingChannelComboBox();
    updateYScaleUi();
    syncWindowCenterSliderRange();
    syncDetectionWidgetsFromState();
    updateDetectionOverlay();
    recomputeAllDetections();
    syncConfigFingerprint();
}

void AntidromicStimulationDialog::recordingChannelIndexChanged(int index)
{
    if (_loading || index < 0) {
        return;
    }
    const QString nativeName = _ui->recordingChannelComboBox->itemData(index, kNativeNameRole).toString();
    if (nativeName != _state->antidromicRecordingChannel->getValue()) {
        _state->antidromicRecordingChannel->setValue(nativeName);
    }
    syncConfigFingerprint();
}

void AntidromicStimulationDialog::setTraceTimeSpan(int index)
{
    if (_loading || index < 0) {
        return;
    }
    _state->antidromicTraceTimeMs->setIndex(index);
    _captureState = antidromicTraceCapture::AntidromicTraceCaptureState();
    clampDetectionWindowToTrace();
    syncWindowWidthSpinBoxLimits();
    syncWindowCenterSliderRange();
    syncDetectionWidgetsFromState();
    updateDetectionOverlay();
    recomputeAllDetections();
}

void AntidromicStimulationDialog::setTraceYScaleMicroVolts(int index)
{
    if (_loading || index < 0) {
        return;
    }
    _state->antidromicTraceYScaleMicroVolts->setIndex(index);
    syncThresholdSliderRange();
    updateDetectionOverlay();
}

void AntidromicStimulationDialog::setTraceYScaleVolts(int index)
{
    if (_loading || index < 0) {
        return;
    }
    _state->antidromicTraceYScaleVolts->setIndex(index);
    syncThresholdSliderRange();
    updateDetectionOverlay();
}

void AntidromicStimulationDialog::setHoldEnabled(bool enabled)
{
    if (_loading) {
        return;
    }
    _state->antidromicTraceHoldEnabled->setValue(enabled);
    if (!enabled) {
        _heldTraces.clear();
        _heldLatenciesMs.clear();
    }
    refreshPlotFromBuffers();
    updateStatisticsLabels();
}

void AntidromicStimulationDialog::setHoldMaxTraces(int index)
{
    if (_loading || index < 0) {
        return;
    }
    _state->antidromicHoldMaxTraces->setIndex(index);
    const int maxTraces = static_cast<int>(_state->antidromicHoldMaxTraces->getNumericValue());
    while (static_cast<int>(_heldTraces.size()) > maxTraces) {
        _heldTraces.pop_front();
        if (!_heldLatenciesMs.empty()) {
            _heldLatenciesMs.pop_front();
        }
    }
    recomputeAllDetections();
    refreshPlotFromBuffers();
}

void AntidromicStimulationDialog::clearHeldTraces()
{
    _heldTraces.clear();
    _heldLatenciesMs.clear();
    _hasLatestTrace = false;
    if (_tracePlot) {
        _tracePlot->clearTraces();
    }
    updateStatisticsLabels();
    _ui->statusLabel->setText(tr("Traces cleared. Enable Hold to accumulate statistics."));
}

antidromicSpikeDetection::AntidromicDetectionSettings AntidromicStimulationDialog::buildDetectionSettings() const
{
    antidromicSpikeDetection::AntidromicDetectionSettings settings;
    if (recordingSourceIsBoardAdc()) {
        settings.threshold = _state->antidromicDetectionThresholdVolts->getValue();
    } else {
        settings.threshold = static_cast<double>(_state->antidromicDetectionThresholdMicroVolts->getValue());
    }
    settings.windowCenterMs = _state->antidromicDetectionWindowCenterMs->getValue();
    settings.windowWidthMs = _state->antidromicDetectionWindowWidthMs->getValue();
    settings.polarity = antidromicSpikeDetection::crossingPolarityFromValueName(
        _state->antidromicDetectionCrossing->getValue().toStdString());
    settings.sampleRateHz = _state->sampleRate->getNumericValue();
    settings.totalTraceTimeMs = _state->antidromicTraceTimeMs->getNumericValue();
    return settings;
}

void AntidromicStimulationDialog::clampDetectionWindowToTrace()
{
    const double halfTraceMs = _state->antidromicTraceTimeMs->getNumericValue() * 0.5;
    double centerMs = _state->antidromicDetectionWindowCenterMs->getValue();
    double widthMs = _state->antidromicDetectionWindowWidthMs->getValue();

    widthMs = std::min(widthMs, _state->antidromicTraceTimeMs->getNumericValue());
    widthMs = std::max(widthMs, _state->antidromicDetectionWindowWidthMs->getMinValue());

    const double maxCenter = halfTraceMs - widthMs * 0.5;
    const double minCenter = -halfTraceMs + widthMs * 0.5;
    centerMs = std::max(minCenter, std::min(maxCenter, centerMs));

    _state->antidromicDetectionWindowCenterMs->setValue(centerMs);
    _state->antidromicDetectionWindowWidthMs->setValue(widthMs);
}

void AntidromicStimulationDialog::syncWindowCenterSliderRange()
{
    const double totalTraceMs = _state->antidromicTraceTimeMs->getNumericValue();
    const double halfSpanMs = totalTraceMs * 0.5;
    const int halfSpanTenths = static_cast<int>(std::lround(halfSpanMs * 10.0));
    _ui->windowCenterSlider->blockSignals(true);
    _ui->windowCenterSlider->setRange(-halfSpanTenths, halfSpanTenths);
    const int centerTenths =
        static_cast<int>(std::lround(_state->antidromicDetectionWindowCenterMs->getValue() * 10.0));
    _ui->windowCenterSlider->setValue(std::max(-halfSpanTenths, std::min(halfSpanTenths, centerTenths)));
    _ui->windowCenterSlider->blockSignals(false);
}

void AntidromicStimulationDialog::syncWindowWidthSpinBoxLimits()
{
    const double totalTraceMs = _state->antidromicTraceTimeMs->getNumericValue();
    const double minWidth = _state->antidromicDetectionWindowWidthMs->getMinValue();

    _ui->windowWidthSpinBox->blockSignals(true);
    _ui->windowWidthSpinBox->setMinimum(minWidth);
    _ui->windowWidthSpinBox->setMaximum(totalTraceMs);
    _ui->windowWidthSpinBox->blockSignals(false);
}

void AntidromicStimulationDialog::syncThresholdSliderRange()
{
    if (!_tracePlot) {
        return;
    }
    _ui->thresholdSlider->setMinimumHeight(_tracePlot->height());

    _ui->thresholdSlider->blockSignals(true);
    if (recordingSourceIsBoardAdc()) {
        const int yHalfMillivolts =
            static_cast<int>(std::lround(_state->antidromicTraceYScaleVolts->getNumericValue() * 1000.0));
        _ui->thresholdSlider->setRange(-yHalfMillivolts, yHalfMillivolts);
        const int scaledMillivolts =
            static_cast<int>(std::lround(_state->antidromicDetectionThresholdVolts->getValue() * 1000.0));
        _ui->thresholdSlider->setValue(std::max(-yHalfMillivolts, std::min(yHalfMillivolts, scaledMillivolts)));
    } else {
        const int yHalfMicroVolts =
            static_cast<int>(std::lround(_state->antidromicTraceYScaleMicroVolts->getNumericValue()));
        _ui->thresholdSlider->setRange(-yHalfMicroVolts, yHalfMicroVolts);
        const int threshold = _state->antidromicDetectionThresholdMicroVolts->getValue();
        _ui->thresholdSlider->setValue(std::max(-yHalfMicroVolts, std::min(yHalfMicroVolts, threshold)));
    }
    _ui->thresholdSlider->blockSignals(false);
}

void AntidromicStimulationDialog::syncWindowCenterSliderMargins()
{
    if (!_tracePlot) {
        return;
    }

    QLayoutItem* leftItem = _ui->windowCenterSliderRow->itemAt(0);
    QLayoutItem* rightItem = _ui->windowCenterSliderRow->itemAt(2);
    if (!leftItem || !rightItem) {
        return;
    }

    const int leftInset = _tracePlot->plotAreaLeftInset();
    const int rightInset = _tracePlot->plotAreaRightInset();
    if (QSpacerItem* leftSpacer = leftItem->spacerItem()) {
        leftSpacer->changeSize(leftInset, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);
    }
    if (QSpacerItem* rightSpacer = rightItem->spacerItem()) {
        rightSpacer->changeSize(rightInset, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);
    }
    _ui->windowCenterSliderRow->invalidate();
}

void AntidromicStimulationDialog::syncDetectionWidgetsFromState()
{
    syncThresholdSliderRange();

    if (std::abs(_ui->windowWidthSpinBox->value() - _state->antidromicDetectionWindowWidthMs->getValue()) > 1.0e-9) {
        _ui->windowWidthSpinBox->blockSignals(true);
        _ui->windowWidthSpinBox->setValue(_state->antidromicDetectionWindowWidthMs->getValue());
        _ui->windowWidthSpinBox->blockSignals(false);
    }

    if (_ui->crossingComboBox->currentIndex() != _state->antidromicDetectionCrossing->getIndex()) {
        _ui->crossingComboBox->blockSignals(true);
        _ui->crossingComboBox->setCurrentIndex(_state->antidromicDetectionCrossing->getIndex());
        _ui->crossingComboBox->blockSignals(false);
    }
}

void AntidromicStimulationDialog::updateDetectionOverlay()
{
    if (!_tracePlot) {
        return;
    }
    const antidromicSpikeDetection::AntidromicDetectionSettings settings = buildDetectionSettings();
    _tracePlot->setDetectionOverlay(settings.threshold, settings.windowCenterMs, settings.windowWidthMs);
}

void AntidromicStimulationDialog::recomputeAllDetections()
{
    if (!_state->antidromicTraceHoldEnabled->getValue()) {
        return;
    }

    const antidromicSpikeDetection::AntidromicDetectionSettings settings = buildDetectionSettings();
    _heldLatenciesMs.clear();
    for (const antidromicTraceCapture::AntidromicTraceSnippet& snippet : _heldTraces) {
        _heldLatenciesMs.push_back(antidromicSpikeDetection::detectLatencyMs(snippet.samples, settings));
    }
    updateStatisticsLabels();
}

void AntidromicStimulationDialog::updateStatisticsLabels()
{
    if (!_state->antidromicTraceHoldEnabled->getValue() || _heldTraces.empty()) {
        _ui->hitRateLabel->setText(tr("Hit rate: N/A"));
        _ui->latencyStatsLabel->setText(tr("Latency: N/A"));
        return;
    }

    std::vector<std::optional<double>> latencies;
    latencies.reserve(_heldLatenciesMs.size());
    for (const std::optional<double>& latency : _heldLatenciesMs) {
        latencies.push_back(latency);
    }

    const antidromicSpikeDetection::AntidromicHeldTraceStats stats =
        antidromicSpikeDetection::computeHeldTraceStats(latencies);

    _ui->hitRateLabel->setText(tr("Hit rate: %1% (%2 / %3)")
                                   .arg(stats.hitRatePercent, 0, 'f', 1)
                                   .arg(stats.numHits)
                                   .arg(stats.numTraces));

    if (!stats.meanMs.has_value()) {
        _ui->latencyStatsLabel->setText(tr("Latency: N/A"));
        return;
    }

    if (stats.stdMs.has_value()) {
        _ui->latencyStatsLabel->setText(tr("Latency: %1 ± %2 ms")
                                            .arg(*stats.meanMs, 0, 'f', 3)
                                            .arg(*stats.stdMs, 0, 'f', 3));
    } else {
        _ui->latencyStatsLabel->setText(tr("Latency: %1 ms").arg(*stats.meanMs, 0, 'f', 3));
    }
}

void AntidromicStimulationDialog::thresholdSliderChanged(int value)
{
    if (_loading) {
        return;
    }
    if (recordingSourceIsBoardAdc()) {
        _state->antidromicDetectionThresholdVolts->setValue(static_cast<double>(value) / 1000.0);
    } else {
        _state->antidromicDetectionThresholdMicroVolts->setValue(value);
    }
    updateDetectionOverlay();
    recomputeAllDetections();
}

void AntidromicStimulationDialog::windowCenterSliderChanged(int value)
{
    if (_loading) {
        return;
    }
    _state->antidromicDetectionWindowCenterMs->setValue(static_cast<double>(value) / 10.0);
    clampDetectionWindowToTrace();
    syncWindowCenterSliderRange();
    updateDetectionOverlay();
    recomputeAllDetections();
}

void AntidromicStimulationDialog::windowWidthChanged(double value)
{
    if (_loading) {
        return;
    }
    _state->antidromicDetectionWindowWidthMs->setValue(value);
    clampDetectionWindowToTrace();
    syncWindowCenterSliderRange();
    syncDetectionWidgetsFromState();
    updateDetectionOverlay();
    recomputeAllDetections();
}

void AntidromicStimulationDialog::crossingIndexChanged(int index)
{
    if (_loading || index < 0) {
        return;
    }
    _state->antidromicDetectionCrossing->setIndex(index);
    recomputeAllDetections();
}
