#include "XboxWebPlugin.h"
#include "WebView2Widget.h"
#include "SettingsManager.h"

#include <QDir>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>

namespace Labs {

namespace {
constexpr auto kPlayUrl = "https://www.xbox.com/play";
constexpr auto kUserAgent =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0";
} // namespace

XboxWebPlugin::XboxWebPlugin() = default;
XboxWebPlugin::~XboxWebPlugin() = default;

void XboxWebPlugin::initialize(const PluginContext& ctx)
{
    m_settings = ctx.settings;
}

void XboxWebPlugin::shutdown()
{
    m_sessions.clear();
}

QString XboxWebPlugin::sanitizeKey(const QString& raw) const
{
    QString out;
    out.reserve(raw.size());
    for (QChar c : raw) {
        const ushort u = c.unicode();
        const bool ok = (u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z')
                     || (u >= '0' && u <= '9') || u == '.' || u == '_' || u == '-';
        out.append(ok ? c : QChar('_'));
    }
    if (out.isEmpty()) out = QStringLiteral("default");
    return out;
}

QString XboxWebPlugin::profileDirFor(const QString& key) const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir d(base);
    d.mkpath(QStringLiteral("xbox-web/profiles/%1").arg(key));
    return d.absoluteFilePath(QStringLiteral("xbox-web/profiles/%1").arg(key));
}

int XboxWebPlugin::findSessionByKey(const QString& key) const
{
    for (const auto& s : m_sessions) if (s.accountKey == key) return s.id;
    return -1;
}

QWidget* XboxWebPlugin::createWidget(QWidget* parent)
{
    auto* root = new QWidget(parent);
    root->setStyleSheet(QStringLiteral("background:#070A14;"));
    auto* outer = new QVBoxLayout(root);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_gridHost = new QWidget(root);
    m_gridHost->setStyleSheet(QStringLiteral("background:#070A14;"));
    m_grid = new QGridLayout(m_gridHost.data());
    m_grid->setContentsMargins(4, 4, 4, 4);
    m_grid->setSpacing(4);
    outer->addWidget(m_gridHost.data(), 1);

    m_placeholder = new QWidget(m_gridHost.data());
    auto* placeLayout = new QVBoxLayout(m_placeholder.data());
    placeLayout->setAlignment(Qt::AlignCenter);
    auto* hint = new QLabel(QStringLiteral(
        "<div style='color:#94A3B8;font-family:Segoe UI;text-align:center;'>"
          "<div style='font-size:24px;font-weight:600;color:#F1F5F9;'>no active session</div>"
          "<div style='margin-top:14px;font-size:13px;'>Click <b style='color:#60A5FA;'>+ new session</b> in the top bar to start one.</div>"
        "</div>"), m_placeholder.data());
    hint->setTextFormat(Qt::RichText);
    hint->setAlignment(Qt::AlignCenter);
    placeLayout->addWidget(hint);

    m_grid->addWidget(m_placeholder.data(), 0, 0);
    return root;
}

void XboxWebPlugin::relayoutGrid()
{
    if (!m_grid || !m_gridHost) return;
    // Strip every item off the grid (without deleting the widgets).
    while (m_grid->count() > 0) {
        QLayoutItem* it = m_grid->takeAt(0);
        if (!it) break;
        if (auto* w = it->widget()) w->setParent(nullptr);
        delete it;
    }
    // Reset row/col stretch factors. QGridLayout doesn't auto-clear these when
    // items are removed, so leftover stretches from N tiles ago would leave the
    // placeholder confined to a corner cell. Walk a generous range and zero out.
    for (int i = 0; i < 8; ++i) {
        m_grid->setColumnStretch(i, 0);
        m_grid->setRowStretch(i, 0);
    }
    const int n = m_sessions.size();
    if (n == 0) {
        if (m_placeholder) {
            m_placeholder->setParent(m_gridHost.data());
            m_grid->addWidget(m_placeholder.data(), 0, 0);
            m_grid->setColumnStretch(0, 1);
            m_grid->setRowStretch(0, 1);
            m_placeholder->setVisible(true);
        }
        return;
    }
    if (m_placeholder) m_placeholder->setVisible(false);

    // Tile auto-layout — square-ish grid, prefer wider cells over tall ones
    // even if it leaves one cell empty. cols = ceil(sqrt(n)) gives:
    //   1→1×1, 2→2×1, 3→2×2 (1 empty), 4→2×2, 5–6→3×2, 7–9→3×3, 10+→ceil-sqrt.
    const int cols = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(n)))));
    const int rows = (n + cols - 1) / cols;
    // Ensure equal cell sizes by setting stretch on each row/col uniformly.
    for (int c = 0; c < cols; ++c) m_grid->setColumnStretch(c, 1);
    for (int r = 0; r < rows; ++r) m_grid->setRowStretch(r, 1);
    int idx = 0;
    for (auto& s : m_sessions) {
        QWidget* cell = s.tile ? s.tile.data() : (s.view ? s.view.data() : nullptr);
        if (!cell) { ++idx; continue; }
        const int r = idx / cols;
        const int c = idx % cols;
        cell->setParent(m_gridHost.data());
        m_grid->addWidget(cell, r, c);
        cell->setVisible(true);
        ++idx;
    }
    // Force the layout to recompute. Without this, QGridLayout retains its
    // previous column count from a removed tile and the survivor stays in
    // its old slot (left half of a 2-col grid even though only 1 cell exists).
    m_grid->invalidate();
    m_grid->activate();
    if (m_gridHost) {
        m_gridHost->updateGeometry();
        m_gridHost->update();
    }
}

