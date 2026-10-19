//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#include "antidromicstimulationdialog.hpp"

#include "antidromicchannelselection.hpp"
#include "signalsources.h"
#include "systemstate.h"
#include "ui_antidromicstimulationdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>

namespace {

constexpr int kNativeNameRole = Qt::UserRole;

} // namespace

AntidromicStimulationDialog::AntidromicStimulationDialog(SystemState* state, QWidget* parent)
    : QDialog(parent)
    , _state(state)
    , _ui(new Ui::AntidromicStimulationDialog)
    , _loading(false)
{
    _ui->setupUi(this);
    setWindowModality(Qt::NonModal);

    _state->antidromicRecordingSource->setupComboBox(_ui->recordingSourceComboBox);

    connect(_state, SIGNAL(stateChanged()), this, SLOT(updateFromState()));
    connect(_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(_ui->stimChannelComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(stimChannelIndexChanged(int)));
    connect(_ui->recordingSourceComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(recordingSourceIndexChanged(int)));
    connect(_ui->recordingChannelComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(recordingChannelIndexChanged(int)));

    refreshChannelComboBoxes();
    updateFromState();
}

AntidromicStimulationDialog::~AntidromicStimulationDialog()
{
    delete _ui;
}

void AntidromicStimulationDialog::activate()
{
    refreshChannelComboBoxes();
    updateFromState();
    show();
    raise();
    activateWindow();
}

void AntidromicStimulationDialog::updateForChangeHeadstages()
{
    refreshChannelComboBoxes();
    updateFromState();
}

bool AntidromicStimulationDialog::recordingSourceIsBoardAdc() const
{
    return _state->antidromicRecordingSource->getValue() == QLatin1String("BoardAdc");
}

void AntidromicStimulationDialog::refreshChannelComboBoxes()
{
    populateStimChannelComboBox();
    populateRecordingChannelComboBox();
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
}

void AntidromicStimulationDialog::recordingSourceIndexChanged(int index)
{
    if (_loading || index < 0) {
        return;
    }
    _state->antidromicRecordingSource->setIndex(index);
    populateRecordingChannelComboBox();
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
}
