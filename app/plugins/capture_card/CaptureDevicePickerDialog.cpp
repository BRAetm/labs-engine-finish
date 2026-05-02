#include "CaptureDevicePickerDialog.h"

#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace Labs {

namespace {

// Labs deep-blue theme — matches PairPSDialog / PsnLoginDialog look.
constexpr const char* kSheet = R"(
    QDialog {
        background: #0b1320;
        color: #e6ecf6;
    }
    QLabel#header {
        color: #ffffff;
        font-size: 22px;
        font-weight: 600;
        letter-spacing: 0.4px;
    }
    QLabel#sub {
        color: #6e7c93;
        font-size: 11px;
        letter-spacing: 1.2px;
    }
    QListWidget {
        background: #0f1828;
        border: 1px solid #1c2638;
        border-radius: 6px;
        padding: 6px;
        color: #e6ecf6;
        outline: none;
    }
    QListWidget::item {
        background: transparent;
        border: 1px solid transparent;
        border-radius: 4px;
        padding: 10px 12px;
        margin: 2px 0;
    }
    QListWidget::item:selected {
        background: #16243d;
        border: 1px solid #2a6df0;
        color: #ffffff;
    }
    QListWidget::item:disabled {
        color: #4a5468;
    }
    QPushButton {
        background: #1a2538;
        color: #e6ecf6;
        border: 1px solid #2a3854;
        border-radius: 4px;
        padding: 8px 16px;
        min-height: 28px;
    }
    QPushButton:hover {
        background: #233252;
        border-color: #3a4f7a;
    }
    QPushButton:disabled {
        color: #4a5468;
        background: #131c2c;
        border-color: #1c2638;
    }
    QPushButton[ghost="true"] {
        background: transparent;
        border: 1px solid #2a3854;
    }
    QPushButton[ghost="true"]:hover {
        background: #16243d;
    }
    QPushButton[primary="true"] {
        background: #2a6df0;
        color: #ffffff;
        border: 1px solid #2a6df0;
        font-weight: 600;
    }
    QPushButton[primary="true"]:hover {
        background: #3a7df8;
        border-color: #3a7df8;
    }
    QPushButton[primary="true"]:disabled {
        background: #182441;
        color: #5b6c8a;
        border-color: #1d2a4a;
    }
)";

QString rowHtml(const CaptureDeviceInfo& d)
{
    const QString badge = d.IsSupported
        ? QString()
        : QStringLiteral("<div style='color:#e85a5a; font-size:11px; "
                         "font-family:Consolas,monospace; margin-top:4px'>"
                         "unsupported &mdash; %1</div>")
              .arg(d.UnsupportedReason.toHtmlEscaped());

    const QString meta = QStringLiteral(
        "<div style='color:#6f8cd0; font-family:Consolas,monospace; "
        "font-size:11px; margin-top:2px'>#%1 &middot; %2&times;%3</div>")
        .arg(d.Index)
        .arg(d.Width)
        .arg(d.Height);

    return QStringLiteral(
        "<div style='font-weight:600; color:#ffffff; font-size:13px'>%1</div>"
        "%2%3")
        .arg(d.FriendlyName.toHtmlEscaped(), meta, badge);
}

} // namespace

CaptureDevicePickerDialog::CaptureDevicePickerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Capture Card"));
    setModal(true);
    setMinimumSize(520, 420);
    setStyleSheet(QString::fromLatin1(kSheet));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 22, 22, 22);
    root->setSpacing(14);

    auto* header = new QLabel(QStringLiteral("select capture card"), this);
    header->setObjectName(QStringLiteral("header"));

    auto* sub = new QLabel(QStringLiteral("1920 × 1080 · 60 fps · MJPG"), this);
    sub->setObjectName(QStringLiteral("sub"));

    m_list = new QListWidget(this);
    m_list->setUniformItemSizes(false);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    m_emptyHint = new QLabel(this);
    m_emptyHint->setText(QStringLiteral(
        "<span style='color:#6e7c93'>no capture cards detected.<br/>"
        "plug a card in and click <b>rescan</b>.</span>"));
    m_emptyHint->setVisible(false);
    m_emptyHint->setAlignment(Qt::AlignCenter);

    m_btnRescan = new QPushButton(QStringLiteral("rescan"), this);
    m_btnRescan->setProperty("ghost", true);
    m_btnCancel = new QPushButton(QStringLiteral("cancel"), this);
    m_btnCancel->setProperty("ghost", true);
    m_btnStart  = new QPushButton(QStringLiteral("start capture  →"), this);
    m_btnStart->setProperty("primary", true);
    m_btnStart->setEnabled(false);

    auto* bottom = new QHBoxLayout();
    bottom->setSpacing(8);
    bottom->addWidget(m_btnRescan);
    bottom->addStretch();
    bottom->addWidget(m_btnCancel);
    bottom->addWidget(m_btnStart);

    root->addWidget(header);
    root->addWidget(sub);
    root->addWidget(m_list, 1);
    root->addWidget(m_emptyHint);
    root->addLayout(bottom);

    connect(m_btnRescan, &QPushButton::clicked, this, &CaptureDevicePickerDialog::onRescan);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnStart,  &QPushButton::clicked, this, &CaptureDevicePickerDialog::onAccept);
    connect(m_list,      &QListWidget::itemSelectionChanged,
            this, &CaptureDevicePickerDialog::onSelectionChanged);

    rescanDevices();
}

CaptureDeviceInfo CaptureDevicePickerDialog::selectedDevice() const
{
    if (m_selectedRow < 0 || m_selectedRow >= static_cast<int>(m_devices.size()))
        return {};
    return m_devices[m_selectedRow];
}

void CaptureDevicePickerDialog::onRescan()
{
    rescanDevices();
}

void CaptureDevicePickerDialog::rescanDevices()
{
    m_btnRescan->setEnabled(false);
    m_btnRescan->setText(QStringLiteral("scanning…"));
    QCoreApplication::processEvents();

    m_devices = MfCaptureSource::scanDevices();
    m_selectedRow = -1;
    rebuildList();

    m_btnRescan->setEnabled(true);
    m_btnRescan->setText(QStringLiteral("rescan"));
    m_btnStart->setEnabled(false);
}

void CaptureDevicePickerDialog::rebuildList()
{
    m_list->clear();

    for (const auto& d : m_devices) {
        auto* item = new QListWidgetItem();
        // Use a label widget to render rich-text rows.
        auto* row = new QLabel(rowHtml(d));
        row->setTextFormat(Qt::RichText);
        row->setContentsMargins(4, 4, 4, 4);

        item->setSizeHint(QSize(0, d.IsSupported ? 56 : 78));
        if (!d.IsSupported) {
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
        }
        m_list->addItem(item);
        m_list->setItemWidget(item, row);
    }

    const bool empty = m_devices.empty();
    m_emptyHint->setVisible(empty);
    m_list->setVisible(!empty);
}

void CaptureDevicePickerDialog::onSelectionChanged()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= static_cast<int>(m_devices.size())) {
        m_selectedRow = -1;
        m_btnStart->setEnabled(false);
        return;
    }
    if (!m_devices[row].IsSupported) {
        m_selectedRow = -1;
        m_btnStart->setEnabled(false);
        return;
    }
    m_selectedRow = row;
    m_btnStart->setEnabled(true);
}

void CaptureDevicePickerDialog::onAccept()
{
    if (m_selectedRow < 0) return;
    accept();
}

} // namespace Labs
