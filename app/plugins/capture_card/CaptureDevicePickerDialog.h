#pragma once

#include "MfCaptureSource.h"

#include <QDialog>
#include <vector>

class QListWidget;
class QPushButton;
class QLabel;

namespace Labs {

class CaptureDevicePickerDialog : public QDialog {
    Q_OBJECT
public:
    explicit CaptureDevicePickerDialog(QWidget* parent = nullptr);
    ~CaptureDevicePickerDialog() override = default;

    bool hasSelection() const { return m_selectedRow >= 0; }
    CaptureDeviceInfo selectedDevice() const;

private slots:
    void onRescan();
    void onSelectionChanged();
    void onAccept();

private:
    void rescanDevices();
    void rebuildList();

    std::vector<CaptureDeviceInfo> m_devices;
    int                            m_selectedRow = -1;

    QListWidget* m_list      = nullptr;
    QPushButton* m_btnStart  = nullptr;
    QPushButton* m_btnRescan = nullptr;
    QPushButton* m_btnCancel = nullptr;
    QLabel*      m_emptyHint = nullptr;
};

} // namespace Labs
