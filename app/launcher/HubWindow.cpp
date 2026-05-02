#include "HubWindow.h"
#include "MarketplaceWidget.h"
#include "SettingsManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QGraphicsColorizeEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace Labs {

namespace {

constexpr const char* kBg       = "#FFFFFF";   // page bg
constexpr const char* kSidebar  = "#FAFBFC";   // sidebar surface, very subtle off-white
constexpr const char* kSelected = "#EEF2FF";   // selected sidebar row, faint blue tint
constexpr const char* kBorder   = "#E2E8F0";
constexpr const char* kText     = "#0F172A";
constexpr const char* kDim      = "#475569";
constexpr const char* kFaint    = "#94A3B8";
constexpr const char* kAccent   = "#3B82F6";
constexpr const char* kWarn     = "#F59E0B";

constexpr int kSidebarWidth     = 240;
constexpr int kSidebarRowHeight = 64;

// Build a QLabel showing the Labs logo at `side` px. If `glyphColor` is
// invalid (the default), the logo renders unmodified — the actual brand mark.
//
// If `glyphColor` is set, the lighter glyph pixels of labs.png (the L,
// the dot, the small accent bar) are replaced with that color while the
// dark navy rounded-square background is preserved. Result: every app
// shares the same dark Labs surface, with a different glyph color per
// slot. Used for Zen Coder (amber glyph) and Marketplace (emerald glyph)
// until they ship their own brand art.
//
// Threshold of 70 / 255 luminosity reliably separates the dark navy bg
// (qGray ~30) from the blue glyph + white accent bar (qGray ~110+).
QLabel* makeAppLogo(int side,
                    const QColor& glyphColor = QColor(),
                    qreal alpha = 1.0,
                    QWidget* parent = nullptr)
{
    auto* label = new QLabel(parent);
    QPixmap raw(QStringLiteral(":/labs/labs.png"));
    QPixmap scaled = raw.scaled(side, side,
                                Qt::KeepAspectRatio,
                                Qt::SmoothTransformation);

    if (glyphColor.isValid()) {
        QImage img = scaled.toImage().convertToFormat(QImage::Format_ARGB32);
        const int gr = glyphColor.red(), gg = glyphColor.green(), gb = glyphColor.blue();
        for (int y = 0; y < img.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                const QRgb c = line[x];
                const int a = qAlpha(c);
                if (a < 8) continue;                     // transparent corner pixels
                if (qGray(c) >= 70) {                    // light glyph pixel
                    line[x] = qRgba(gr, gg, gb, a);
                }
            }
        }
        scaled = QPixmap::fromImage(img);
    }

    if (alpha < 1.0) {
        QPixmap dim(scaled.size());
        dim.fill(Qt::transparent);
        QPainter p(&dim);
        p.setOpacity(alpha);
        p.drawPixmap(0, 0, scaled);
        scaled = dim;
    }
    label->setPixmap(scaled);
    label->setFixedSize(scaled.size());
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral("background: transparent;"));
    return label;
}

} // namespace


// ── SidebarEntry ─────────────────────────────────────────────────────────────
//
// One clickable row in the left rail: small tinted Labs glyph + app name.
// Subclasses QFrame so we can use stylesheet selectors for the selected state.
class SidebarEntry : public QFrame {
    Q_OBJECT
public:
    using ClickFn = std::function<void()>;

    SidebarEntry(const QString& name,
                 const QString& accentHex,
                 bool nativeBrand,
                 ClickFn cb,
                 QWidget* parent = nullptr)
        : QFrame(parent), m_cb(std::move(cb))
    {
        setObjectName(QStringLiteral("sidebarEntry"));
        setCursor(Qt::PointingHandCursor);
        setFixedHeight(kSidebarRowHeight);
        setAttribute(Qt::WA_Hover, true);

        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(16, 0, 16, 0);
        row->setSpacing(12);

        // nativeBrand = use the real labs.png as-is (Labs Engine).
        // Otherwise tint it to the app's accent (placeholder for apps
        // without their own brand mark yet).
        m_icon = makeAppLogo(28,
                             nativeBrand ? QColor() : QColor(accentHex),
                             1.0, this);
        row->addWidget(m_icon);

        m_label = new QLabel(name, this);
        QFont f(QStringLiteral("Segoe UI Variable Text"));
        f.setPointSize(10);
        f.setWeight(QFont::DemiBold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 0.6);
        m_label->setFont(f);
        m_label->setStyleSheet(QString("color: %1; background: transparent;").arg(kText));
        row->addWidget(m_label, 1);

        applyStyle();
    }

