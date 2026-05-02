#include "XboxConsolePickerDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace Labs {

XboxConsolePickerDialog::XboxConsolePickerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Pick Xbox Console"));
    setModal(true);
    setMinimumSize(420, 360);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("<b>STEP 2 — Pick Console</b>"), this);
    root->addWidget(title);

    auto* hint = new QLabel(QStringLiteral("Pick the Xbox to stream from. Make sure it's powered on or in instant-on standby."), this);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: #888; font-size: 11px;"));
    root->addWidget(hint);

    m_list = new QListWidget(this);
    m_list->setStyleSheet(QStringLiteral(
        "QListWidget {"
        "  background: #0c1424; color: #c0c8d8;"
        "  border: 1px solid #1a2640;"
        "  font-family: Consolas, monospace;"
        "}"
        "QListWidget::item { padding: 8px 6px; }"
        "QListWidget::item:selected { background: #1c2c52; color: #fff; }"
    ));
    root->addWidget(m_list, 1);
    connect(m_list, &QListWidget::itemSelectionChanged,
            this, &XboxConsolePickerDialog::onSelectionChanged);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem*) { onAccept(); });

    m_status = new QLabel(QStringLiteral("Looking for consoles…"), this);
    m_status->setWordWrap(true);
    m_status->setStyleSheet(QStringLiteral("color: #c0c8d8; font-weight: 600;"));
    root->addWidget(m_status);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), this);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(cancel);
    m_okBtn = new QPushButton(QStringLiteral("start stream  →"), this);
    m_okBtn->setEnabled(false);
    m_okBtn->setDefault(true);
    connect(m_okBtn, &QPushButton::clicked, this, &XboxConsolePickerDialog::onAccept);
    btnRow->addWidget(m_okBtn);
    root->addLayout(btnRow);
}

void XboxConsolePickerDialog::setConsoles(const QVector<XboxConsoleEntry>& consoles)
{
    m_entries = consoles;
    m_list->clear();
    for (int i = 0; i < consoles.size(); ++i) {
        const auto& c = consoles[i];
        const QString stateBadge = c.state.isEmpty()
            ? QString()
            : QStringLiteral("  [%1]").arg(c.state);
        const QString text = QStringLiteral("%1%2").arg(
            c.name.isEmpty() ? c.id : c.name,
            stateBadge);
        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, i);
        m_list->addItem(item);
    }
    if (!consoles.isEmpty()) {
        m_list->setCurrentRow(0);
        setStatus(QStringLiteral("Found %1 console%2.")
                  .arg(consoles.size())
                  .arg(consoles.size() == 1 ? QString() : QStringLiteral("s")));
    } else {
        setStatus(QStringLiteral("No consoles found on this account."), true);
    }
}

void XboxConsolePickerDialog::setStatus(const QString& message, bool error)
{
    if (!m_status) return;
    m_status->setStyleSheet(error
        ? QStringLiteral("color: #d55; font-weight: 600;")
        : QStringLiteral("color: #c0c8d8; font-weight: 600;"));
    m_status->setText(message);
}

void XboxConsolePickerDialog::onSelectionChanged()
{
    const auto* item = m_list->currentItem();
    m_okBtn->setEnabled(item != nullptr);
}

void XboxConsolePickerDialog::onAccept()
{
    const auto* item = m_list->currentItem();
    if (!item) return;
    const int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_entries.size()) return;
    m_selectedId   = m_entries[idx].id;
    m_selectedName = m_entries[idx].name;
    accept();
}

} // namespace Labs
