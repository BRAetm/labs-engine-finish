#pragma once

#include <QDialog>
#include <QString>
#include <QVector>

class QListWidget;
class QPushButton;
class QLabel;

namespace Labs {

struct XboxConsoleEntry {
    QString id;
    QString name;
    QString state;
    QString type;
};

// Modal picker shown for Home (Remote Play) mode after the sidecar reports
// the available consoles. For xCloud no picker is shown — the sidecar
// auto-picks a region.
class XboxConsolePickerDialog : public QDialog {
    Q_OBJECT
public:
    explicit XboxConsolePickerDialog(QWidget* parent = nullptr);

    void setConsoles(const QVector<XboxConsoleEntry>& consoles);
    void setStatus(const QString& message, bool error = false);

    QString selectedConsoleId() const { return m_selectedId; }
    QString selectedConsoleName() const { return m_selectedName; }

private slots:
    void onSelectionChanged();
    void onAccept();

private:
    QListWidget* m_list   = nullptr;
    QPushButton* m_okBtn  = nullptr;
    QLabel*      m_status = nullptr;

    QVector<XboxConsoleEntry> m_entries;
    QString m_selectedId;
    QString m_selectedName;
};

} // namespace Labs