    void setSelected(bool s) {
        if (m_selected == s) return;
        m_selected = s;
        applyStyle();
    }

protected:
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) m_pressed = true;
    }
    void mouseReleaseEvent(QMouseEvent* e) override {
        const bool was = m_pressed; m_pressed = false;
        if (e->button() == Qt::LeftButton && was && rect().contains(e->pos()) && m_cb)
            m_cb();
    }
    void enterEvent(QEnterEvent*)   override { m_hover = true;  applyStyle(); }
    void leaveEvent(QEvent*)        override { m_hover = false; applyStyle(); }

private:
    void applyStyle() {
        QString bg = "transparent";
        if (m_selected)      bg = kSelected;
        else if (m_hover)    bg = "#F1F5F9";
        setStyleSheet(QString(
            "QFrame#sidebarEntry { background: %1; border: none; border-radius: 8px; }"
        ).arg(bg));
    }

    ClickFn m_cb;
    QLabel* m_icon = nullptr;
    QLabel* m_label = nullptr;
    bool m_selected = false;
    bool m_hover = false;
    bool m_pressed = false;
};


// ── HubWindow ────────────────────────────────────────────────────────────────

HubWindow::HubWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Labs Launcher"));
    setWindowIcon(QIcon(QStringLiteral(":/labs/labs.png")));
    resize(1100, 680);

    m_settings = new SettingsManager(this);

    setStyleSheet(QString(
        "QMainWindow { background: %1; }"
        "QWidget { color: %2; font-family: 'Segoe UI Variable Text','Segoe UI'; }"
    ).arg(kBg).arg(kText));

    auto* root = new QWidget(this);
    auto* row  = new QHBoxLayout(root);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    row->addWidget(buildSidebar());
    row->addWidget(buildHeroStack(), 1);

    setCentralWidget(root);

    // Default selection: first entry (Labs Engine).
    selectApp(0);
}

HubWindow::~HubWindow() = default;

QWidget* HubWindow::buildSidebar()
{
    auto* sidebar = new QWidget(this);
    sidebar->setFixedWidth(kSidebarWidth);
    sidebar->setStyleSheet(QString(
        "background: %1; border-right: 1px solid %2;"
    ).arg(kSidebar).arg(kBorder));

    auto* col = new QVBoxLayout(sidebar);
    col->setContentsMargins(14, 22, 14, 18);
    col->setSpacing(4);

    // Wordmark at the top — small, quiet.
    auto* mark = new QLabel(QStringLiteral("LABS"));
    {
        QFont f(QStringLiteral("Segoe UI Variable Display"));
        f.setPointSize(13);
        f.setWeight(QFont::Black);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 4.0);
        mark->setFont(f);
        mark->setStyleSheet(QString("color: %1; padding: 0 4px 8px 4px; background: transparent;").arg(kText));
    }
    col->addWidget(mark);

    auto* sub = new QLabel(QStringLiteral("LAUNCHER"));
    {
        QFont f(QStringLiteral("Segoe UI Variable Text"));
        f.setPointSize(8);
        f.setWeight(QFont::DemiBold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 2.4);
        sub->setFont(f);
        sub->setStyleSheet(QString("color: %1; padding: 0 4px 16px 4px; background: transparent;").arg(kFaint));
    }
    col->addWidget(sub);

    // Entries — clicking selects + drives the hero stack index.
    // `nativeBrand=true` means use the real labs.png unmodified (the app's
    // actual brand mark). Zen and Market are placeholders until they get
    // their own art.
    struct Entry { QString name; QString accent; bool nativeBrand; };
    const QList<Entry> entries = {
        { QStringLiteral("Labs Engine"), QStringLiteral("#3B82F6"), true  },
        { QStringLiteral("Zen Coder"),   QStringLiteral("#F59E0B"), false },
        { QStringLiteral("Marketplace"), QStringLiteral("#10B981"), false },
    };

    for (int i = 0; i < entries.size(); ++i) {
        auto* e = new SidebarEntry(entries[i].name,
                                   entries[i].accent,
                                   entries[i].nativeBrand,
                                   [this, i]() { selectApp(i); },
                                   sidebar);
        m_sidebarEntries.append(e);
        col->addWidget(e);
    }

    col->addStretch(1);

    auto* version = new QLabel(QStringLiteral("v0.1.0"));
    version->setStyleSheet(QString(
        "color: %1; padding: 0 4px; font-family: 'Cascadia Mono','Consolas';"
        "font-size: 10px; background: transparent;"
    ).arg(kFaint));
    col->addWidget(version);

    return sidebar;
}

