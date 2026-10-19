//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//
//------------------------------------------------------------------------------

#ifndef ANTIDROMICSTIMULATIONDIALOG_HPP
#define ANTIDROMICSTIMULATIONDIALOG_HPP

#include <QDialog>

class SystemState;

namespace Ui {
class AntidromicStimulationDialog;
}

/**
 * @brief Channel selection shell for antidromic stimulation on the RHS controller.
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

private slots:
    void updateFromState();
    void stimChannelIndexChanged(int index);
    void recordingSourceIndexChanged(int index);
    void recordingChannelIndexChanged(int index);

private:
    void refreshChannelComboBoxes();
    void populateStimChannelComboBox();
    void populateRecordingChannelComboBox();
    int stimComboIndexForNativeName(const QString& nativeName) const;
    int recordingComboIndexForNativeName(const QString& nativeName) const;
    bool recordingSourceIsBoardAdc() const;

    SystemState* _state;
    Ui::AntidromicStimulationDialog* _ui;
    bool _loading;
};

#endif // ANTIDROMICSTIMULATIONDIALOG_HPP
