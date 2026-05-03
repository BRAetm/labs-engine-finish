#include "LabsMainWindow.h"
#include "ControllerMonitorWidget.h"
#include "LabsBackgroundWidget.h"
#include "LabsLogoWidget.h"
#include "LabsPaths.h"
#include "LabsTheme.h"
#include "MarketplaceWidget.h"
#include "LabsThemeDialog.h"
#include "Ps5Discovery.h"

#include "IPlugin.h"
#include "InputOverride.h"
#include "PluginHost.h"
#include "SettingsManager.h"
// XboxStreamPlugin and XboxStreamSession live in a plugin DLL the engine
// doesn't link against. All cross-boundary access goes through Qt's meta
// system (QMetaObject::invokeMethod, string-form connect) and dynamic_cast
// to the core IFrameSource / IControllerSink interfaces (which DO live in
// LabsCore.lib that both engine and plugin link against).

// Win32 reparenting for embedding the Electron sidecar's BrowserWindow inside
// the Labs Engine main window.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <QResizeEvent>
#include <QShowEvent>

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QMenu>
#include <QMouseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QIcon>
#include <QScrollArea>
#include <QSize>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QEvent>
#include <QMouseEvent>
#include <QStackedWidget>
#include <QStyle>
#include <QStatusBar>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

namespace Labs {

// Frame-sink fan-out — two lanes:
//   - realtime: pushed synchronously on the source thread. Cheap because
//               the realtime sinks (DisplayPlugin) immediately enqueue to
//               the GUI thread via QMetaObject::invokeMethod.
//   - async:    a single dedicated worker thread with a 1-slot drop-oldest
//               queue. CvPython's SHM write can stall (paging, slow CV
//               consumer) and we DON'T want that to backpressure the
//               capture rate. Newer frames overwrite the unsent one in
//               the queue — CV consumers see the freshest frame, never
//               a backlog.
//
// Frame's _pool_holder shared_ptr keeps the FrameBufferPool slot reserved
// for the async sink while the realtime sink is also using it; the slot
// returns to the pool only once the LAST consumer drops the Frame.
class LabsMainWindow::FanOutFrameSink : public IFrameSink {
public:
    FanOutFrameSink() = default;
    ~FanOutFrameSink() {
        {
            std::lock_guard<std::mutex> lk(m_mx);
            m_running = false;
            m_pending.reset();
        }
        m_cv.notify_all();
        if (m_worker.joinable()) m_worker.join();
    }

    // Default registration is realtime — back-compat with existing call sites.
    void addSink(IFrameSink* s) {
        if (!s) return;
        std::lock_guard<std::mutex> lk(m_mx);
        m_realtime.push_back(s);
    }
    // Async registration — sink runs on the worker thread, off the source
    // critical path. Use for slow / blocking sinks (CvPython SHM).
    void addAsyncSink(IFrameSink* s) {
        if (!s) return;
        std::lock_guard<std::mutex> lk(m_mx);
        m_async.push_back(s);
    }

    // Deferred worker start: keep the worker thread off until plugin
    // registration is complete, otherwise asyncLoop races addAsyncSink.
    void startWorker() {
        if (m_running.exchange(true)) return;
        m_worker = std::thread([this]{ asyncLoop(); });
    }

    void pushFrame(const Frame& frame) override {
        // Snapshot under the lock so we don't read while addSink is mutating
        // the vectors. Copying pointer vectors is cheap.
        std::vector<IFrameSink*> realtime;
        bool haveAsync = false;
        {
            std::lock_guard<std::mutex> lk(m_mx);
            realtime = m_realtime;
            haveAsync = !m_async.empty();
            if (haveAsync) {
                m_pending = frame;       // shared_ptr / QByteArray refs bumped
            }
        }
        // Realtime lane — synchronous on the source thread (outside lock).
        for (auto* s : realtime) s->pushFrame(frame);

        // Async lane — drop-oldest. Replace whatever's pending so the
        // worker always picks up the freshest frame.
        if (haveAsync) m_cv.notify_one();
    }

private:
    void asyncLoop() {
        while (true) {
            Frame f;
            std::vector<IFrameSink*> async;
            {
                std::unique_lock<std::mutex> lk(m_mx);
                m_cv.wait(lk, [this]{ return !m_running || m_pending.has_value(); });
                if (!m_running) return;
                f = std::move(*m_pending);
                m_pending.reset();
                async = m_async;
            }
            for (auto* s : async) s->pushFrame(f);
        }
    }

    std::vector<IFrameSink*>  m_realtime;
    std::vector<IFrameSink*>  m_async;
    std::thread               m_worker;
    std::mutex                m_mx;
    std::condition_variable   m_cv;
    std::optional<Frame>      m_pending;
    std::atomic<bool>         m_running{false};
};

// Inference results fan-out. One frame processor → many sinks (logger,
// overlay, future assisted-aim). Synchronous: sinks run on the producing
// processor's worker thread. Sinks must be quick (mutex+write or copy+
// QueuedConnection) — anything heavier should marshal off the worker.
class LabsMainWindow::InferenceResultsFanOut : public IInferenceResultsSink {
public:
    void addSink(IInferenceResultsSink* s) {
        if (!s) return;
        std::lock_guard<std::mutex> lk(m_mx);
        m_sinks.push_back(s);
    }
    void onResults(const InferenceResults& r) override {
        std::vector<IInferenceResultsSink*> snap;
        {
            std::lock_guard<std::mutex> lk(m_mx);
            snap = m_sinks;
        }
        for (auto* s : snap) s->onResults(r);
    }
private:
    std::vector<IInferenceResultsSink*> m_sinks;
    std::mutex                          m_mx;
};

// Simple controller fan-out with one always-on monitor slot and a swappable
// output slot (ViGEm vs PS, depending on mode).
class FanOutCtrlSink : public IControllerSink {
public:
    void setMonitor(IControllerSink* s)  { m_monitor = s; }
    void setOutput(IControllerSink* s)   { m_output  = s; }
    // Filters mutate state in flight (assisted aim, accessibility tweaks).
    // Registered once during plugin init; the vector is read-only after
    // startup so the hot path doesn't need a lock.
    void addFilter(IControllerStateFilter* f) {
        if (f) m_filters.push_back(f);
    }
    void pushState(const ControllerState& state) override {
        // Apply external SHM-based overrides (Labs2K shot release etc.) HERE
        // — at the fan-out — so EVERY source pushing through us sees the
        // override. Doing it at individual sources lost the race against
        // parallel pushes from XInputSource.
        ControllerState s = state;
        InputOverride::instance().apply(s);
        for (auto* f : m_filters) f->apply(s);
        if (m_monitor) m_monitor->pushState(s);
        if (m_output)  m_output ->pushState(s);
    }
private:
    IControllerSink* m_monitor = nullptr;
    IControllerSink* m_output  = nullptr;
    std::vector<IControllerStateFilter*> m_filters;
};
static FanOutCtrlSink s_ctrlFanOut;

// Small helpers for consistent labels. All style comes from LabsTheme.cpp's
// central stylesheet via object names — nothing is hardcoded here so the
// theme dialog re-tints them when the user picks a different accent.
static QLabel* eyebrowLabel(const QString& text, QWidget* parent)
{
    auto* l = new QLabel(text.toUpper(), parent);
    l->setObjectName(QStringLiteral("eyebrow"));
    return l;
}
static QLabel* sectionLabel(const QString& text, QWidget* parent)
{
    auto* l = new QLabel(text, parent);
    l->setObjectName(QStringLiteral("sectionTitle"));
    return l;
}
// Reads LabsSharp's paired-host store at %AppData%/PSRemotePlay/ and copies the
// registration credentials into our settings (base64 strings in the same form
// PSRemotePlayPlugin::start() expects). No-op if our settings already have a
// regist key, or if LabsSharp's files aren't present.
static void importLabsSharpPairing(SettingsManager* s)
{
    if (!s) return;
    if (!s->value(QStringLiteral("ps/registKey")).toByteArray().isEmpty()) return;

    const QString appData = QString::fromLocal8Bit(qgetenv("APPDATA"));
    if (appData.isEmpty()) return;
    const QString base        = appData + QStringLiteral("/PSRemotePlay");
    const QString hostsPath   = base + QStringLiteral("/hosts.json");
    const QString accountPath = base + QStringLiteral("/account.txt");

    QFile hostsFile(hostsPath);
    if (!hostsFile.exists() || !hostsFile.open(QIODevice::ReadOnly)) return;

    const QJsonDocument doc = QJsonDocument::fromJson(hostsFile.readAll());
    hostsFile.close();
    if (!doc.isObject()) return;
    const QJsonObject hosts = doc.object();
    if (hosts.isEmpty()) return;

    // Take the first paired host.
    const QJsonObject host = hosts.begin().value().toObject();
    const QString registKey = host.value(QStringLiteral("RpRegistKey")).toString();
    const QString rpKey     = host.value(QStringLiteral("RpKey")).toString();
    const QString apName    = host.value(QStringLiteral("ApName")).toString();
    if (registKey.isEmpty() || rpKey.isEmpty()) return;

    s->setValue(QStringLiteral("ps/registKey"), registKey.toLatin1());
    s->setValue(QStringLiteral("ps/morning"),   rpKey.toLatin1());
    s->setValue(QStringLiteral("ps/isPs5"),     apName.startsWith(QStringLiteral("PS5"), Qt::CaseInsensitive));

    // Account ID is in a sibling text file.
    QFile accFile(accountPath);
    if (accFile.open(QIODevice::ReadOnly)) {
        const QString acc = QString::fromUtf8(accFile.readAll()).trimmed();
        accFile.close();
        if (!acc.isEmpty())
            s->setValue(QStringLiteral("ps/psnAccountId"), acc);
    }
    s->sync();
}

static QFrame* hSeparator(QWidget* parent)
{
    auto* s = new QFrame(parent);
    s->setObjectName(QStringLiteral("hrSep"));
    s->setFrameShape(QFrame::HLine);
    s->setFixedHeight(1);
    return s;
}

// True if the IP is in a private/LAN range — no point throttling for "internet" caps
// when traffic stays inside the home network.
//   192.168.0.0/16   10.0.0.0/8   172.16.0.0/12   169.254.0.0/16 (link-local)
static bool isLanIp(const QString& ip)
{
    if (ip.startsWith(QStringLiteral("192.168."))) return true;
    if (ip.startsWith(QStringLiteral("10.")))      return true;
    if (ip.startsWith(QStringLiteral("169.254.")))  return true;
    if (ip.startsWith(QStringLiteral("172."))) {
        const auto parts = ip.split(QChar('.'));
        if (parts.size() >= 2) {
            const int oct2 = parts[1].toInt();
            if (oct2 >= 16 && oct2 <= 31) return true;
        }
    }
    return false;
}

// Canonical user scripts folder. Auto-scanned on startup, also where downloaded
// scripts land. User can drop their own .py files in here and they show up.
//
// Wraps Labs::Paths::userScriptsDir() so the engine, launcher, marketplace,
// and future Zen Coder all share a single canonical location independent
// of each binary's per-app AppData. See app/core/LabsPaths.h.
static QString userScriptsDir()
{
    return Labs::Paths::userScriptsDir();
}

// On first launch, copy any bundled .py scripts into the user folder so the app
// has SOMETHING to run with. Won't overwrite if the user has already edited the
// script in their folder.
static void seedDefaultScripts()
{
    const QString userDir = userScriptsDir();
    // Look in install-dir-relative locations; first hit wins.
    QStringList sourceDirs = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/scripts"),       // installed: <install>/scripts/
        QDir::cleanPath(QCoreApplication::applicationDirPath()
                        + QStringLiteral("/../../../../labs-engine/scripts")),     // dev tree
    };
    for (const QString& src : sourceDirs) {
        if (!QDir(src).exists()) continue;
        for (const QFileInfo& fi : QDir(src).entryInfoList(
                 QStringList{QStringLiteral("*.py")}, QDir::Files)) {
            if (fi.fileName().startsWith(QChar('_'))) continue;
            const QString dst = userDir + QChar('/') + fi.fileName();
            if (!QFile::exists(dst)) {
                QFile::copy(fi.absoluteFilePath(), dst);
            }
        }
        // Also copy cv-scripts/ helper modules into user dir (sibling), so the
        // script's `sys.path.insert(0, ROOT.parent / "cv-scripts")` resolves.
        const QString cvSrc = QDir::cleanPath(src + QStringLiteral("/../cv-scripts"));
        const QString cvDst = QDir::cleanPath(userDir + QStringLiteral("/../cv-scripts"));
        if (QDir(cvSrc).exists()) {
            QDir().mkpath(cvDst);
            for (const QFileInfo& fi : QDir(cvSrc).entryInfoList(
                     QStringList{QStringLiteral("*.py")}, QDir::Files)) {
                const QString dst = cvDst + QChar('/') + fi.fileName();
                if (!QFile::exists(dst)) QFile::copy(fi.absoluteFilePath(), dst);
            }
        }
        
        // Also copy zp_core/ into the user's scripts folder
        const QString zpSrc = QDir::cleanPath(src + QStringLiteral("/zp_core"));
        const QString zpDst = QDir::cleanPath(userDir + QStringLiteral("/zp_core"));
        if (QDir(zpSrc).exists()) {
            QDir().mkpath(zpDst);
            for (const QFileInfo& fi : QDir(zpSrc).entryInfoList(
                     QStringList{QStringLiteral("*.py")}, QDir::Files)) {
                const QString dst = zpDst + QChar('/') + fi.fileName();
                if (!QFile::exists(dst)) QFile::copy(fi.absoluteFilePath(), dst);
            }
        }
        
        break;  // first matching source dir wins
    }
}