QWidget* HubWindow::buildHeroStack()
{
    m_heroStack = new QStackedWidget(this);
    m_heroStack->setStyleSheet(QString("background: %1;").arg(kBg));

    m_heroStack->addWidget(buildAppHero(
        QStringLiteral("LABS ENGINE"),
        QStringLiteral("PS REMOTE PLAY · XCLOUD · SCRIPTS"),
        QStringLiteral("Multi-session xCloud and PlayStation Remote Play with full plugin support, the script bridge, and the per-session frame pipe. The runtime where everything actually plays."),
        QStringLiteral("LAUNCH"),
        QString(),
        true,
        QStringLiteral("#3B82F6"),
        true,    // nativeBrand — show the real labs.png as the hero anchor
        [this]() { onLaunchLabsEngine(); }));

    m_heroStack->addWidget(buildAppHero(
        QStringLiteral("ZEN CODER"),
        QStringLiteral("GPC3 · VISUAL SCRIPTING"),
        QStringLiteral("Build GPC3 scripts visually with a node graph instead of typing them. Drag the blocks, hit compile, ship to your device or publish straight to the Marketplace. In active development."),
        QStringLiteral("COMING SOON"),
        QStringLiteral("COMING SOON"),
        false,
        QStringLiteral("#F59E0B"),
        false,
        []() {}));

    m_heroStack->addWidget(buildMarketplaceHero());

    return m_heroStack;
}

