#pragma once

#include "IPlugin.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVector>

class QGridLayout;
class QToolButton;
class QWidget;

namespace Labs {

class SettingsManager;
class WebView2Widget;

struct XboxWebSession {
    int       id = -1;
    QString   accountKey;
    QPointer<WebView2Widget> view;
    QPointer<QWidget>        tile;     // wrapper widget added to the grid (holds header + view)
    ::QToolButton*           runBtn = nullptr;  // ▶ / ■ in the tile header (global Qt class)
    bool                     scriptRunning = false;
};

class XboxWebPlugin : public QObject,
                     public IUIPlugin,
                     public IPairablePlugin {
    Q_OBJECT
public:
    XboxWebPlugin();
    ~XboxWebPlugin() override;

    QString name()        const override { return QStringLiteral("Xbox Web"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("xCloud via embedded WebView2 — sign in on the page (multi-account)"); }
    QString version()     const override { return QStringLiteral("0.3.0"); }

    void initialize(const PluginContext& ctx) override;
    void shutdown() override;

    QObject* qobject() override { return this; }

    // IUIPlugin — returns a QStackedWidget hosting all session WebView2s.
    QWidget* createWidget(QWidget* parent) override;

    // IPairablePlugin — prompts for an account label, creates an isolated
    // WebView2 session, switches the stack to show it.
    bool pair(QWidget* parent) override;

    // Multi-session API exposed to the engine via meta-system.
    static constexpr int kMaxSessions = 8;
    Q_INVOKABLE int     sessionCount() const;
    Q_INVOKABLE int     currentSessionId() const;
    Q_INVOKABLE bool    focusSession(int id);
    Q_INVOKABLE void    terminateSession(int id);
    Q_INVOKABLE QString sessionLabel(int id) const;

signals:
    void sessionAdded(int id, const QString& label);
    void sessionRemoved(int id);
    void sessionFocused(int id);
    // Per-session script run/stop — engine handles the spawn so it can pass
    // the same env vars / cv-scripts dir / --session-id arg uniformly.
    void runScriptRequested(int sessionId);
    void stopScriptRequested(int sessionId);

public slots:
    // Engine tells us when a session's script process actually started/stopped
    // so we can flip the tile button between ▶ and ■.
    void setSessionScriptRunning(int sessionId, bool running);

private:
    QString sanitizeKey(const QString& raw) const;
    QString profileDirFor(const QString& key) const;
    int     findSessionByKey(const QString& key) const;

    void relayoutGrid();    // re-tile sessions when count changes

    SettingsManager*           m_settings = nullptr;
    QPointer<QWidget>          m_gridHost;     // outer container with QGridLayout
    QPointer<QGridLayout>      m_grid;
    QPointer<QWidget>          m_placeholder;  // shown when sessionCount == 0
    QVector<XboxWebSession>    m_sessions;
    int                        m_nextId = 0;
    int                        m_currentId = -1;
};

} // namespace Labs