// ── ctor ────────────────────────────────────────────────────────────────────

LabsMainWindow::LabsMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_settings(std::make_unique<SettingsManager>())
    , m_pluginHost(std::make_unique<PluginHost>(this))
{
    setWindowTitle(QStringLiteral("Labs Engine"));
    importLabsSharpPairing(m_settings.get());
    const QByteArray geom  = m_settings->value(QStringLiteral("window/geometry")).toByteArray();
    const QByteArray state = m_settings->value(QStringLiteral("window/state")).toByteArray();
    if (!geom.isEmpty())  restoreGeometry(geom);
    else                  resize(1560, 900);
    if (!state.isEmpty()) restoreState(state);

    m_fpsTimer = new QTimer(this);
    m_fpsTimer->setInterval(1000);
    connect(m_fpsTimer, &QTimer::timeout, this, &LabsMainWindow::onFpsTick);

    // Build the three-column body: top bar above a H-split of rails + stage,
    // with a log strip at the bottom.
    QWidget* top   = buildTopBar();
    QWidget* left  = buildScriptsRail();
    QWidget* stage = buildCenterStage();
    QWidget* right = buildDevicesRail();

    auto* body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);
    body->addWidget(left);
    body->addWidget(stage, 1);
    body->addWidget(right);

    m_log = new QPlainTextEdit(this);
    m_log->setObjectName(QStringLiteral("logStrip"));
    m_log->setReadOnly(true);
    m_log->setMinimumHeight(70);
    m_log->setMaximumHeight(110);
    m_log->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_log->setLineWrapMode(QPlainTextEdit::NoWrap);

    // Log header strip — eyebrow label + clear button on a divider line above the log
    auto* logHeader = new QWidget(this);
    logHeader->setObjectName(QStringLiteral("logHeader"));
    logHeader->setFixedHeight(28);
    auto* logHeaderRow = new QHBoxLayout(logHeader);
    logHeaderRow->setContentsMargins(16, 0, 10, 0);
    logHeaderRow->setSpacing(8);
    auto* logLabel = eyebrowLabel(QStringLiteral("output log"), logHeader);
    auto* clearBtn = new QPushButton(QStringLiteral("clear"), logHeader);
    clearBtn->setProperty("ghost", true);
    clearBtn->setMinimumHeight(22);
    connect(clearBtn, &QPushButton::clicked, this, &LabsMainWindow::onClearLog);
    logHeaderRow->addWidget(logLabel);
    logHeaderRow->addStretch();
    logHeaderRow->addWidget(clearBtn);

    auto* logCol = new QVBoxLayout();
    logCol->setContentsMargins(0, 0, 0, 0);
    logCol->setSpacing(0);
    logCol->addWidget(logHeader);
    logCol->addWidget(m_log);

    auto* root = new QVBoxLayout();
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(top);
    root->addLayout(body, 1);
    root->addLayout(logCol);

    m_bgWidget = new LabsBackgroundWidget(this);
    m_bgWidget->setLayout(root);
    setCentralWidget(m_bgWidget);

    // Load plugins + wire.
    const QString pluginDir = QCoreApplication::applicationDirPath() + QStringLiteral("/plugins");
    const int n = m_pluginHost->loadAll(pluginDir);
    PluginContext ctx;
    ctx.settings = m_settings.get();
    m_fanOut        = std::make_unique<FanOutFrameSink>();
    m_resultsFanOut = std::make_unique<InferenceResultsFanOut>();

    // Collected during the plugin walk; wired up after the loop so every
    // IInferenceResultsSinkPlugin has joined the fan-out before any
    // processor's worker can fire its first onResults().
    std::vector<IFrameProcessor*> processorsToStart;

    IControllerSource* cvCtrlSource = nullptr;
    IControllerSource* xinputCtrlSource = nullptr;

    for (IPlugin* p : m_pluginHost->plugins()) {
        try { p->initialize(ctx); }
        catch (const std::exception& e) { appendLog(QStringLiteral("plugin %1 init threw: %2").arg(p->name(), e.what())); continue; }
        catch (...)                     { appendLog(QStringLiteral("plugin %1 init threw unknown").arg(p->name())); continue; }
        m_pluginsByName.insert(p->name(), p);
        if (auto* sp = dynamic_cast<IFrameSourcePlugin*>(p); sp)
            m_frameSources.insert(p->name(), sp->frameSource());
        if (auto* kp = dynamic_cast<IFrameSinkPlugin*>(p); kp) {
            // CvPython publishes to a SHM block that can stall (paging,
            // slow CV consumer) — route it through the async lane so a
            // stuck SHM write doesn't backpressure capture / display.
            if (p->name() == QStringLiteral("CV Python")) {
                m_fanOut->addAsyncSink(kp->frameSink());
            } else {
                m_fanOut->addSink(kp->frameSink());
            }
        }
        // IFrameProcessor — async lane like CvPython. Inference is heavy
        // by category and the realtime path must stay unblockable; the
        // plugin also drops-old internally, so two layers of drop-old
        // here is intentional, not redundant. start() is best-effort:
        // if its settings (e.g. onnx_inference/model_path) aren't
        // configured the plugin warns and pushFrame() becomes a no-op,
        // so capture stays uncontaminated.
        if (auto* fp = dynamic_cast<IFrameProcessorPlugin*>(p); fp) {
            if (auto* proc = fp->frameProcessor()) {
                m_fanOut->addAsyncSink(proc);
                processorsToStart.push_back(proc);
            }
        }
        if (auto* sp = dynamic_cast<IInferenceResultsSinkPlugin*>(p); sp) {
            if (auto* sink = sp->inferenceResultsSink())
                m_resultsFanOut->addSink(sink);
        }
        if (auto* fp = dynamic_cast<IControllerStateFilterPlugin*>(p); fp) {
            if (auto* filter = fp->controllerStateFilter())
                s_ctrlFanOut.addFilter(filter);
        }
        if (auto* sp = dynamic_cast<IControllerSourcePlugin*>(p); sp) {
            if      (p->name() == QStringLiteral("CV Python")) cvCtrlSource    = sp->controllerSource();
            else if (p->name() == QStringLiteral("XInput"))    xinputCtrlSource = sp->controllerSource();
            else if (p->name() == QStringLiteral("DualSense")) m_dualSenseSource = sp->controllerSource();
        }
        if (p->name() == QStringLiteral("XInput")) {
            if (auto* sp = dynamic_cast<IControllerSourcePlugin*>(p))
                m_xinputSource = sp->controllerSource();
        }
        if (auto* kp = dynamic_cast<IControllerSinkPlugin*>(p); kp)
            m_ctrlSinks.insert(p->name(), kp->controllerSink());

        // Mount every IUIPlugin into the center stage. applyMode() shows
        // the one matching the current mode and hides the rest. We need
        // BOTH the Display surface (PS mode + Xbox Stream sidecar) AND
        // the Xbox Web (WebView2 → xbox.com/play) widget mounted, then
        // switch visibility per mode.
        if (auto* ui = dynamic_cast<IUIPlugin*>(p); ui && m_stageHost) {
            QWidget* w = ui->createWidget(m_stageHost);
            if (w) {
                w->setProperty("uiPluginName", p->name());
                // Stretch the UI plugin widget so it fills the stage area
                // (esp. WebView2 — without stretch it collapses to its size hint).
                if (auto* vbox = qobject_cast<QVBoxLayout*>(m_stageHost->layout())) {
                    vbox->addWidget(w, 1);
                } else {
                    m_stageHost->layout()->addWidget(w);
                }
                w->setVisible(false);  // applyMode() reveals the right one
            }
        }
    }

    // All plugin sinks have registered — safe to start the async worker now.
    // Starting it earlier would race addAsyncSink / addSink mutations.
    if (m_fanOut) m_fanOut->startWorker();

    // Now that every IInferenceResultsSinkPlugin has joined the fan-out,
    // hand each processor its sink and start its worker. Doing this here
    // (rather than inline during the plugin walk) means no processor can
    // emit a result before all subscribers are wired in.
    for (IFrameProcessor* proc : processorsToStart) {
        proc->setResultsSink(m_resultsFanOut.get());
        proc->start();
    }

    // Cross-DLL queued signals carrying quintptr need explicit metatype
    // registration on the engine side (the plugin's MOC registered it on the
    // plugin side; both sides need it for marshaling to succeed).
    qRegisterMetaType<quintptr>("quintptr");

    // XboxStream is a session factory now (multi-session). Hook its signals so
    // we can route the focused session's IFrameSource into m_activeSource and
    // its IControllerSink into m_activeCtrlSink.
    hookXboxStreamSignals();
    hookXboxWebSignals();

    // Always run XInput. It feeds the controller monitor with live physical-pad
    // state at 125Hz and (with idleEmit on) keeps a heartbeat going so the ViGEm
    // virtual pad looks "alive" to xbox.com/play even when no script is running.
    // CV Python's controller-source path is legacy (current scripts use the
    // InputOverride SHM bridge instead); only use it if XInput isn't loaded.
    m_ctrlSource = xinputCtrlSource ? xinputCtrlSource : cvCtrlSource;

    // Single controller source pipe: primary source → fan-out → [monitor, mode output].
    s_ctrlFanOut.setMonitor(m_monitor);
    if (m_ctrlSource) {
        m_ctrlSource->setSink(&s_ctrlFanOut);
        m_ctrlSource->start();
    }
    // CvPython is also a frame-sink: when started, it publishes every fan-out
    // frame to the Labs_<pid>_Frame_<sessionId> SHM block so external scripts
    // can read decoded pixels directly. Even when XInput is the primary
    // controller source, we need CvPython running for that frame pipe.
    if (cvCtrlSource && cvCtrlSource != m_ctrlSource) {
        cvCtrlSource->start();
    }
    // DualSense runs in parallel so PS pads work even when ViGEmBus is absent.
    // It pushes into the same fan-out as the primary source; the XInputSource
    // simply emits nothing when no XInput pad is plugged in, so they don't
    // fight. Last-write-wins is fine — only one physical pad is ever moving.
    if (m_dualSenseSource && m_dualSenseSource != m_ctrlSource) {
        m_dualSenseSource->setSink(&s_ctrlFanOut);
        m_dualSenseSource->start();
    }

    const int savedMode = m_settings->value(QStringLiteral("session/mode"), 0).toInt();
    m_modeBox->setCurrentIndex(qBound(0, savedMode, 2));
    applyMode(static_cast<Mode>(m_modeBox->currentIndex()));

    connect(m_modeBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LabsMainWindow::onModeChanged);

    refreshDevicesList();
    qInfo() << "plugins:" << n << "sources:" << m_frameSources.size()
            << "ctrl sinks:" << m_ctrlSinks.size();
    applyThemeImage();
    updateActions();

    // Auto-discover the user's PS5 on the LAN. If a console responds we update
    // ps/host with its current IP — handles DHCP changes between sessions so
    // the user never has to type an IP. Pairing must already exist (regist key).
    const bool havePair = !m_settings->value(QStringLiteral("ps/registKey"))
                                      .toByteArray().isEmpty();
    if (havePair) {
        m_ps5Discovery = new Ps5Discovery(this);
        connect(m_ps5Discovery, &Ps5Discovery::hostsFound, this,
                [this](QList<Ps5Discovery::Host> hosts) {
            if (hosts.isEmpty()) {
                appendLog(QStringLiteral("PS5 discovery: no console found on network"));
                return;
            }
            // Prefer a PS5 over a PS4, otherwise first one wins.
            const Ps5Discovery::Host* pick = &hosts.first();
            for (const auto& h : hosts) {
                if (h.type.compare(QStringLiteral("PS5"), Qt::CaseInsensitive) == 0) {
                    pick = &h; break;
                }
            }
            const QString prev = m_settings->value(QStringLiteral("ps/host")).toString();
            if (prev != pick->ip) {
                m_settings->setValue(QStringLiteral("ps/host"), pick->ip);
                m_settings->sync();
                appendLog(QStringLiteral("PS5 found: %1 (%2) — saved as host")
                          .arg(pick->name.isEmpty() ? QStringLiteral("(unnamed)") : pick->name,
                               pick->ip));
            } else {
                appendLog(QStringLiteral("PS5 confirmed at %1").arg(pick->ip));
            }
        });
        m_ps5Discovery->discover(2500);
    }
}