QWidget* HubWindow::buildAppHero(const QString& titleTxt,
                                 const QString& subtitleTxt,
                                 const QString& descriptionTxt,
                                 const QString& actionText,
                                 const QString& badgeTxt,
                                 bool enabled,
                                 const QString& accentHex,
                                 bool nativeBrand,
                                 std::function<void()> onAction)
{
    auto* hero = new QWidget();
    hero->setObjectName(QStringLiteral("hero"));

    // Faint accent wash in the background corner — gives each app its own
    // identity without overwhelming the white surface. Same idea as the
    // XCOM-launcher hero art, just toned way down because we don't have
    // game art (yet).
    hero->setStyleSheet(QString(
        "QWidget#hero {"
        "  background: qradialgradient(cx:1.0,cy:0.15,radius:0.85,"
        "    stop:0 %1, stop:0.6 #FFFFFF, stop:1 #FFFFFF);"
        "}"
    ).arg(enabled ? "rgba(59,130,246,0.10)" : "rgba(245,158,11,0.06)"));

    auto* row = new QHBoxLayout(hero);
    row->setContentsMargins(56, 56, 56, 56);
    row->setSpacing(40);

    // Left: copy column — kicker, title, description, action.
    auto* col = new QVBoxLayout();
    col->setSpacing(10);
    col->addStretch(1);

    if (!badgeTxt.isEmpty()) {
        auto* badge = new QLabel(badgeTxt);
        badge->setStyleSheet(QString(
            "color: %1; background: rgba(245,158,11,0.12);"
            "border: 1px solid rgba(245,158,11,0.4);"
            "padding: 4px 10px; border-radius: 4px;"
            "font-family: 'Cascadia Mono','Consolas';"
            "font-size: 10px; font-weight: 700;"
            "letter-spacing: 1.4px;"
        ).arg(kWarn));
        badge->setFixedWidth(badge->fontMetrics().horizontalAdvance(badgeTxt) + 28);
        col->addWidget(badge);
        col->addSpacing(6);
    }

    auto* sub = new QLabel(subtitleTxt);
    sub->setStyleSheet(QString(
        "color: %1; font-size: 10px; letter-spacing: 2.6px;"
        "font-weight: 700; background: transparent;"
    ).arg(kDim));
    col->addWidget(sub);

    auto* title = new QLabel(titleTxt);
    {
        QFont f(QStringLiteral("Segoe UI Variable Display"));
        f.setPointSize(48);
        f.setWeight(QFont::Black);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
        title->setFont(f);
        title->setStyleSheet(QString("color: %1; background: transparent;").arg(kText));
        title->setWordWrap(true);
    }
    col->addWidget(title);

    auto* desc = new QLabel(descriptionTxt);
    desc->setStyleSheet(QString(
        "color: %1; font-size: 13px; line-height: 1.55;"
        "background: transparent;"
    ).arg(kDim));
    desc->setWordWrap(true);
    desc->setMaximumWidth(440);
    col->addSpacing(4);
    col->addWidget(desc);

    col->addSpacing(20);

    auto* action = new QPushButton(actionText);
    action->setCursor(enabled ? Qt::PointingHandCursor : Qt::ForbiddenCursor);
    action->setEnabled(enabled);
    action->setMinimumHeight(44);
    action->setFixedWidth(200);
    {
        QFont f(QStringLiteral("Segoe UI Variable Display"));
        f.setPointSize(11);
        f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 2.2);
        action->setFont(f);
    }
    action->setStyleSheet(QString(
        "QPushButton {"
        "  background: %1; color: white; border: none;"
        "  border-radius: 8px; padding: 0 24px;"
        "}"
        "QPushButton:hover { background: %2; }"
        "QPushButton:disabled {"
        "  background: rgba(15,23,42,0.06); color: %3;"
        "}"
    ).arg(accentHex).arg("#2563EB").arg(kFaint));
    if (enabled && onAction) {
        connect(action, &QPushButton::clicked, this,
                [cb = std::move(onAction)]() { cb(); });
    }
    col->addWidget(action, 0, Qt::AlignLeft);

    col->addStretch(1);
    row->addLayout(col, 1);

    // Right: big anchor glyph. Native brand → labs.png as-is; else tint.
    auto* logo = makeAppLogo(320,
                             nativeBrand ? QColor() : QColor(accentHex),
                             enabled ? 1.0 : 0.55);
    row->addWidget(logo, 0, Qt::AlignVCenter | Qt::AlignRight);

    return hero;
}

QWidget* HubWindow::buildMarketplaceHero()
{
    auto* page = new QWidget();
    auto* col = new QVBoxLayout(page);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    m_marketplace = new MarketplaceWidget(m_settings, page);
    col->addWidget(m_marketplace.data(), 1);

    return page;
}

void HubWindow::selectApp(int index)
{
    if (index < 0 || index >= m_sidebarEntries.size()) return;
    m_selectedIndex = index;
    for (int i = 0; i < m_sidebarEntries.size(); ++i)
        m_sidebarEntries[i]->setSelected(i == index);
    if (m_heroStack) m_heroStack->setCurrentIndex(index);
}

void HubWindow::onLaunchLabsEngine()
{
    const QString exe = resolveLabsEngineExe();
    if (exe.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("Labs Engine"),
                             QStringLiteral("LabsEngine.exe not found next to the launcher. "
                                            "Expected sibling path:\n%1")
                             .arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath())));
        return;
    }
    qint64 pid = 0;
    const bool ok = QProcess::startDetached(exe, {}, QFileInfo(exe).absolutePath(), &pid);
    if (!ok) {
        QMessageBox::warning(this,
                             QStringLiteral("Labs Engine"),
                             QStringLiteral("Failed to launch:\n%1").arg(exe));
        return;
    }
    qInfo() << "LabsLauncher: launched" << exe << "pid=" << pid;
}

QString HubWindow::resolveLabsEngineExe() const
{
    const QDir d(QCoreApplication::applicationDirPath());
    const QString candidate = d.absoluteFilePath(QStringLiteral("LabsEngine.exe"));
    return QFileInfo::exists(candidate) ? candidate : QString();
}

} // namespace Labs

#include "HubWindow.moc"
