#pragma once

#include "IPlugin.h"
#include "XboxStreamSession.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVector>

namespace Labs {

class SettingsManager;

class XboxStreamPlugin : public QObject,
                         public IPairablePlugin {
    Q_OBJECT
public:
    XboxStreamPlugin();
    ~XboxStreamPlugin() override;

    QString name()        const override { return QStringLiteral("Xbox Stream"); }
    QString author()      const override { return QStringLiteral("Labs"); }
    QString description() const override { return QStringLiteral("Multi-session Xbox Remote Play / xCloud (up to 8)"); }
    QString version()     const override { return QStringLiteral("0.3.0"); }

    void initialize(const PluginContext& ctx) override;
    void shutdown() override;

    QObject* qobject() override { return this; }

    // IPairablePlugin — opens the sign-in / new-session flow.
    bool pair(QWidget* parent) override;

    // Multi-session API. Engine UI calls these to manage sessions.
    // Q_INVOKABLE so the engine can call across the plugin DLL boundary
    // via QMetaObject::invokeMethod (no link-time dependency).
    static constexpr int kMaxSessions = 8;
    Q_INVOKABLE int                sessionCount() const;
    Q_INVOKABLE QObject*           sessionAtObject(int index) const;
    Q_INVOKABLE QObject*           sessionByIdObject(int id) const;
    XboxStreamSession*             sessionAt(int index) const;
    XboxStreamSession*             sessionById(int id) const;

    // Create a new session. Returns the session, or nullptr if at limit / error.
    Q_INVOKABLE XboxStreamSession* createSession(const QString& accountKey,
                                                 Labs::XboxStreamMode mode,
                                                 const QString& consoleId = QString());

    // Terminate session by id and remove from the list.
    Q_INVOKABLE void terminateSession(int id);

signals:
    void sessionAdded(int id);
    void sessionRemoved(int id);
    void sessionEvent(int id, const QString& topic, const QByteArray& payload);

private:
    QString locateSidecarDir() const;
    QString locateElectronExe(const QString& sidecarDir) const;
    bool    pickConsoleForAccount(QWidget* parent,
                                  const QString& accountKey,
                                  QString* outConsoleId,
                                  QString* outConsoleName);

    void    pruneTombstones();
    void    onSessionEvent(int id, const QByteArray& payload);

    SettingsManager* m_settings = nullptr;
    QString          m_sidecarDir;
    QVector<QPointer<XboxStreamSession>> m_sessions;
    int              m_nextId = 0;
};

} // namespace Labs