void LabsMainWindow::applyThemeImage()
{
    if (!m_bgWidget || !m_settings) return;
    const LabsThemeData t = labsThemeLoad(m_settings.get());
    m_bgWidget->setImage(t.imagePath);
    m_bgWidget->setOpacity(t.imageOpacity);
}

LabsMainWindow::~LabsMainWindow()
{
    // ORDER MATTERS — see fix audit for crash-on-shutdown.
    //
    // 1) Stop every controller source thread BEFORE we let any child widget
    //    die. The sources push into s_ctrlFanOut on their own threads and
    //    s_ctrlFanOut holds raw pointers to m_monitor and m_activeCtrlSink
    //    (child widgets / plugin sinks). If a thread fires pushState after
    //    those have been destroyed we get a UAF (0xc0000409 stack-overrun).
    //
    //    NOTE: IControllerSource::stop() must be SYNCHRONOUS — it must join
    //    its internal thread before returning. All current implementations
    //    (XInputSource, DualSensePlugin, CvPythonPlugin) honor that. If a
    //    future plugin makes stop() async, add an explicit join/wait here.
    stopActiveSource();
    if (m_ctrlSource) m_ctrlSource->stop();
    if (m_dualSenseSource && m_dualSenseSource != m_ctrlSource) m_dualSenseSource->stop();
    // m_xinputSource is normally the same pointer as m_ctrlSource; only stop
    // it if it's somehow distinct (defensive — covers a hypothetical future
    // where the monitor source diverges from the primary).
    if (m_xinputSource && m_xinputSource != m_ctrlSource &&
        m_xinputSource != m_dualSenseSource) {
        m_xinputSource->stop();
    }
    // Any other IControllerSource the plugin layer surfaced (e.g. CV Python
    // started as the secondary frame-pipe at ctor) — stop them via the
    // plugin shutdown path. We iterate live plugins so we don't have to
    // track every source as a member.
    for (IPlugin* p : m_pluginsByName) {
        if (auto* sp = dynamic_cast<IControllerSourcePlugin*>(p)) {
            if (auto* cs = sp->controllerSource()) {
                if (cs != m_ctrlSource && cs != m_dualSenseSource &&
                    cs != m_xinputSource) {
                    cs->stop();
                }
            }
        }
    }

    // 2) Null out the static fan-out's pointers so any racing pushState
    //    that slipped through stop() (shouldn't, but cheap insurance) is
    //    a no-op instead of a deref into freed widget memory.
    s_ctrlFanOut.setMonitor(nullptr);
    s_ctrlFanOut.setOutput(nullptr);

    if (m_scriptProc && m_scriptProc->state() != QProcess::NotRunning) {
        m_scriptProc->terminate();
        m_scriptProc->waitForFinished(1000);
    }

    // 3) Now Qt can safely destroy child widgets, then unique_ptr members
    //    destruct in reverse declaration order: m_fanOut (joins its worker
    //    thread), then m_pluginHost (calls IPlugin::shutdown + DLL unload),
    //    then m_settings.
}

void LabsMainWindow::closeEvent(QCloseEvent* event)
{
    if (m_settings) {
        m_settings->setValue(QStringLiteral("window/geometry"), saveGeometry());
        m_settings->setValue(QStringLiteral("window/state"),    saveState());
        m_settings->setValue(QStringLiteral("session/mode"),    m_modeBox->currentIndex());
        if (m_scriptCombo) {
            m_settings->setValue(QStringLiteral("cv/scriptPath"), m_scriptCombo->currentData().toString());
        }
        m_settings->sync();
    }
    QMainWindow::closeEvent(event);
}

// ── top bar ─────────────────────────────────────────────────────────────────

QWidget* LabsMainWindow::buildTopBar()
{
    auto* bar = new QWidget(this);
    bar->setObjectName(QStringLiteral("topBar"));
    bar->setFixedHeight(56);

    bar->setFixedHeight(64);

    auto* logo = new LabsLogoWidget(bar);

    auto* wordmark = new QLabel(QStringLiteral("Labs Engine"), bar);
    wordmark->setObjectName(QStringLiteral("wordmark"));

    auto* version = new QLabel(QStringLiteral("v1.0 · creator studio"), bar);
    version->setObjectName(QStringLiteral("versionTag"));

    auto* titleColumn = new QVBoxLayout();
    titleColumn->setContentsMargins(0, 0, 0, 0);
    titleColumn->setSpacing(2);
    titleColumn->addWidget(wordmark);
    titleColumn->addWidget(version);

    // Engine state pill — only visible when capture or script is running.
    // Shows "▶ SecretK.py" while running so you can tell which script is live.
    m_statePill = new QLabel(QString(), bar);
    m_statePill->setObjectName(QStringLiteral("statePill"));
    m_statePill->setProperty("state", "running");
    m_statePill->setAlignment(Qt::AlignCenter);
    m_statePill->setVisible(false);

    // Active-session switcher (hidden when nothing is running). Stays in the
    // top bar but only surfaces once you've started something.
    auto* modeLabel = new QLabel(QStringLiteral("active"), bar);
    modeLabel->setObjectName(QStringLiteral("modeLabel"));

    m_modeBox = new QComboBox(bar);
    // Dropdown index → Mode enum (must stay in sync with enum values in .h).
    // Used only as the *type* of the active session — entries are populated
    // dynamically once sessions exist. Pre-populate with the kinds so the
    // legacy applyMode() switch keeps compiling; hidden until session count > 0.
    m_modeBox->addItem(QStringLiteral("Xbox Remote Play"));
    m_modeBox->addItem(QStringLiteral("PS Remote Play"));
    m_modeBox->addItem(QStringLiteral("xCloud"));
    m_modeBox->setMinimumWidth(190);
    modeLabel->setVisible(false);
    m_modeBox->setVisible(false);

    // ── primary action: + new session ─────────────────────────────────────
    auto* btnNewSession = new QPushButton(QStringLiteral("+ new session"), bar);
    btnNewSession->setProperty("accent", true);
    btnNewSession->setMinimumHeight(34);
    // Hidden actions wired below; the button triggers a Labs-themed modal
    // dialog with 4 cards which then triggers the right action.
    QAction* aCap   = new QAction(this);
    QAction* aPs    = new QAction(this);
    QAction* aXboxR = new QAction(this);
    QAction* aCloud = new QAction(this);
    connect(btnNewSession, &QPushButton::clicked, this, [this, aCap, aPs, aXboxR, aCloud]() {
        QDialog dlg(this);
        dlg.setWindowTitle(QStringLiteral("New session"));
        dlg.setStyleSheet(QStringLiteral(
            "QDialog{background:#070A14;}"
            "QFrame#card{background:#11182C;border:1px solid #1E2A4A;border-radius:6px;}"
            "QFrame#card:hover{border-color:#3B82F6;background:#182142;}"
            "QLabel#cardTitle{color:#F1F5F9;font-family:'Segoe UI Variable Display','Segoe UI';"
                "font-size:16px;font-weight:600;}"
            "QLabel#cardDesc{color:#94A3B8;font-family:'Segoe UI Variable Text','Segoe UI';"
                "font-size:12px;}"
            "QLabel#cardIcon{color:#60A5FA;font-size:34px;}"
            "QLabel#hdr{color:#F1F5F9;font-family:'Segoe UI Variable Display','Segoe UI';"
                "font-size:22px;font-weight:600;}"
        ));
        dlg.setMinimumSize(720, 360);
        auto* col = new QVBoxLayout(&dlg);
        col->setContentsMargins(28, 22, 28, 22);
        col->setSpacing(18);
        auto* hdr = new QLabel(QStringLiteral("Start a new session"), &dlg);
        hdr->setObjectName(QStringLiteral("hdr"));
        col->addWidget(hdr);
        auto* sub = new QLabel(QStringLiteral("Pick a source. You can start more after this — up to 8 at once."), &dlg);
        sub->setStyleSheet(QStringLiteral("color:#94A3B8;font-size:12px;"));
        col->addWidget(sub);
        auto* grid = new QHBoxLayout();
        grid->setSpacing(14);
        col->addLayout(grid, 1);

        auto makeCard = [&](const QString& icon, const QString& title, const QString& desc, QAction* action) {
            auto* card = new QFrame(&dlg);
            card->setObjectName(QStringLiteral("card"));
            card->setCursor(Qt::PointingHandCursor);
            card->setMinimumSize(160, 200);
            auto* cv = new QVBoxLayout(card);
            cv->setContentsMargins(18, 22, 18, 22);
            cv->setSpacing(10);
            auto* iconL = new QLabel(icon, card);
            iconL->setObjectName(QStringLiteral("cardIcon"));
            iconL->setAlignment(Qt::AlignCenter);
            cv->addWidget(iconL);
            auto* titleL = new QLabel(title, card);
            titleL->setObjectName(QStringLiteral("cardTitle"));
            titleL->setAlignment(Qt::AlignCenter);
            cv->addWidget(titleL);
            auto* descL = new QLabel(desc, card);
            descL->setObjectName(QStringLiteral("cardDesc"));
            descL->setWordWrap(true);
            descL->setAlignment(Qt::AlignCenter);
            cv->addWidget(descL, 1);
            // Click-to-trigger via event filter — QFrame doesn't have click signal.
            card->installEventFilter(&dlg);
            QObject::connect(&dlg, &QObject::destroyed, action, [](){});  // no-op tie
            // Use a captured raw pointer dance: associate the action with the card via dynamic property.
            card->setProperty("triggerAction", QVariant::fromValue<void*>(action));
            grid->addWidget(card, 1);
        };
        // Custom event filter to handle clicks on cards and dispatch.
        struct CardClickFilter : public QObject {
            QDialog* dlg;
            CardClickFilter(QDialog* d) : QObject(d), dlg(d) {}
            bool eventFilter(QObject* o, QEvent* e) override {
                if (e->type() == QEvent::MouseButtonRelease) {
                    auto* card = qobject_cast<QFrame*>(o);
                    if (card) {
                        auto* a = static_cast<QAction*>(card->property("triggerAction").value<void*>());
                        if (a) {
                            dlg->accept();
                            QMetaObject::invokeMethod(a, "trigger", Qt::QueuedConnection);
                        }
                    }
                }
                return QObject::eventFilter(o, e);
            }
        };
        auto* filter = new CardClickFilter(&dlg);
        // Re-install filter on each card after creation.
        // (Cards already have installEventFilter(&dlg) — replace with our filter.)
        // Simpler: walk the children after building.
        makeCard(QStringLiteral("📺"), QStringLiteral("Capture Card"),
                 QStringLiteral("HDMI in from any console — best for CV scripts."), aCap);
        makeCard(QStringLiteral("🔵"), QStringLiteral("PS Remote Play"),
                 QStringLiteral("Stream from your PS5 over the network."), aPs);
        makeCard(QStringLiteral("🟢"), QStringLiteral("Xbox Remote Play"),
                 QStringLiteral("Stream from your Xbox console."), aXboxR);
        makeCard(QStringLiteral("☁️"), QStringLiteral("xCloud"),
                 QStringLiteral("Browser-based, sign in directly on xbox.com/play."), aCloud);
        // Re-install our filter on the cards (replacing the placeholder).
        for (auto* card : dlg.findChildren<QFrame*>(QStringLiteral("card"))) {
            card->removeEventFilter(&dlg);
            card->installEventFilter(filter);
        }
        dlg.exec();
    });

    // Keep m_btnPair as a hidden no-op so existing code paths that reference
    // it stay valid until the next refactor pass.
    m_btnPair = new QPushButton(QString(), bar);
    m_btnPair->setVisible(false);

    auto* btnTheme = new QPushButton(QStringLiteral("theme…"),  bar);
    btnTheme ->setProperty("ghost",  true);

    m_btnStart = new QPushButton(QStringLiteral("START ENGINE"), bar);
    m_btnStop  = new QPushButton(QStringLiteral("STOP"),         bar);
    m_btnStart->setProperty("accent", true);
    m_btnStop ->setProperty("danger", true);
    m_btnStop->setVisible(false);  // only show when running

    // Performance-tier segmented toggle. Persisted as cv/perfMode and applied
    // to BOTH the streaming session (ps/fps + ps/bitrate + ps/codec) and the
    // python script (--low-end).
    m_perfLiteBtn = new QPushButton(QStringLiteral("Low End"),  bar);
    m_perfProBtn  = new QPushButton(QStringLiteral("High End"), bar);
    m_perfLiteBtn->setCheckable(true); m_perfProBtn->setCheckable(true);
    m_perfLiteBtn->setProperty("segLeft",  true);
    m_perfProBtn ->setProperty("segRight", true);
    m_perfLiteBtn->setMinimumHeight(34); m_perfProBtn->setMinimumHeight(34);
    {
        const QString saved = m_settings ? m_settings->value(
            QStringLiteral("cv/perfMode"), QStringLiteral("high")).toString()
            : QStringLiteral("high");
        const bool low = (saved.toLower() == QStringLiteral("low") || saved.toLower() == QStringLiteral("lite"));
        m_perfLiteBtn->setChecked(low);
        m_perfProBtn ->setChecked(!low);
    }
    connect(m_perfLiteBtn, &QPushButton::clicked, this, [this]() {
        m_perfLiteBtn->setChecked(true); m_perfProBtn->setChecked(false);
        if (m_settings) { m_settings->setValue(QStringLiteral("cv/perfMode"), "low"); m_settings->sync(); }
    });
    connect(m_perfProBtn, &QPushButton::clicked, this, [this]() {
        m_perfProBtn->setChecked(true); m_perfLiteBtn->setChecked(false);
        if (m_settings) { m_settings->setValue(QStringLiteral("cv/perfMode"), "high"); m_settings->sync(); }
    });

    connect(btnTheme, &QPushButton::clicked, this, &LabsMainWindow::onOpenTheme);

    // + new session → menu actions. Each action sets the mode (so applyMode +
    // pair() route correctly) and triggers the corresponding flow.
    auto setModeAndPair = [this](Mode m) {
        if (m_modeBox) m_modeBox->setCurrentIndex(static_cast<int>(m));
        applyMode(m);
        // Persist xbox/mode so XboxStreamPlugin::pair sees the right value.
        if (m_settings) {
            const QString modeStr = (m == Mode::Cloud) ? QStringLiteral("cloud")
                                  : (m == Mode::Xbox)  ? QStringLiteral("home")
                                                       : QString();
            if (!modeStr.isEmpty()) {
                m_settings->setValue(QStringLiteral("xbox/mode"), modeStr);
                m_settings->sync();
            }
        }
        const QString pluginName =
              m == Mode::PS    ? QStringLiteral("PS Remote Play")
            : m == Mode::Cloud ? QStringLiteral("Xbox Web")     // visible WebView2 → xbox.com/play
            : m == Mode::Xbox  ? QStringLiteral("Xbox Stream")  // headless Greenlight Remote Play
                               : QString();
        if (pluginName.isEmpty()) return;
        IPlugin* p = m_pluginsByName.value(pluginName, nullptr);
        if (auto* pr = dynamic_cast<IPairablePlugin*>(p)) {
            pr->pair(this);
        }
    };

    connect(aCap, &QAction::triggered, this, [this]() {
        IPlugin* p = m_pluginsByName.value(QStringLiteral("Capture Card"), nullptr);
        if (!p) { appendLog(QStringLiteral("Capture Card plugin not loaded")); return; }
        auto* pr = dynamic_cast<IPairablePlugin*>(p);
        if (!pr || !pr->pair(this)) return;

        stopActiveSource();
        if (IPlugin* wgc = m_pluginsByName.value(QStringLiteral("WGC Capture"), nullptr)) {
            QMetaObject::invokeMethod(wgc->qobject(), "stopCapture", Qt::DirectConnection);
        }
        IFrameSource* src = m_frameSources.value(QStringLiteral("Capture Card"), nullptr);
        if (!src) { appendLog(QStringLiteral("Capture Card source missing")); return; }
        src->setSink(m_fanOut.get());
        m_activeSource = src;
        if (src->start()) {
            m_lastFrameCount = src->frameCount();
            m_fpsTimer->start();
            if (m_targetLabel) m_targetLabel->setText(src->targetLabel());
            m_scriptStatus->setText(QStringLiteral("running"));
            appendLog(QStringLiteral("Capture card started — %1").arg(src->targetLabel()));
            updateActions();
        } else {
            appendLog(QStringLiteral("Capture card start failed"));
        }
    });
    connect(aPs,    &QAction::triggered, this, [setModeAndPair]() { setModeAndPair(Mode::PS); });
    connect(aXboxR, &QAction::triggered, this, [setModeAndPair]() { setModeAndPair(Mode::Xbox); });
    connect(aCloud, &QAction::triggered, this, [setModeAndPair]() { setModeAndPair(Mode::Cloud); });

    connect(m_btnStart, &QPushButton::clicked, this, &LabsMainWindow::onStart);
    connect(m_btnStop,  &QPushButton::clicked, this, &LabsMainWindow::onStop);

    auto* row = new QHBoxLayout(bar);
    row->setContentsMargins(20, 0, 18, 0);
    row->setSpacing(10);
    row->addWidget(logo);
    row->addSpacing(8);
    row->addLayout(titleColumn);
    row->addSpacing(18);
    row->addWidget(m_statePill);
    row->addStretch();
    row->addWidget(modeLabel);
    row->addWidget(m_modeBox);
    row->addWidget(btnNewSession);
    row->addWidget(btnTheme);
    row->addSpacing(8);
    // perf segmented control sits right before Start/Stop so it reads as the choice
    auto* perfWrap = new QHBoxLayout();
    perfWrap->setSpacing(0);
    perfWrap->addWidget(m_perfLiteBtn);
    perfWrap->addWidget(m_perfProBtn);
    row->addLayout(perfWrap);
    row->addSpacing(6);
    row->addWidget(m_btnStart);
    row->addWidget(m_btnStop);

    return bar;
}