bool XboxWebPlugin::pair(QWidget* parent)
{
    if (!m_gridHost) {
        QMessageBox::warning(parent, QStringLiteral("Xbox Web"),
            QStringLiteral("UI not ready yet."));
        return false;
    }

    QString suggested;
    for (int i = 1; i <= kMaxSessions; ++i) {
        const QString candidate = QStringLiteral("acct%1").arg(i);
        if (findSessionByKey(candidate) < 0) { suggested = candidate; break; }
    }
    if (suggested.isEmpty()) suggested = QStringLiteral("acct1");

    bool ok = false;
    const QString rawKey = QInputDialog::getText(parent,
        QStringLiteral("New xCloud session"),
        QStringLiteral("Name this session (cookies isolated per name):"),
        QLineEdit::Normal, suggested, &ok);
    if (!ok) return false;
    const QString key = sanitizeKey(rawKey.trimmed());

    // Existing? Just focus.
    const int existingId = findSessionByKey(key);
    if (existingId >= 0) return focusSession(existingId);

    if (m_sessions.size() >= kMaxSessions) {
        QMessageBox::warning(parent, QStringLiteral("Xbox Web"),
            QStringLiteral("Session limit reached (%1).").arg(kMaxSessions));
        return false;
    }

    // Build a tile: small header (label + close ×) on top, WebView2 below.
    auto* tile = new QWidget(m_gridHost.data());
    tile->setStyleSheet(QStringLiteral(
        "QWidget{background:#0B1020;}"
        "QLabel#tileLabel{color:#F1F5F9;font-family:'Segoe UI Variable Display','Segoe UI';"
            "font-size:11px;font-weight:600;padding-left:6px;}"
        "QToolButton#tileClose{color:#94A3B8;background:transparent;border:none;"
            "font-size:13px;padding:2px 8px;}"
        "QToolButton#tileClose:hover{color:#F87171;background:#11182C;}"
    ));
    auto* col = new QVBoxLayout(tile);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    auto* header = new QWidget(tile);
    header->setFixedHeight(22);
    header->setStyleSheet(QStringLiteral("background:#0B1020; border-bottom:1px solid #1E2A4A;"));
    auto* hbar = new QHBoxLayout(header);
    hbar->setContentsMargins(2, 0, 0, 0);
    hbar->setSpacing(0);
    auto* tileLabel = new QLabel(QStringLiteral("xCloud · %1").arg(key), header);
    tileLabel->setObjectName(QStringLiteral("tileLabel"));
    auto* focusBtn = new QToolButton(header);
    focusBtn->setObjectName(QStringLiteral("tileClose"));
    focusBtn->setText(QStringLiteral("🎮"));
    focusBtn->setToolTip(QStringLiteral("Send physical controller input to this session"));
    focusBtn->setCursor(Qt::PointingHandCursor);
    auto* runBtn = new QToolButton(header);
    runBtn->setObjectName(QStringLiteral("tileClose"));
    runBtn->setText(QStringLiteral("▶"));
    runBtn->setToolTip(QStringLiteral("Run script for this session"));
    runBtn->setCursor(Qt::PointingHandCursor);
    auto* closeBtn = new QToolButton(header);
    closeBtn->setObjectName(QStringLiteral("tileClose"));
    closeBtn->setText(QStringLiteral("✕"));
    closeBtn->setToolTip(QStringLiteral("Close this session"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    hbar->addWidget(tileLabel);
    hbar->addStretch();
    hbar->addWidget(focusBtn);
    hbar->addWidget(runBtn);
    hbar->addWidget(closeBtn);
    col->addWidget(header);

    auto* view = new WebView2Widget(profileDirFor(key),
                                    QString::fromUtf8(kPlayUrl),
                                    QString::fromUtf8(kUserAgent),
                                    tile);
    col->addWidget(view, 1);

    XboxWebSession sess;
    sess.id         = m_nextId++;
    sess.accountKey = key;
    sess.view       = view;
    sess.tile       = tile;
    sess.runBtn     = runBtn;
    // New sessions start with the gamepad gate OFF — only the focused tile
    // gets physical input. The focusSession() call below flips this on for
    // the newly-created tile (which is auto-focused).
    view->setGamepadEnabled(false);
    m_sessions.append(sess);

    const int sid = sess.id;
    QObject::connect(closeBtn, &QToolButton::clicked, this, [this, sid]() {
        terminateSession(sid);
    });
    QObject::connect(runBtn, &QToolButton::clicked, this, [this, sid]() {
        // Toggle: emit run if not running, stop if running. The engine slot
        // setSessionScriptRunning(sid,true/false) flips state back to us.
        for (const auto& s : m_sessions) {
            if (s.id == sid) {
                if (s.scriptRunning) emit stopScriptRequested(sid);
                else                 emit runScriptRequested(sid);
                return;
            }
        }
    });
    QObject::connect(focusBtn, &QToolButton::clicked, this, [this, sid]() {
        focusSession(sid);
    });

    relayoutGrid();
    // Auto-focus the newly created session (so input flows to it right away).
    focusSession(sess.id);

    emit sessionAdded(sess.id, key);
    return true;
}

int XboxWebPlugin::sessionCount() const { return m_sessions.size(); }
int XboxWebPlugin::currentSessionId() const { return m_currentId; }

bool XboxWebPlugin::focusSession(int id)
{
    bool found = false;
    for (auto& s : m_sessions) {
        if (s.id == id) found = true;
    }
    if (!found) return false;

    m_currentId = id;
    // Per-tile gamepad gate (no-op for now until per-session ViGEm).
    // No visual tint — every tile keeps the same neutral header so closing
    // a session doesn't leave a blue artifact on the survivor.
    for (auto& s : m_sessions) {
        const bool active = (s.id == id);
        if (s.view) s.view->setGamepadEnabled(active);
    }
    emit sessionFocused(id);
    return true;
}

void XboxWebPlugin::terminateSession(int id)
{
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].id != id) continue;
        const bool wasFocused = (m_currentId == id);
        // Force-hide and explicitly close the WebView2 controller BEFORE
        // detaching from the layout. setParent(nullptr) alone leaves the
        // WebView2's native HWND alive as a top-level orphan window until
        // the Qt deleteLater fires next event-loop tick — that's the black
        // half-screen the user sees. close() runs the destructor's cleanup
        // synchronously and the layout sees the widget vanish immediately.
        if (m_sessions[i].view) {
            m_sessions[i].view->hide();
            m_sessions[i].view->close();
        }
        if (m_sessions[i].tile) {
            m_sessions[i].tile->hide();
            if (m_grid) m_grid->removeWidget(m_sessions[i].tile.data());
            m_sessions[i].tile->setParent(nullptr);
            m_sessions[i].tile->deleteLater();
        } else if (m_sessions[i].view) {
            m_sessions[i].view->deleteLater();
        }
        m_sessions.removeAt(i);
        relayoutGrid();
        if (wasFocused) {
            // The focused tile is gone — promote the first remaining (if any)
            // and re-apply focus styling so it doesn't render stale.
            if (!m_sessions.isEmpty()) {
                m_currentId = -1;        // force focusSession to flip-to-true
                focusSession(m_sessions.front().id);
            } else {
                m_currentId = -1;
            }
        }
        emit sessionRemoved(id);
        return;
    }
}

QString XboxWebPlugin::sessionLabel(int id) const
{
    for (const auto& s : m_sessions) if (s.id == id) return s.accountKey;
    return {};
}

void XboxWebPlugin::setSessionScriptRunning(int sessionId, bool running)
{
    for (auto& s : m_sessions) {
        if (s.id != sessionId) continue;
        s.scriptRunning = running;
        if (s.runBtn) {
            s.runBtn->setText(running ? QStringLiteral("■") : QStringLiteral("▶"));
            s.runBtn->setToolTip(running
                ? QStringLiteral("Stop the script running in this session")
                : QStringLiteral("Run script for this session"));
        }
        return;
    }
}

} // namespace Labs

extern "C" __declspec(dllexport) Labs::IPlugin* createPlugin()
{
    return new Labs::XboxWebPlugin();
}
