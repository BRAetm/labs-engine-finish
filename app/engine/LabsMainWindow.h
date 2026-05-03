#pragma once

#include <QMainWindow>
#include <QMap>
#include <QPointer>
#include <memory>
#include <vector>

class QAction;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QStackedWidget;
class QTimer;

namespace Labs {

class PluginHost;
class SettingsManager;
class IPlugin;
class IFrameSource;
class IFrameSink;
class IControllerSource;
class IControllerSink;
class LabsBackgroundWidget;
class ControllerMonitorWidget;
class Ps5Discovery;

class LabsMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit LabsMainWindow(QWidget* parent = nullptr);
    ~LabsMainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;

private slots:
    void onModeChanged(int index);
    void onPair();
    void onStart();
    void onStop();
    void onFpsTick();
    void onBrowseScript();
    void onRefreshScripts();
    void onRunScript();
    void onStopScript();
    void onScriptFinished(int exitCode, int exitStatus);
    void onClearLog();
    void onOpenTheme();

    void onXboxWebRunRequested(int sessionId);
    void onXboxWebStopRequested(int sessionId);
    void onXboxWebSessionRemoved(int sessionId);

private:
    class FanOutFrameSink;
    class InferenceResultsFanOut;

    enum class Mode { Xbox = 0, PS = 1, Cloud = 2 };

    QWidget* buildTopBar();
    QWidget* buildScriptsRail();
    QWidget* buildCenterStage();
    QWidget* buildDevicesRail();

    void applyMode(Mode mode);
    void stopActiveSource();
    void updateActions();
    void refreshDevicesList();
    void appendLog(const QString& text);

    void applyThemeImage();  // re-reads imagePath from settings and updates root

    std::unique_ptr<SettingsManager> m_settings;
    std::unique_ptr<PluginHost>      m_pluginHost;
    std::unique_ptr<FanOutFrameSink>        m_fanOut;
    std::unique_ptr<InferenceResultsFanOut> m_resultsFanOut;

    QMap<QString, IFrameSource*>       m_frameSources;
    QMap<QString, IControllerSink*>    m_ctrlSinks;
    QMap<QString, IPlugin*>            m_pluginsByName;
    IControllerSource*                 m_ctrlSource   = nullptr;

    IFrameSource*    m_activeSource   = nullptr;
    IControllerSink* m_activeCtrlSink = nullptr;

    // Multi-session Xbox tracking. The plugin is a session factory now;
    // we track the focused session via its QObject* (engine doesn't link the
    // plugin DLL, so we can't hold a typed pointer). Cast to IFrameSource* /
    // IControllerSink* via dynamic_cast — those interfaces live in LabsCore.
    QPointer<QObject> m_focusedXboxSession;
    int m_focusedXboxSessionId = -1;
    void hookXboxStreamSignals();
    void onXboxSessionAdded(int id);
    void onXboxSessionRemoved(int id);
    void onXboxWindowHwndReady(int sessionId, quintptr hwnd);
    void focusXboxSession(QObject* session);

    // xCloud session container in the center stage. Each session = one
    // reparented Electron HWND inside its own host widget. Tabs at top to
    // switch between sessions; one visible at a time.
    QWidget*        m_xboxContainer  = nullptr;  // outer wrapper inside m_stageHost
    QWidget*        m_xboxTabStrip   = nullptr;  // horizontal row of QPushButton tabs
    QStackedWidget* m_xboxStack      = nullptr;  // pages, one per session
    QMap<int, QWidget*>      m_xboxHosts;        // sessionId → host widget (containing HWND)
    QMap<int, QPushButton*>  m_xboxTabs;         // sessionId → tab button
    void buildXboxContainer();
    void embedXboxHwnd(int sessionId, quintptr hwnd, const QString& label);
    void removeXboxHost(int sessionId);
    void showXboxContainer(bool show);

    // Per-xCloud-session script processes. Map keyed by sessionId.
    QMap<int, QProcess*> m_xboxScriptProcs;
    void hookXboxWebSignals();
    void spawnXboxSessionScript(int sessionId);
    void killXboxSessionScript(int sessionId);

    // Top bar controls.
    QComboBox*   m_modeBox    = nullptr;
    QPushButton* m_btnPair    = nullptr;
    QPushButton* m_btnStart   = nullptr;
    QPushButton* m_btnStop    = nullptr;
    QLabel*      m_statePill  = nullptr;

    // Left rail.
    QComboBox*   m_scriptCombo     = nullptr;
    QPushButton* m_scriptBrowseBtn = nullptr;
    QPushButton* m_scriptRunBtn    = nullptr;
    QPushButton* m_scriptStopBtn   = nullptr;
    QLabel*      m_scriptStatus    = nullptr;
    QProcess*    m_scriptProc      = nullptr;
    QPushButton* m_perfLiteBtn     = nullptr;
    QPushButton* m_perfProBtn      = nullptr;

    // Center stage.
    QLabel*          m_targetLabel = nullptr;
    QLabel*          m_fpsLabel    = nullptr;
    QWidget*         m_stageHost   = nullptr;
    QStackedWidget*  m_stagePages  = nullptr;
    QLabel*          m_tabVideo    = nullptr;
    QLabel*          m_tabMonitor  = nullptr;
    QLabel*          m_tabMarket   = nullptr;   // re-added — Marketplace tab in-engine
    ControllerMonitorWidget* m_monitor = nullptr;
    IControllerSource* m_xinputSource = nullptr;   // always-on feed for the monitor
    IControllerSource* m_dualSenseSource = nullptr; // HID-based PS pad source, feeds same fan-out

    // Right rail — Titan Bridge.
    QLabel*      m_devicesList        = nullptr;
    QComboBox*   m_titanDeviceCombo   = nullptr;
    QLabel*      m_titanDeviceStatus  = nullptr;
    QPushButton* m_titanRestartBtn    = nullptr;
    QLabel*      m_titanSlots[3]      = {};
    QComboBox*   m_titanOutputCombo   = nullptr;
    QLabel*      m_titanUsbLabels[4]  = {};

    // Right rail — Cronus Bridge.
    QComboBox*   m_cronusDeviceCombo  = nullptr;
    QLabel*      m_cronusDeviceStatus = nullptr;
    QPushButton* m_cronusRestartBtn   = nullptr;
    QLabel*      m_cronusSlots[3]     = {};
    QComboBox*   m_cronusOutputCombo  = nullptr;
    QLabel*      m_cronusUsbLabels[4] = {};

    // Log strip.
    QPlainTextEdit* m_log = nullptr;
    LabsBackgroundWidget* m_bgWidget = nullptr;

    QTimer*      m_fpsTimer = nullptr;
    qint64       m_lastFrameCount = 0;

    Ps5Discovery* m_ps5Discovery = nullptr;
};

} // namespace Labs