// ── left rail (scripts) ─────────────────────────────────────────────────────

QWidget* LabsMainWindow::buildScriptsRail()
{
    auto* rail = new QWidget(this);
    rail->setObjectName(QStringLiteral("leftRail"));
    rail->setFixedWidth(270);

    auto* col = new QVBoxLayout(rail);
    col->setContentsMargins(14, 16, 14, 16);
    col->setSpacing(16);

    // Panel: CV Python
    auto* panel = new QFrame(rail);
    panel->setObjectName(QStringLiteral("panel"));
    auto* panelL = new QVBoxLayout(panel);
    panelL->setContentsMargins(0, 0, 0, 0);
    panelL->setSpacing(0);
    auto* header = new QFrame(panel);
    header->setObjectName(QStringLiteral("panelHeader"));
    auto* headerL = new QHBoxLayout(header);
    headerL->setContentsMargins(12, 8, 12, 8);
    headerL->addWidget(sectionLabel(QStringLiteral("CV Python"), header));
    auto* content = new QWidget(panel);
    auto* contentL = new QVBoxLayout(content);
    contentL->setContentsMargins(12, 12, 12, 12);
    contentL->setSpacing(8);
    panelL->addWidget(header);
    panelL->addWidget(content);
    col->addWidget(panel);

    auto* eb = eyebrowLabel(QStringLiteral("active script"), content);
    contentL->addWidget(eb);
    contentL->addSpacing(4);

    m_scriptCombo = new QComboBox(rail);
    m_scriptCombo->setEditable(false);
    m_scriptCombo->setMinimumHeight(30);
    {
        // Seed the user folder with bundled scripts on first run, then auto-scan.
        // Users can drop their own .py here. Future "download" feature lands here too.
        seedDefaultScripts();
        const QString scriptsDir = userScriptsDir();
        QStringList found;
        for (const QFileInfo& fi : QDir(scriptsDir).entryInfoList(
                 QStringList{QStringLiteral("*.py")}, QDir::Files, QDir::Name)) {
            if (fi.fileName().startsWith(QChar('_'))) continue;            // skip __init__ etc
            if (fi.completeBaseName().endsWith(QStringLiteral("_test"))) continue;
            found << fi.absoluteFilePath();
        }

        // Saved path (may be a custom one outside the user dir — keep it).
        QString saved = m_settings ? m_settings->value(QStringLiteral("cv/scriptPath")).toString() : QString();
        if (!saved.isEmpty() && QFileInfo::exists(saved) && !found.contains(saved)) {
            found.prepend(saved);
        }

        for (const QString& path : found) {
            m_scriptCombo->addItem(QFileInfo(path).fileName(), path);
        }

        int idx = -1;
        if (!saved.isEmpty()) idx = m_scriptCombo->findData(saved);
        if (idx < 0 && m_scriptCombo->count() > 0) idx = 0;
        if (idx >= 0) m_scriptCombo->setCurrentIndex(idx);
    }
    connect(m_scriptCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateActions(); });
    contentL->addWidget(m_scriptCombo);

    contentL->addSpacing(8);
    auto* scriptToolRow = new QHBoxLayout();
    scriptToolRow->setSpacing(6);
    m_scriptBrowseBtn = new QPushButton(QStringLiteral("browse…"), content);
    m_scriptBrowseBtn->setProperty("ghost", true);
    auto* openFolderBtn = new QPushButton(QStringLiteral("open folder"), content);
    openFolderBtn->setProperty("ghost", true);
    auto* refreshBtn = new QPushButton(QStringLiteral("refresh"), content);
    refreshBtn->setProperty("ghost", true);
    refreshBtn->setToolTip(QStringLiteral("Re-scan scripts folder for new .py files"));
    connect(m_scriptBrowseBtn, &QPushButton::clicked, this, &LabsMainWindow::onBrowseScript);
    connect(openFolderBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(userScriptsDir()));
    });
    connect(refreshBtn, &QPushButton::clicked, this, &LabsMainWindow::onRefreshScripts);
    scriptToolRow->addWidget(m_scriptBrowseBtn);
    scriptToolRow->addWidget(openFolderBtn);
    scriptToolRow->addWidget(refreshBtn);
    contentL->addLayout(scriptToolRow);

    contentL->addSpacing(14);
    contentL->addWidget(eyebrowLabel(QStringLiteral("Control"), content));
    contentL->addSpacing(4);
    m_scriptRunBtn  = new QPushButton(QStringLiteral("Start"), content);
    m_scriptStopBtn = new QPushButton(QStringLiteral("Restart"), content);
    m_scriptStopBtn->setEnabled(false);
    m_scriptRunBtn ->setProperty("accent", true);
    m_scriptStopBtn->setProperty("ghost", true); // Restart button isn't primary danger in screenshot
    connect(m_scriptRunBtn,  &QPushButton::clicked, this, &LabsMainWindow::onRunScript);
    connect(m_scriptStopBtn, &QPushButton::clicked, this, &LabsMainWindow::onStopScript);
    
    auto* ctrlRow = new QHBoxLayout();
    ctrlRow->addWidget(m_scriptRunBtn);
    ctrlRow->addWidget(m_scriptStopBtn);
    contentL->addLayout(ctrlRow);

    contentL->addSpacing(20);
    contentL->addWidget(eyebrowLabel(QStringLiteral("status"), content));
    contentL->addSpacing(4);
    m_scriptStatus = new QLabel(QStringLiteral("idle"), content);
    m_scriptStatus->setObjectName(QStringLiteral("statusMono"));
    contentL->addWidget(m_scriptStatus);

    col->addStretch();
    col->addWidget(hSeparator(rail));
    col->addSpacing(10);
    col->addWidget(eyebrowLabel(QStringLiteral("settings ini"), rail));
    col->addSpacing(4);
    auto* settingsPath = new QLabel(m_settings ? m_settings->filePath() : QString(), rail);
    settingsPath->setObjectName(QStringLiteral("pathBlue"));
    settingsPath->setWordWrap(true);
    col->addWidget(settingsPath);

    return rail;
}

// ── center stage ────────────────────────────────────────────────────────────

QWidget* LabsMainWindow::buildCenterStage()
{
    auto* stage = new QWidget(this);
    stage->setObjectName(QStringLiteral("stage"));

    // Tab strip.
    auto* tabs = new QWidget(stage);
    tabs->setObjectName(QStringLiteral("stageTabs"));
    tabs->setFixedHeight(38);

    m_tabVideo = new QLabel(QStringLiteral("video display"), tabs);
    m_tabVideo->setObjectName(QStringLiteral("tabActive"));
    m_tabVideo->setCursor(Qt::PointingHandCursor);
    m_tabVideo->installEventFilter(this);

    m_tabMonitor = new QLabel(QStringLiteral("controller monitor"), tabs);
    m_tabMonitor->setObjectName(QStringLiteral("tabInactive"));
    m_tabMonitor->setCursor(Qt::PointingHandCursor);
    m_tabMonitor->installEventFilter(this);

    // Marketplace tab — same widget the launcher embeds, third tab so users
    // can install scripts without bouncing back to the hub. Both surfaces
    // read/write the same canonical %LocalAppData%\Labs\scripts folder, so
    // anything installed here shows up in the launcher immediately.
    m_tabMarket = new QLabel(QStringLiteral("marketplace"), tabs);
    m_tabMarket->setObjectName(QStringLiteral("tabInactive"));
    m_tabMarket->setCursor(Qt::PointingHandCursor);
    m_tabMarket->installEventFilter(this);

    m_targetLabel = new QLabel(QStringLiteral(""), tabs);
    m_targetLabel->setObjectName(QStringLiteral("targetText"));

    m_fpsLabel = new QLabel(QStringLiteral("— fps"), tabs);
    m_fpsLabel->setObjectName(QStringLiteral("fpsPill"));

    auto* tabsRow = new QHBoxLayout(tabs);
    tabsRow->setContentsMargins(0, 0, 10, 0);
    tabsRow->setSpacing(0);
    tabsRow->addWidget(m_tabVideo);
    tabsRow->addWidget(m_tabMonitor);
    tabsRow->addWidget(m_tabMarket);
    tabsRow->addStretch();
    tabsRow->addWidget(m_targetLabel);
    tabsRow->addWidget(m_fpsLabel);

    m_stagePages = new QStackedWidget(stage);
    m_stagePages->setObjectName(QStringLiteral("stage"));

    // Page 0: video display — IUIPlugin widget mounts here later.
    m_stageHost = new QWidget(m_stagePages);
    m_stageHost->setObjectName(QStringLiteral("stage"));
    auto* hostLayout = new QVBoxLayout(m_stageHost);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    m_stagePages->addWidget(m_stageHost);

    // Page 1: controller monitor.
    m_monitor = new ControllerMonitorWidget(m_stagePages);
    m_stagePages->addWidget(m_monitor);

    // Page 2: marketplace. Wrapped in an opaque white container so the
    // engine's theme background (LabsBackgroundWidget) doesn't bleed
    // through. Same shape the launcher uses to embed this widget; here
    // we apply it inside the stack instead of next to a sidebar.
    auto* marketWrap = new QWidget(m_stagePages);
    marketWrap->setObjectName(QStringLiteral("marketplaceWrap"));
    marketWrap->setAutoFillBackground(true);
    marketWrap->setStyleSheet(QStringLiteral(
        "QWidget#marketplaceWrap { background: #FFFFFF; }"
    ));
    auto* marketWrapL = new QVBoxLayout(marketWrap);
    marketWrapL->setContentsMargins(0, 0, 0, 0);
    marketWrapL->setSpacing(0);
    auto* market = new MarketplaceWidget(m_settings.get(), marketWrap);
    marketWrapL->addWidget(market, 1);
    m_stagePages->addWidget(marketWrap);
    connect(market, &MarketplaceWidget::scriptListChanged, this, [this]() {
        onRefreshScripts();
    });
    connect(market, &MarketplaceWidget::runRequested, this, [this](const QString& path) {
        if (m_scriptCombo) {
            int idx = m_scriptCombo->findData(path);
            if (idx < 0) {
                m_scriptCombo->addItem(QFileInfo(path).fileName(), path);
                idx = m_scriptCombo->count() - 1;
            }
            m_scriptCombo->setCurrentIndex(idx);
        }
        onRunScript();
    });

    auto* col = new QVBoxLayout(stage);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);
    col->addWidget(tabs);
    col->addWidget(m_stagePages, 1);

    return stage;
}

bool LabsMainWindow::eventFilter(QObject* obj, QEvent* ev)
{
    auto setActive = [this](QLabel* active) {
        for (QLabel* t : { m_tabVideo, m_tabMonitor, m_tabMarket }) {
            if (!t) continue;
            t->setObjectName(t == active ? QStringLiteral("tabActive")
                                         : QStringLiteral("tabInactive"));
            t->style()->polish(t);
        }
    };
    if (ev->type() == QEvent::MouseButtonRelease) {
        if (obj == m_tabVideo && m_stagePages) {
            m_stagePages->setCurrentIndex(0);
            setActive(m_tabVideo);
            return true;
        }
        if (obj == m_tabMonitor && m_stagePages) {
            m_stagePages->setCurrentIndex(1);
            setActive(m_tabMonitor);
            return true;
        }
        if (obj == m_tabMarket && m_stagePages) {
            m_stagePages->setCurrentIndex(2);
            setActive(m_tabMarket);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, ev);
}

// ── right rail (devices) ────────────────────────────────────────────────────

// Builds one bridge section (Titan or Cronus) into a parent layout.
// Fills the provided combo/status/restart/slots/output/usb pointers.
static void buildBridgeSection(
        QVBoxLayout* col,
        const QString& bridgeTitle,
        const QString& deviceTitle,
        const QString& scanTip,
        QComboBox*&   outDeviceCombo,
        QLabel*&      outDeviceStatus,
        QPushButton*& outRestartBtn,
        QLabel*       outSlots[3],
        QComboBox*&   outOutputCombo,
        QLabel*       outUsb[4])
{
    QWidget* parent = col->parentWidget();

    col->addWidget(sectionLabel(bridgeTitle, parent));
    col->addSpacing(10);

    // ── Device panel ──────────────────────────────────────────────────────────
    auto* grpDevice = new QFrame(parent);
    grpDevice->setObjectName(QStringLiteral("panel"));
    auto* grpOuter = new QVBoxLayout(grpDevice);
    grpOuter->setContentsMargins(0, 0, 0, 0);
    grpOuter->setSpacing(0);

    auto* hdr = new QFrame(grpDevice);
    hdr->setObjectName(QStringLiteral("panelHeader"));
    auto* hdrL = new QHBoxLayout(hdr);
    hdrL->setContentsMargins(12, 8, 12, 8);
    hdrL->addWidget(sectionLabel(deviceTitle, hdr));
    grpOuter->addWidget(hdr);

    auto* body = new QWidget(grpDevice);
    auto* bodyL = new QVBoxLayout(body);
    bodyL->setSpacing(6);
    bodyL->setContentsMargins(12, 10, 12, 10);
    grpOuter->addWidget(body);

    // combo + refresh
    auto* devRow = new QHBoxLayout;
    outDeviceCombo = new QComboBox(body);
    outDeviceCombo->addItem(QStringLiteral("No devices found"));
    outDeviceCombo->setEnabled(false);
    auto* scanBtn = new QPushButton(body);
    scanBtn->setIcon(QIcon(QStringLiteral(":/labs/icon_refresh.svg")));
    scanBtn->setIconSize(QSize(13, 13));
    scanBtn->setFixedSize(26, 26);
    scanBtn->setToolTip(scanTip);
    devRow->addWidget(outDeviceCombo, 1);
    devRow->addWidget(scanBtn);
    bodyL->addLayout(devRow);

    outDeviceStatus = new QLabel(QStringLiteral("No device selected"), body);
    outDeviceStatus->setObjectName(QStringLiteral("titanStatus"));
    bodyL->addWidget(outDeviceStatus);

    auto* restartRow = new QHBoxLayout;
    outRestartBtn = new QPushButton(QStringLiteral("Restart Bridge"), body);
    outRestartBtn->setEnabled(false);
    auto* indicator = new QFrame(body);
    indicator->setObjectName(QStringLiteral("titanIndicator"));
    indicator->setFixedSize(22, 22);
    restartRow->addWidget(outRestartBtn, 1);
    restartRow->addWidget(indicator);
    bodyL->addLayout(restartRow);
    col->addWidget(grpDevice);
    col->addSpacing(8);

    // ── Memory Slots ──────────────────────────────────────────────────────────
    auto* grpSlots = new QGroupBox(QStringLiteral("Memory Slots"), parent);
    auto* slotsL = new QVBoxLayout(grpSlots);
    slotsL->setSpacing(4);
    slotsL->setContentsMargins(8, 10, 8, 10);

    for (int i = 0; i < 3; ++i) {
        auto* slotFrame = new QFrame(grpSlots);
        slotFrame->setObjectName(i == 0 ? QStringLiteral("titanSlotActive")
                                        : QStringLiteral("titanSlot"));
        auto* slotRow = new QHBoxLayout(slotFrame);
        slotRow->setContentsMargins(10, 5, 10, 5);

        auto* num = new QLabel(QString::number(i + 1), slotFrame);
        num->setObjectName(i == 0 ? QStringLiteral("titanSlotNumActive")
                                  : QStringLiteral("titanSlotNum"));
        num->setFixedWidth(18);

        outSlots[i] = new QLabel(QStringLiteral("Empty"), slotFrame);
        outSlots[i]->setObjectName(i == 0 ? QStringLiteral("titanSlotLabelActive")
                                           : QStringLiteral("titanSlotLabel"));
        slotRow->addWidget(num);
        slotRow->addWidget(outSlots[i], 1);
        slotsL->addWidget(slotFrame);
    }
    col->addWidget(grpSlots);
    col->addSpacing(8);

    // ── Tabs ──────────────────────────────────────────────────────────────────
    auto* tabs = new QTabWidget(parent);
    tabs->setDocumentMode(true);

    auto* cfgTab = new QWidget;
    auto* cfgL   = new QVBoxLayout(cfgTab);
    cfgL->setContentsMargins(0, 8, 0, 0);
    cfgL->setSpacing(6);

    auto* outRow = new QHBoxLayout;
    outRow->addWidget(new QLabel(QStringLiteral("Output:"), cfgTab));
    outOutputCombo = new QComboBox(cfgTab);
    outOutputCombo->addItem(QStringLiteral("Automatic"));
    outRow->addWidget(outOutputCombo, 1);
    cfgL->addLayout(outRow);

    auto* grpUsb = new QGroupBox(QStringLiteral("USB Ports"), cfgTab);
    auto* usbL   = new QVBoxLayout(grpUsb);
    usbL->setSpacing(3);
    usbL->setContentsMargins(8, 8, 8, 8);

    const char* ports[] = { "IN A", "IN B", "OUT", "PROG" };
    for (int i = 0; i < 4; ++i) {
        auto* row = new QHBoxLayout;
        auto* lbl = new QLabel(QString::fromLatin1(ports[i]), grpUsb);
        lbl->setObjectName(QStringLiteral("titanSlotNum")); // reuse dim style
        lbl->setFixedWidth(38);
        outUsb[i] = new QLabel(QStringLiteral("Empty"), grpUsb);
        outUsb[i]->setObjectName(QStringLiteral("titanSlotLabel")); // reuse dim style
        row->addWidget(lbl);
        row->addWidget(outUsb[i], 1);
        usbL->addLayout(row);
    }
    cfgL->addWidget(grpUsb);
    cfgL->addStretch();
    tabs->addTab(cfgTab, QStringLiteral("Device Config"));

    auto* kmgTab = new QWidget;
    auto* kmgL   = new QVBoxLayout(kmgTab);
    auto* kmgLbl = new QLabel(QStringLiteral("KMG Capture\nnot connected"), kmgTab);
    kmgLbl->setAlignment(Qt::AlignCenter);
    kmgLbl->setObjectName(QStringLiteral("titanStatus"));
    kmgL->addWidget(kmgLbl);
    tabs->addTab(kmgTab, QStringLiteral("KMG Capture"));

    col->addWidget(tabs);
    col->addSpacing(16);
}

QWidget* LabsMainWindow::buildDevicesRail()
{
    auto* rail = new QWidget(this);
    rail->setObjectName(QStringLiteral("rightRail"));
    rail->setFixedWidth(240);

    // Scroll area so both bridges fit
    auto* scroll = new QScrollArea(rail);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(true);

    auto* inner = new QWidget;
    auto* col = new QVBoxLayout(inner);
    col->setContentsMargins(14, 16, 14, 16);
    col->setSpacing(0);

    buildBridgeSection(col,
        QStringLiteral("Titan Bridge"),
        QStringLiteral("Titan Two Device"),
        QStringLiteral("Scan for Titan Two"),
        m_titanDeviceCombo, m_titanDeviceStatus, m_titanRestartBtn,
        m_titanSlots, m_titanOutputCombo, m_titanUsbLabels);

    col->addStretch();

    m_devicesList = new QLabel(inner);
    m_devicesList->hide();

    scroll->setWidget(inner);

    auto* railLayout = new QVBoxLayout(rail);
    railLayout->setContentsMargins(0, 0, 0, 0);
    railLayout->addWidget(scroll);

    return rail;
}

void LabsMainWindow::refreshDevicesList()
{
    if (!m_titanDeviceCombo) return;
    // Update Titan device combo with any HID devices found
    // (real Titan Two enumeration goes here — for now reflects plugin state)
}

// ── mode wiring ─────────────────────────────────────────────────────────────

void LabsMainWindow::applyMode(Mode mode)
{
    stopActiveSource();

    // Source plugin per mode:
    //   Xbox Remote Play / xCloud → XboxStreamPlugin (headless Greenlight).
    //   PS Remote Play           → PSRemotePlayPlugin via labs.dll.
    // XboxStreamPlugin reads xbox/mode (set in onPair) to decide streamType.
    const bool xboxFamily = (mode == Mode::Xbox || mode == Mode::Cloud);

    // Xbox family: source/sink come from the focused XboxStreamSession (multi-
    // session factory pattern). dynamic_cast across the DLL boundary works for
    // IFrameSource / IControllerSink because both live in LabsCore.
    // PS family: classic single-source plugin lookup.
    if (xboxFamily) {
        QObject* sessObj = m_focusedXboxSession.data();
        m_activeSource   = sessObj ? dynamic_cast<IFrameSource*>(sessObj)    : nullptr;
        m_activeCtrlSink = sessObj ? dynamic_cast<IControllerSink*>(sessObj) : nullptr;
    } else {
        m_activeSource   = m_frameSources.value(QStringLiteral("PS Remote Play"), nullptr);
        m_activeCtrlSink = m_ctrlSinks.value(QStringLiteral("PS Remote Play"), nullptr);
    }

    // Tell XInput whether to keep emitting an idle baseline when no physical
    // pad is plugged in. Both Xbox modes need this so InputOverride scripts
    // get a 125Hz baseline to patch onto. PS mode goes silent when pad is
    // unplugged, which is correct chiaki behavior.
    if (IPlugin* xi = m_pluginsByName.value(QStringLiteral("XInput"), nullptr)) {
        QMetaObject::invokeMethod(xi->qobject(), "setIdleEmit", Qt::DirectConnection,
                                  Q_ARG(bool, xboxFamily));
    }

    if (m_activeSource) m_activeSource->setSink(m_fanOut.get());

    // Swap the output half of the fan-out; monitor half stays permanently
    // connected so the pad visualization keeps updating regardless of mode.
    s_ctrlFanOut.setOutput(m_activeCtrlSink);

    // Swap the center stage widget. xCloud uses the embedded WebView2 (Xbox
    // Web plugin); other modes use the standard Display widget that consumes
    // the IFrameSink fan-out.
    const QString uiName = (mode == Mode::Cloud)
                           ? QStringLiteral("Xbox Web")
                           : QStringLiteral("Display");

    if (m_stageHost && m_stageHost->layout()) {
        for (int i = 0; i < m_stageHost->layout()->count(); ++i) {
            QWidget* w = m_stageHost->layout()->itemAt(i)
                ? m_stageHost->layout()->itemAt(i)->widget() : nullptr;
            if (!w) continue;
            const bool match = (w->property("uiPluginName").toString() == uiName);
            w->setVisible(match);
        }
    }
    // Make sure the central stage stack is on the video page (m_stageHost)
    // so the WebView2 / Display widget is actually visible.
    if (m_stagePages && m_stageHost) m_stagePages->setCurrentWidget(m_stageHost);

    // ALWAYS skip the ViGEm slot regardless of mode. Two reasons:
    //   1. Xbox mode: prevents XInput from reading back the override we
    //      just wrote (feedback loop that would freeze the pad on the
    //      last override value after expiry).
    //   2. PS mode with an Xbox controller: ViGEm's virtual X360 sits on
    //      a slot already; if we don't skip it, XInput finds the silent
    //      virtual pad first and binds there, leaving the user's real
    //      Xbox controller (on a higher slot) invisible.
    // PS mode with DualSense doesn't care either way — DualSensePlugin
    // reads HID directly, separate from XInput.
    int xiSkipMask = 0;
    int viGemSlot  = -1;
    if (IPlugin* vg = m_pluginsByName.value(QStringLiteral("ViGEm Output"), nullptr)) {
        QMetaObject::invokeMethod(vg->qobject(), "userIndex", Qt::DirectConnection,
                                  Q_RETURN_ARG(int, viGemSlot));
        if (viGemSlot >= 0 && viGemSlot < 4) xiSkipMask = (1 << viGemSlot);
    }
    if (IPlugin* xi = m_pluginsByName.value(QStringLiteral("XInput"), nullptr)) {
        QMetaObject::invokeMethod(xi->qobject(), "setSkipMask", Qt::DirectConnection,
                                  Q_ARG(int, xiSkipMask));
    }
    qInfo() << "applyMode: mode="
            << (mode == Mode::Xbox  ? "Xbox Remote Play"
              : mode == Mode::Cloud ? "xCloud"
                                    : "PS Remote Play")
            << "viGemSlot=" << viGemSlot << "xinputSkipMask=" << xiSkipMask;

    // WGC capture is no longer needed for any streaming mode — XboxStream
    // produces decoded H.264 frames natively, and PS Remote Play's chiaki
    // layer feeds frames directly. Stop any leftover WGC capture from
    // a previous session (e.g. if the user manually launched a window
    // capture).
    if (IPlugin* wgc = m_pluginsByName.value(QStringLiteral("WGC Capture"), nullptr)) {
        QMetaObject::invokeMethod(wgc->qobject(), "stopCapture",
                                  Qt::DirectConnection);
    }

    updateActions();
}

void LabsMainWindow::stopActiveSource()
{
    if (m_activeSource) m_activeSource->stop();
    m_fpsTimer->stop();
    if (m_fpsLabel) m_fpsLabel->setText(QStringLiteral("— fps"));
}

// ── multi-session XboxStream wiring ─────────────────────────────────────────

// XboxStreamPlugin's methods + signals are accessed by NAME via Qt's meta
// system. The engine doesn't link to the plugin DLL, so direct member calls
// would be unresolved externals at link time. String-based connect + invokeMethod
// dispatches at runtime through the loaded plugin's metaobject.

// XboxStreamPlugin's methods + signals are accessed by NAME via Qt's meta
// system. The engine doesn't link to the plugin DLL, so direct member calls
// or qobject_cast to plugin types would be unresolved externals at link time.
// String-based connect + invokeMethod dispatches at runtime through the
// loaded plugin's metaobject. Casting to IFrameSource/IControllerSink uses
// dynamic_cast (those interfaces live in LabsCore which both link to).

void LabsMainWindow::hookXboxStreamSignals()
{
    IPlugin* p = m_pluginsByName.value(QStringLiteral("Xbox Stream"), nullptr);
    if (!p) return;
    QObject* xs = p->qobject();
    if (!xs) return;
    connect(xs, SIGNAL(sessionAdded(int)),
            this, SLOT(onXboxSessionAdded(int)));
    connect(xs, SIGNAL(sessionRemoved(int)),
            this, SLOT(onXboxSessionRemoved(int)));
    // The HWND-ready signal fires per-session — connect once globally to the
    // plugin, but the actual signal lives on each XboxStreamSession. Defer
    // the per-session hookup to onXboxSessionAdded.
}

void LabsMainWindow::hookXboxWebSignals()
{
    IPlugin* p = m_pluginsByName.value(QStringLiteral("Xbox Web"), nullptr);
    if (!p) return;
    QObject* xw = p->qobject();
    if (!xw) return;
    connect(xw, SIGNAL(runScriptRequested(int)),  this, SLOT(onXboxWebRunRequested(int)));
    connect(xw, SIGNAL(stopScriptRequested(int)), this, SLOT(onXboxWebStopRequested(int)));
    connect(xw, SIGNAL(sessionRemoved(int)),      this, SLOT(onXboxWebSessionRemoved(int)));
}

void LabsMainWindow::onXboxWebRunRequested(int sessionId)
{
    spawnXboxSessionScript(sessionId);
}

void LabsMainWindow::onXboxWebStopRequested(int sessionId)
{
    killXboxSessionScript(sessionId);
}

void LabsMainWindow::onXboxWebSessionRemoved(int sessionId)
{
    killXboxSessionScript(sessionId);
}

void LabsMainWindow::spawnXboxSessionScript(int sessionId)
{
    if (m_xboxScriptProcs.contains(sessionId)) return;
    if (!m_scriptCombo) return;
    const QString scriptPath = m_scriptCombo->currentData().toString();
    if (scriptPath.isEmpty() || !QFileInfo::exists(scriptPath)) {
        appendLog(QStringLiteral("xCloud session #%1 — no script selected").arg(sessionId));
        return;
    }
    auto* proc = new QProcess(this);
    proc->setProgram(QStringLiteral("python"));
    QStringList args;
    args << scriptPath
         << QStringLiteral("--labs-pid")   << QString::number(QCoreApplication::applicationPid())
         << QStringLiteral("--session-id") << QString::number(sessionId)
         << QStringLiteral("--session")    << QString::number(sessionId);
    if (m_perfLiteBtn && m_perfLiteBtn->isChecked()) args << QStringLiteral("--low-end");
    proc->setArguments(args);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, sessionId, proc]() {
        const QByteArray bytes = proc->readAllStandardOutput();
        for (const QByteArray& line : bytes.split('\n')) {
            const QString l = QString::fromUtf8(line).trimmed();
            if (!l.isEmpty()) appendLog(QStringLiteral("[s%1] %2").arg(sessionId).arg(l));
        }
    });
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, sessionId](int code, QProcess::ExitStatus) {
        appendLog(QStringLiteral("xCloud session #%1 script exited (%2)").arg(sessionId).arg(code));
        QProcess* p = m_xboxScriptProcs.take(sessionId);
        if (p) p->deleteLater();
        IPlugin* plg = m_pluginsByName.value(QStringLiteral("Xbox Web"), nullptr);
        if (plg && plg->qobject()) {
            QMetaObject::invokeMethod(plg->qobject(), "setSessionScriptRunning",
                                      Qt::DirectConnection,
                                      Q_ARG(int, sessionId), Q_ARG(bool, false));
        }
    });
    proc->start();
    if (!proc->waitForStarted(2000)) {
        appendLog(QStringLiteral("xCloud session #%1 — script spawn failed: %2")
                  .arg(sessionId).arg(proc->errorString()));
        proc->deleteLater();
        return;
    }
    m_xboxScriptProcs.insert(sessionId, proc);
    appendLog(QStringLiteral("xCloud session #%1 — running %2").arg(sessionId)
              .arg(QFileInfo(scriptPath).fileName()));
    IPlugin* plg = m_pluginsByName.value(QStringLiteral("Xbox Web"), nullptr);
    if (plg && plg->qobject()) {
        QMetaObject::invokeMethod(plg->qobject(), "setSessionScriptRunning",
                                  Qt::DirectConnection,
                                  Q_ARG(int, sessionId), Q_ARG(bool, true));
    }
}

void LabsMainWindow::killXboxSessionScript(int sessionId)
{
    QProcess* proc = m_xboxScriptProcs.value(sessionId, nullptr);
    if (!proc) return;
    if (proc->state() != QProcess::NotRunning) {
        proc->terminate();
        if (!proc->waitForFinished(1500)) proc->kill();
    }
    // The finished() handler removes from m_xboxScriptProcs and updates UI.
}

void LabsMainWindow::buildXboxContainer()
{
    if (m_xboxContainer || !m_stagePages) return;

    m_xboxContainer = new QWidget(m_stagePages);
    m_xboxContainer->setObjectName(QStringLiteral("xboxContainer"));
    m_xboxContainer->setStyleSheet(QStringLiteral("background:#070A14;"));
    auto* col = new QVBoxLayout(m_xboxContainer);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    // Thin tab strip at top: one button per session, click to switch.
    m_xboxTabStrip = new QWidget(m_xboxContainer);
    m_xboxTabStrip->setObjectName(QStringLiteral("xboxTabStrip"));
    m_xboxTabStrip->setFixedHeight(34);
    m_xboxTabStrip->setStyleSheet(QStringLiteral("background:#0B1020; border-bottom:1px solid #1E2A4A;"));
    auto* tabRow = new QHBoxLayout(m_xboxTabStrip);
    tabRow->setContentsMargins(10, 4, 10, 4);
    tabRow->setSpacing(6);
    tabRow->addStretch();
    col->addWidget(m_xboxTabStrip);

    m_xboxStack = new QStackedWidget(m_xboxContainer);
    m_xboxStack->setObjectName(QStringLiteral("xboxStack"));
    col->addWidget(m_xboxStack, 1);

    // Add as its own page in the central stage stack so it gets the full
    // stage area when active. Switching between video / monitor / xbox is
    // handled in showXboxContainer.
    m_stagePages->addWidget(m_xboxContainer);
}

namespace {
// Re-parent a top-level Win32 window into a Qt widget. Strips the OS frame
// so it behaves like a child control, then SetParent + position to fill.
void reparentChildHwnd(HWND child, HWND parent, int w, int h)
{
    if (!child || !parent) return;
    LONG style = GetWindowLong(child, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    style |= WS_CHILD | WS_VISIBLE;
    SetWindowLong(child, GWL_STYLE, style);
    SetParent(child, parent);
    SetWindowPos(child, nullptr, 0, 0, w, h,
                 SWP_FRAMECHANGED | SWP_NOZORDER | SWP_SHOWWINDOW);
}

// QWidget that owns a reparented HWND and resizes it to fill on every layout.
class EmbeddedHwndHost : public QWidget {
public:
    explicit EmbeddedHwndHost(quintptr hwnd, QWidget* parent = nullptr)
        : QWidget(parent), m_child(reinterpret_cast<HWND>(hwnd)) {
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_DontCreateNativeAncestors);
        setMinimumSize(640, 360);
    }
    void showEvent(QShowEvent*) override {
        if (m_child && !m_attached) {
            reparentChildHwnd(m_child, reinterpret_cast<HWND>(winId()), width(), height());
            m_attached = true;
        }
    }
    void resizeEvent(QResizeEvent* e) override {
        if (m_child && m_attached) {
            SetWindowPos(m_child, nullptr, 0, 0, e->size().width(), e->size().height(),
                         SWP_NOZORDER);
        }
    }
private:
    HWND m_child = nullptr;
    bool m_attached = false;
};
} // namespace

void LabsMainWindow::embedXboxHwnd(int sessionId, quintptr hwnd, const QString& label)
{
    buildXboxContainer();
    if (!m_xboxStack) return;

    // Replace existing host for this session if it was already wired.
    if (m_xboxHosts.contains(sessionId)) {
        QWidget* old = m_xboxHosts.take(sessionId);
        if (old) {
            m_xboxStack->removeWidget(old);
            old->deleteLater();
        }
    }

    auto* host = new EmbeddedHwndHost(hwnd, m_xboxStack);
    m_xboxStack->addWidget(host);
    m_xboxHosts.insert(sessionId, host);
    m_xboxStack->setCurrentWidget(host);

    // Add a tab button in the strip for this session.
    if (!m_xboxTabs.contains(sessionId) && m_xboxTabStrip) {
        auto* btn = new QPushButton(label.isEmpty() ? QStringLiteral("session %1").arg(sessionId) : label,
                                    m_xboxTabStrip);
        btn->setProperty("ghost", true);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, sessionId]() {
            QWidget* w = m_xboxHosts.value(sessionId, nullptr);
            if (w) m_xboxStack->setCurrentWidget(w);
        });
        // Insert before the trailing stretch (last item).
        auto* row = qobject_cast<QHBoxLayout*>(m_xboxTabStrip->layout());
        if (row) row->insertWidget(row->count() - 1, btn);
        m_xboxTabs.insert(sessionId, btn);
    }

    showXboxContainer(true);
}

void LabsMainWindow::removeXboxHost(int sessionId)
{
    if (m_xboxStack && m_xboxHosts.contains(sessionId)) {
        QWidget* w = m_xboxHosts.take(sessionId);
        if (w) {
            m_xboxStack->removeWidget(w);
            w->deleteLater();
        }
    }
    if (m_xboxTabs.contains(sessionId)) {
        QPushButton* btn = m_xboxTabs.take(sessionId);
        if (btn) btn->deleteLater();
    }
    if (m_xboxStack && m_xboxStack->count() == 0) {
        showXboxContainer(false);
    }
}

void LabsMainWindow::showXboxContainer(bool show)
{
    if (!m_xboxContainer || !m_stagePages) return;
    if (show) {
        m_stagePages->setCurrentWidget(m_xboxContainer);
    } else {
        // Default back to the video display stage.
        if (m_stageHost) m_stagePages->setCurrentWidget(m_stageHost);
    }
}

void LabsMainWindow::onXboxWindowHwndReady(int sessionId, quintptr hwnd)
{
    if (!hwnd) return;
    QString label;
    // Look up the session's account label for the tab text.
    IPlugin* p = m_pluginsByName.value(QStringLiteral("Xbox Stream"), nullptr);
    if (p) {
        QObject* xs = p->qobject();
        QObject* sessObj = nullptr;
        if (xs) {
            QMetaObject::invokeMethod(xs, "sessionByIdObject", Qt::DirectConnection,
                                      Q_RETURN_ARG(QObject*, sessObj),
                                      Q_ARG(int, sessionId));
            if (auto* fs = dynamic_cast<IFrameSource*>(sessObj)) label = fs->targetLabel();
        }
    }
    embedXboxHwnd(sessionId, hwnd, label);
    appendLog(QStringLiteral("xCloud session #%1 embedded").arg(sessionId));
}

void LabsMainWindow::onXboxSessionAdded(int id)
{
    IPlugin* p = m_pluginsByName.value(QStringLiteral("Xbox Stream"), nullptr);
    if (!p) return;
    QObject* xs = p->qobject();
    if (!xs) return;
    QObject* sessObj = nullptr;
    QMetaObject::invokeMethod(xs, "sessionByIdObject", Qt::DirectConnection,
                              Q_RETURN_ARG(QObject*, sessObj),
                              Q_ARG(int, id));
    if (!sessObj) return;
    QString label;
    auto* fs = dynamic_cast<IFrameSource*>(sessObj);
    if (fs) label = fs->targetLabel();
    appendLog(QStringLiteral("xbox session #%1 added (%2)").arg(id).arg(label));
    // Per-session connection: when the cloud sidecar reports its window HWND,
    // embed it. String-form connect because we don't link the plugin DLL.
    connect(sessObj, SIGNAL(windowHwndReady(int,quintptr)),
            this, SLOT(onXboxWindowHwndReady(int,quintptr)));
    // If the session already has an HWND (e.g. fast loadURL completed before
    // we wired up the signal), fetch it now and embed.
    quintptr existing = 0;
    QMetaObject::invokeMethod(sessObj, "windowHwnd", Qt::DirectConnection,
                              Q_RETURN_ARG(quintptr, existing));
    if (existing) onXboxWindowHwndReady(id, existing);
    if (!m_focusedXboxSession) focusXboxSession(sessObj);
}

void LabsMainWindow::onXboxSessionRemoved(int id)
{
    appendLog(QStringLiteral("xbox session #%1 removed").arg(id));
    removeXboxHost(id);

    // Tell CvPython to close that session's frame SHM block. Matches the
    // 100..107 namespace stamped by XboxStreamSession::pushFrame.
    if (auto* cv = m_pluginsByName.value(QStringLiteral("CV Python"), nullptr); cv && cv->qobject()) {
        QMetaObject::invokeMethod(cv->qobject(), "closeSession",
                                  Qt::DirectConnection,
                                  Q_ARG(int, 100 + id));
    }
    if (m_focusedXboxSession && m_focusedXboxSessionId == id) {
        m_focusedXboxSession.clear();
        m_focusedXboxSessionId = -1;
        // Try to focus another remaining session if any.
        IPlugin* p = m_pluginsByName.value(QStringLiteral("Xbox Stream"), nullptr);
        if (!p) return;
        QObject* xs = p->qobject();
        if (!xs) return;
        int count = 0;
        QMetaObject::invokeMethod(xs, "sessionCount", Qt::DirectConnection,
                                  Q_RETURN_ARG(int, count));
        if (count > 0) {
            QObject* sessObj = nullptr;
            QMetaObject::invokeMethod(xs, "sessionAtObject", Qt::DirectConnection,
                                      Q_RETURN_ARG(QObject*, sessObj),
                                      Q_ARG(int, 0));
            if (sessObj) focusXboxSession(sessObj);
        }
    }
}

void LabsMainWindow::focusXboxSession(QObject* session)
{
    if (!session) return;
    m_focusedXboxSession = session;
    int sid = -1;
    QMetaObject::invokeMethod(session, "id", Qt::DirectConnection,
                              Q_RETURN_ARG(int, sid));
    m_focusedXboxSessionId = sid;
    if (m_modeBox) {
        const Mode mode = static_cast<Mode>(qBound(0, m_modeBox->currentIndex(), 2));
        if (mode == Mode::Xbox || mode == Mode::Cloud) {
            applyMode(mode);
        }
    }
    appendLog(QStringLiteral("focused xbox session #%1").arg(sid));
}

void LabsMainWindow::onModeChanged(int index)
{
    applyMode(static_cast<Mode>(qBound(0, index, 2)));
    qInfo() << "mode:" << m_modeBox->currentText();
}

// ── actions ─────────────────────────────────────────────────────────────────

void LabsMainWindow::onPair()
{
    const Mode mode = static_cast<Mode>(m_modeBox->currentIndex());

    // Both Xbox modes (Remote Play / xCloud) use the same plugin — only the
    // streamType differs. Persist that so the sidecar picks the right one.
    if (mode == Mode::Xbox || mode == Mode::Cloud) {
        if (m_settings) {
            m_settings->setValue(QStringLiteral("xbox/mode"),
                mode == Mode::Cloud ? QStringLiteral("cloud") : QStringLiteral("home"));
            m_settings->sync();
        }
    }

    const QString name = (mode == Mode::PS)
        ? QStringLiteral("PS Remote Play")
        : QStringLiteral("Xbox Stream");
    IPlugin* p = m_pluginsByName.value(name, nullptr);
    if (auto* pr = dynamic_cast<IPairablePlugin*>(p)) {
        if (pr->pair(this)) appendLog(QStringLiteral("paired"));
    }
}

void LabsMainWindow::onStart()
{
    if (m_settings && m_scriptCombo) {
        m_settings->setValue(QStringLiteral("cv/scriptPath"), m_scriptCombo->currentData().toString());
    }

    // Apply perf-tier settings to the streaming pipeline BEFORE starting the source.
    // The PS Remote Play plugin reads ps/fps, ps/bitrate, ps/codec at start() time.
    // PS5 stream caps at 60fps — that's a console hardware limit, not ours. The
    // BetterCam side that the script captures from CAN do 240fps; that's wired
    // separately via --target-fps when the script launches.
    if (m_settings && m_perfLiteBtn) {
        const bool low = m_perfLiteBtn->isChecked();

        // Default tier-based settings (assume worst-case internet path)
        int  fps     = low ? 30   : 60;
        int  bitrate = low ? 5000 : 15000;
        int  width   = 1280;       // 720p — safe over the internet
        int  height  = 720;
        int  codec   = 0;          // H.264 — fastest decode, lowest latency

        // LAN override — when the PS5 is on the local network, lift the caps:
        // bandwidth is free, latency is the only target. High End jumps to 1080p
        // since the bitrate can support it; Low End stays at 720p so weak GPUs
        // don't choke on the bigger frames.
        const QString host = m_settings->value(QStringLiteral("ps/host")).toString();
        const bool onLan = isLanIp(host);
        if (onLan) {
            fps     = 60;
            if (low) {
                bitrate = 12000;            // 720p sharp
            } else {
                width   = 1920; height = 1080;
                bitrate = 25000;            // 1080p sharp
            }
            appendLog(QStringLiteral("LAN — %1×%2 @ %3fps · %4 Mbps").arg(width).arg(height).arg(fps).arg(bitrate / 1000));
        }

        m_settings->setValue(QStringLiteral("ps/fps"),     fps);
        m_settings->setValue(QStringLiteral("ps/bitrate"), bitrate);
        m_settings->setValue(QStringLiteral("ps/width"),   width);
        m_settings->setValue(QStringLiteral("ps/height"),  height);
        m_settings->setValue(QStringLiteral("ps/codec"),   codec);
        m_settings->sync();
    }

    if (!m_activeSource) { appendLog(QStringLiteral("no source for this mode")); return; }
    if (m_activeSource->start()) {
        m_lastFrameCount = m_activeSource->frameCount();
        m_fpsTimer->start();
        const QString label = m_activeSource->targetLabel();
        if (m_targetLabel) m_targetLabel->setText(label);
        m_scriptStatus->setText(QStringLiteral("running"));
        appendLog(QStringLiteral("Engine started — %1").arg(label.isEmpty() ? QStringLiteral("no target") : label));

        // No WGC fallback wiring — XboxStream and PS Remote Play both produce
        // decoded frames natively through the IFrameSink fan-out.
    } else {
        const Mode mode = static_cast<Mode>(m_modeBox->currentIndex());
        const QString msg =
            mode == Mode::PS    ? QStringLiteral("PS start failed — check [ps] settings (pair first)")
          : mode == Mode::Cloud ? QStringLiteral("xCloud start failed — click pair… and sign in first")
                                : QStringLiteral("Xbox Remote Play start failed — click pair… and sign in first");
        appendLog(msg);
    }
    updateActions();
}

void LabsMainWindow::onStop()
{
    stopActiveSource();
    // Stop WGC too — no point burning cycles capturing when nothing's streaming
    if (IPlugin* wgc = m_pluginsByName.value(QStringLiteral("WGC Capture"), nullptr)) {
        QMetaObject::invokeMethod(wgc->qobject(), "stopCapture", Qt::DirectConnection);
    }
    if (m_targetLabel) m_targetLabel->setText(QString());
    m_scriptStatus->setText(QStringLiteral("idle"));
    appendLog(QStringLiteral("Engine stopped"));
    updateActions();
}

void LabsMainWindow::onFpsTick()
{
    if (!m_activeSource || !m_fpsLabel) return;
    const qint64 now = m_activeSource->frameCount();
    const qint64 delta = now - m_lastFrameCount;
    m_lastFrameCount = now;
    // All three streaming modes (Xbox RP / xCloud / PS RP) push decoded
    // frames through the IFrameSink, so a real fps number is always available.
    m_fpsLabel->setText(QStringLiteral("%1 fps").arg(delta));
}

void LabsMainWindow::onRefreshScripts()
{
    if (!m_scriptCombo) return;

    const QString currentPath = m_scriptCombo->currentData().toString();
    const QString scriptsDir  = userScriptsDir();

    QStringList found;
    for (const QFileInfo& fi : QDir(scriptsDir).entryInfoList(
             QStringList{QStringLiteral("*.py")}, QDir::Files, QDir::Name)) {
        if (fi.fileName().startsWith(QChar('_'))) continue;
        if (fi.completeBaseName().endsWith(QStringLiteral("_test"))) continue;
        found << fi.absoluteFilePath();
    }
    // Preserve a custom path the user picked via Browse if it lives outside
    // the scripts folder.
    if (!currentPath.isEmpty() && QFileInfo::exists(currentPath) && !found.contains(currentPath))
        found.prepend(currentPath);

    m_scriptCombo->blockSignals(true);
    m_scriptCombo->clear();
    for (const QString& p : found)
        m_scriptCombo->addItem(QFileInfo(p).fileName(), p);
    int idx = currentPath.isEmpty() ? 0 : m_scriptCombo->findData(currentPath);
    if (idx < 0 && m_scriptCombo->count() > 0) idx = 0;
    if (idx >= 0) m_scriptCombo->setCurrentIndex(idx);
    m_scriptCombo->blockSignals(false);

    appendLog(QStringLiteral("scripts refreshed — %1 found").arg(found.size()));
    updateActions();
}

void LabsMainWindow::onBrowseScript()
{
    const QString start = m_scriptCombo ? m_scriptCombo->currentData().toString() : QString();
    const QString picked = QFileDialog::getOpenFileName(this,
        QStringLiteral("Pick CV script"),
        start.isEmpty() ? QDir::homePath() : QFileInfo(start).absolutePath(),
        QStringLiteral("Python (*.py);;All files (*.*)"));
    if (picked.isEmpty()) return;
    if (m_scriptCombo) {
        int idx = m_scriptCombo->findData(picked);
        if (idx < 0) {
            m_scriptCombo->addItem(QFileInfo(picked).fileName(), picked);
            idx = m_scriptCombo->count() - 1;
        }
        m_scriptCombo->setCurrentIndex(idx);
    }
    if (m_settings) m_settings->setValue(QStringLiteral("cv/scriptPath"), picked);
    appendLog(QStringLiteral("script → %1").arg(picked));
}

void LabsMainWindow::onRunScript()
{
    const QString path = m_scriptCombo ? m_scriptCombo->currentData().toString().trimmed() : QString();
    if (path.isEmpty()) {
        appendLog(QStringLiteral("select a script first"));
        return;
    }
    if (m_scriptProc && m_scriptProc->state() != QProcess::NotRunning) return;

    if (!m_scriptProc) {
        m_scriptProc = new QProcess(this);
        m_scriptProc->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_scriptProc, &QProcess::readyRead, this, [this]() {
            if (m_log) m_log->appendPlainText(QString::fromLocal8Bit(m_scriptProc->readAll()).trimmed());
        });
        connect(m_scriptProc,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int code, QProcess::ExitStatus) { onScriptFinished(code, 0); });
    }

    const QFileInfo fi(path);
    m_scriptProc->setWorkingDirectory(fi.absolutePath());

    // Build the script arg list. The perf toggle drives both:
    //   Low End  → --low-end  (60fps mss, every-2 frame, 250Hz passthrough)
    //   High End → --target-fps 240  (BetterCam at its real cap)
    //
    // --labs-pid + --session are what unlock the SHM frame path: scripts that
    // implement CV (xDrive3K, Labs2K, …) read frames from a SHM block named
    // Labs_<labs-pid>_Frame_<session>. PS Remote Play stamps sessionId=1, so
    // defaulting --session to 1 makes the manual Run path work end-to-end on
    // the PS path. xCloud uses spawnXboxSessionScript with its own session id.
    QStringList args { path };
    args << QStringLiteral("--labs-pid") << QString::number(QCoreApplication::applicationPid());
    args << QStringLiteral("--session")  << QStringLiteral("1");
    const bool low = m_perfLiteBtn && m_perfLiteBtn->isChecked();
    if (low) {
        args << QStringLiteral("--low-end");
    } else {
        args << QStringLiteral("--target-fps") << QStringLiteral("240");
    }

    m_scriptProc->start(QStringLiteral("python"), args);

    if (!m_scriptProc->waitForStarted(3000)) {
        appendLog(QStringLiteral("failed to start python — is it on PATH?"));
        return;
    }

    m_scriptStatus->setText(QStringLiteral("running"));
    appendLog(QStringLiteral("Script started — %1").arg(fi.fileName()));
    updateActions();
    if (m_settings) m_settings->setValue(QStringLiteral("cv/scriptPath"), path);
}

void LabsMainWindow::onStopScript()
{
    if (!m_scriptProc || m_scriptProc->state() == QProcess::NotRunning) return;
    m_scriptProc->terminate();
    if (!m_scriptProc->waitForFinished(2000))
        m_scriptProc->kill();
}

void LabsMainWindow::onScriptFinished(int exitCode, int /*exitStatus*/)
{
    m_scriptStatus->setText(exitCode == 0
        ? QStringLiteral("exited")
        : QStringLiteral("exited (%1)").arg(exitCode));
    appendLog(QStringLiteral("Script stopped"));
    Q_UNUSED(exitCode);
    updateActions();
}

void LabsMainWindow::onClearLog()
{
    if (m_log) m_log->clear();
}

void LabsMainWindow::onOpenTheme()
{
    LabsThemeDialog dlg(m_settings.get(), this);
    connect(&dlg, &LabsThemeDialog::themeChanged,
            this, &LabsMainWindow::applyThemeImage);
    dlg.exec();
    applyThemeImage();  // final state on close (handles cancel revert too)
}

void LabsMainWindow::updateActions()
{
    const bool haveSource = (m_activeSource != nullptr);
    const bool running = haveSource && m_activeSource->isRunning();
    const Mode mode = static_cast<Mode>(m_modeBox ? m_modeBox->currentIndex() : 0);
    const bool scriptRunning = m_scriptProc && m_scriptProc->state() != QProcess::NotRunning;

    if (m_btnPair)      m_btnPair ->setEnabled(!running);
    if (m_btnStart) {
        m_btnStart->setEnabled(haveSource && !running);
        m_btnStart->setVisible(!running);
    }
    if (m_btnStop) {
        m_btnStop ->setEnabled(haveSource &&  running);
        m_btnStop ->setVisible(running);
    }
    if (m_modeBox)      m_modeBox ->setEnabled(!running);
    const bool haveScript = m_scriptCombo && !m_scriptCombo->currentData().toString().trimmed().isEmpty();
    if (m_scriptRunBtn) {
        m_scriptRunBtn->setEnabled(haveSource && haveScript && !scriptRunning);
        m_scriptRunBtn->setVisible(!scriptRunning);
    }
    if (m_scriptStopBtn) {
        m_scriptStopBtn->setText(QStringLiteral("STOP SCRIPT"));
        m_scriptStopBtn->setEnabled(scriptRunning);
        m_scriptStopBtn->setVisible(scriptRunning);
    }

    // State pill — only visible when something is running. Shows the script
    // name when the script is up, otherwise just "STREAMING".
    if (m_statePill) {
        const bool anyRunning = running || scriptRunning;
        m_statePill->setVisible(anyRunning);
        if (anyRunning) {
            QString text;
            if (scriptRunning && m_scriptCombo) {
                const QString picked = m_scriptCombo->currentData().toString();
                const QString name = picked.isEmpty()
                    ? QStringLiteral("script") : QFileInfo(picked).fileName();
                text = QStringLiteral("running script: %1").arg(name);
            } else {
                text = QStringLiteral("streaming");
            }
            m_statePill->setText(text);
            m_statePill->setProperty("state", "running");
            m_statePill->style()->unpolish(m_statePill);
            m_statePill->style()->polish(m_statePill);
        }
    }
}

void LabsMainWindow::appendLog(const QString& text)
{
    if (m_log) m_log->appendPlainText(text);
}

} // namespace Labs
