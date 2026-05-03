#include "MarketplaceWidget.h"
#include "LabsPaths.h"
#include "SettingsManager.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QtMath>

namespace Labs {

namespace {
constexpr auto kSettingsRemoteUrl = "marketplace/remote_url";  // optional remote manifest
constexpr auto kSettingsKeyPrefix = "marketplace/keys/";       // appended with <slug>

QString slugify(const QString& s)
{
    QString out = s.toLower();
    out.replace(QRegularExpression(QStringLiteral("[^a-z0-9._-]+")), QStringLiteral("-"));
    while (out.startsWith('-')) out.remove(0, 1);
    while (out.endsWith('-'))   out.chop(1);
    return out;
}
}

MarketplaceWidget::MarketplaceWidget(SettingsManager* settings, QWidget* parent)
    : QWidget(parent), m_settings(settings)
{
    m_nam = new QNetworkAccessManager(this);
    // Light theme. Cards keep the blue thumbnail gradient so each one still
    // reads as a "tile"; everything else flips to white-ink on white-bg.
    setStyleSheet(QStringLiteral(
        "QWidget{background:#FFFFFF;}"
        "QScrollArea{border:none;background:transparent;}"
        "QLineEdit, QComboBox{background:#F8FAFC;border:1px solid #E2E8F0;color:#0F172A;"
            "padding:8px 12px;border-radius:4px;font-family:'Segoe UI Variable Text','Segoe UI';"
            "font-size:12px;min-height:18px;}"
        "QLineEdit:focus, QComboBox:focus{border-color:#3B82F6;}"
        "QComboBox::drop-down{border:none;width:24px;}"
        "QComboBox QAbstractItemView{background:#FFFFFF;color:#0F172A;border:1px solid #E2E8F0;"
            "selection-background-color:#DBEAFE;selection-color:#0F172A;}"
        "QPushButton#headerBtn{background:transparent;color:#475569;border:1px solid #E2E8F0;"
            "padding:7px 14px;border-radius:4px;font-size:12px;}"
        "QPushButton#headerBtn:hover{color:#0F172A;border-color:#3B82F6;}"
        "QPushButton#publishBtn{background:#3B82F6;color:white;border:none;"
            "padding:8px 18px;border-radius:4px;font-weight:600;font-size:12px;}"
        "QPushButton#publishBtn:hover{background:#2563EB;}"
        "QPushButton#publishBtn:disabled{background:#E2E8F0;color:#94A3B8;}"
        "QFrame#card{background:#FFFFFF;border:1px solid #E2E8F0;border-radius:10px;}"
        "QFrame#card:hover{border-color:#3B82F6;}"
        "QLabel#cardTitle{color:#0F172A;font-family:'Segoe UI Variable Display','Segoe UI';"
            "font-size:17px;font-weight:700;background:transparent;}"
        "QLabel#cardDesc{color:#475569;font-size:12px;line-height:1.55;background:transparent;}"
        "QLabel#cardMeta{color:#94A3B8;font-size:11px;font-family:'Cascadia Mono','Consolas';"
            "background:transparent;}"
        "QLabel#cardLikes{color:#DB2777;font-size:13px;font-family:'Cascadia Mono','Consolas';"
            "font-weight:600;background:transparent;}"
        "QLabel#cardType[ttype=\"py\"]   {color:#475569;background:#F1F5F9;padding:3px 8px;border-radius:4px;"
            "font-size:10px;font-weight:700;font-family:'Cascadia Mono','Consolas';}"
        "QLabel#cardType[ttype=\"gpc3\"] {color:#475569;background:#F1F5F9;padding:3px 8px;border-radius:4px;"
            "font-size:10px;font-weight:700;font-family:'Cascadia Mono','Consolas';}"
        "QLabel#cardType[ttype=\"ennx\"] {color:#6D28D9;background:#EDE9FE;padding:3px 8px;border-radius:4px;"
            "font-size:10px;font-weight:700;font-family:'Cascadia Mono','Consolas';}"
        "QLabel#cardType[ttype=\"henv\"] {color:#B45309;background:#FEF3C7;padding:3px 8px;border-radius:4px;"
            "font-size:10px;font-weight:700;font-family:'Cascadia Mono','Consolas';}"
        // Thumbnail gradient ends in deep blue (not near-black) so it doesn't
        // look like a black void clipped against a white card.
        "QLabel#cardThumb{background:qradialgradient(cx:0.5,cy:0.45,radius:0.85,"
            "stop:0 #60A5FA, stop:0.35 #3B82F6, stop:0.7 #1D4ED8, stop:1 #1E3A8A);"
            "border-top-left-radius:10px;border-top-right-radius:10px;}"
        "QPushButton#cardAction{background:#F1F5F9;color:#0F172A;border:1px solid #E2E8F0;"
            "padding:9px 16px;border-radius:6px;font-size:12px;font-weight:600;}"
        "QPushButton#cardAction:hover{background:#E2E8F0;border-color:#CBD5E1;}"
        "QPushButton#cardActionAccent{background:#3B82F6;color:white;border:none;"
            "padding:9px 16px;border-radius:6px;font-size:12px;font-weight:600;}"
        "QPushButton#cardActionAccent:hover{background:#2563EB;}"
        "QLabel#hdr{color:#0F172A;font-family:'Segoe UI Variable Display','Segoe UI';"
            "font-size:24px;font-weight:600;background:transparent;}"
        "QLabel#sub{color:#475569;font-size:13px;background:transparent;}"
    ));

    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(20, 14, 20, 16);
    col->setSpacing(12);

    // No big "Marketplace" header — the tab strip already labels this page,
    // matching how the video display and controller monitor tabs work.

    // Top toolbar — search, filters, publish.
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(QStringLiteral("Search scripts…"));
    connect(m_search, &QLineEdit::textChanged, this, &MarketplaceWidget::onSearchChanged);
    m_typeBox = new QComboBox(this);
    m_typeBox->addItem(QStringLiteral("All Types"), QString());
    m_typeBox->addItem(QStringLiteral("Python"), QStringLiteral("py"));
    m_typeBox->addItem(QStringLiteral("GPC3"),   QStringLiteral("gpc3"));
    m_typeBox->addItem(QStringLiteral("ENNX"),   QStringLiteral("ennx"));
    m_typeBox->addItem(QStringLiteral("HENV"),   QStringLiteral("henv"));
    connect(m_typeBox, &QComboBox::currentTextChanged, this, &MarketplaceWidget::onFilterChanged);
    m_sortBox = new QComboBox(this);
    m_sortBox->addItem(QStringLiteral("Popular"), QStringLiteral("popular"));
    m_sortBox->addItem(QStringLiteral("Newest"),  QStringLiteral("newest"));
    m_sortBox->addItem(QStringLiteral("A → Z"),   QStringLiteral("alpha"));
    connect(m_sortBox, &QComboBox::currentTextChanged, this, &MarketplaceWidget::onFilterChanged);
    m_refresh = new QPushButton(QStringLiteral("⟳"), this);
    m_refresh->setObjectName(QStringLiteral("headerBtn"));
    m_refresh->setToolTip(QStringLiteral("Refresh manifest"));
    connect(m_refresh, &QPushButton::clicked, this, &MarketplaceWidget::refresh);
    m_publish = new QPushButton(QStringLiteral("Publish Script"), this);
    m_publish->setObjectName(QStringLiteral("publishBtn"));
    connect(m_publish, &QPushButton::clicked, this, &MarketplaceWidget::onPublishClicked);

    toolbar->addWidget(m_search, 1);
    toolbar->addWidget(m_typeBox);
    toolbar->addWidget(m_sortBox);
    toolbar->addWidget(m_refresh);
    toolbar->addWidget(m_publish);
    col->addLayout(toolbar);

    // Card grid in a scroll area.
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    auto* gridHost = new QWidget(m_scroll);
    m_grid = new QGridLayout(gridHost);
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setSpacing(14);
    m_scroll->setWidget(gridHost);
    col->addWidget(m_scroll, 1);

    m_emptyLbl = new QLabel(QStringLiteral(""), this);
    m_emptyLbl->setStyleSheet(QStringLiteral("color:#64748B;padding:40px;"));
    m_emptyLbl->setAlignment(Qt::AlignCenter);
    col->addWidget(m_emptyLbl);

    refresh();
}

MarketplaceWidget::~MarketplaceWidget() = default;

QString MarketplaceWidget::accessKeyFor(const QString& slug) const
{
    if (!m_settings || slug.isEmpty()) return {};
    return m_settings->value(QString::fromLatin1(kSettingsKeyPrefix) + slug).toString();
}

void MarketplaceWidget::setAccessKeyFor(const QString& slug, const QString& key)
{
    if (!m_settings || slug.isEmpty()) return;
    m_settings->setValue(QString::fromLatin1(kSettingsKeyPrefix) + slug, key);
}

void MarketplaceWidget::clearAccessKeyFor(const QString& slug)
{
    if (!m_settings || slug.isEmpty()) return;
    m_settings->remove(QString::fromLatin1(kSettingsKeyPrefix) + slug);
}

QString MarketplaceWidget::cvScriptsDir() const
{
    // Canonical user-scripts dir shared by every Labs binary. Anything the
    // user installs here shows up in Labs Engine's script combo on its
    // next refresh. See app/core/LabsPaths.h for the rationale (different
    // applicationName per exe → different AppData paths without this).
    return Labs::Paths::userScriptsDir();
}

void MarketplaceWidget::refresh()
{
    loadManifest();
    rebuildGrid();
}

void MarketplaceWidget::loadManifest()
{
    m_all.clear();

    // 1) Local manifest at <cv-scripts>/marketplace.json (where Publish writes).
    const QString localPath = cvScriptsDir() + QStringLiteral("/marketplace.json");
    auto parseJson = [&](const QByteArray& bytes, bool markLocal) {
        QJsonParseError err {};
        auto doc = QJsonDocument::fromJson(bytes, &err);
        if (err.error != QJsonParseError::NoError) return;
        const QJsonArray arr = doc.isArray() ? doc.array()
                                : doc.object().value(QStringLiteral("scripts")).toArray();
        for (const auto& v : arr) {
            const auto o = v.toObject();
            MarketScript s;
            s.slug         = o.value(QStringLiteral("slug")).toString();
            s.name         = o.value(QStringLiteral("name")).toString();
            s.description  = o.value(QStringLiteral("description")).toString();
            s.type         = o.value(QStringLiteral("type")).toString(QStringLiteral("py")).toLower();
            // Tolerate both the original schema and the server's variant
            // ("creator" instead of "author", "download_url" instead of "download").
            s.author       = o.value(QStringLiteral("author")).toString();
            if (s.author.isEmpty())
                s.author   = o.value(QStringLiteral("creator")).toString();
            s.version      = o.value(QStringLiteral("version")).toString();
            s.thumbnailUrl = o.value(QStringLiteral("thumbnail")).toString();
            if (s.thumbnailUrl.isEmpty())
                s.thumbnailUrl = o.value(QStringLiteral("thumbnail_url")).toString();
            s.downloadUrl  = o.value(QStringLiteral("download")).toString();
            if (s.downloadUrl.isEmpty())
                s.downloadUrl = o.value(QStringLiteral("download_url")).toString();
            s.localFilename = o.value(QStringLiteral("filename")).toString();
            s.likes        = o.value(QStringLiteral("likes")).toVariant().toLongLong();
            s.downloads    = o.value(QStringLiteral("downloads")).toVariant().toLongLong();
            s.isPrivate    = o.value(QStringLiteral("is_private")).toBool(false);
            s.isLocal      = markLocal;
            if (s.slug.isEmpty()) s.slug = slugify(s.name);
            // Mark installed if a file with the resolved name already exists.
            const QString fname = s.localFilename.isEmpty()
                                  ? QFileInfo(QUrl(s.downloadUrl).path()).fileName()
                                  : s.localFilename;
            s.isInstalled = !fname.isEmpty() && QFileInfo::exists(cvScriptsDir() + "/" + fname);
            if (!s.name.isEmpty()) m_all.append(s);
        }
    };
    if (QFile::exists(localPath)) {
        QFile f(localPath);
        if (f.open(QIODevice::ReadOnly)) parseJson(f.readAll(), true);
    }

    // First-launch samples — populate the grid so the user can see the layout
    // before anything's actually published. These are non-installable templates;
    // Install will no-op until you wire a real download URL or use Publish.
    if (m_all.isEmpty()) {
        struct Demo {
            const char* name; const char* desc; const char* type;
            const char* version; qint64 likes; qint64 downloads;
        };
        static const Demo demos[] = {
            { "Labs2K Vision", "Shot-meter timing for NBA 2K. Reads green-flash on release via InputOverride.",
              "py",   "v1.00",  483, 26621 },
            { "Aim Engine",    "Universal aim assist for FPS titles. Reads frame buffer + drives ViGEm sticks.",
              "py",   "v3.10",  271, 14802 },
            { "Skele",         "2K Skele shooting mode add-on for Labs2K Vision. Bigger green window.",
              "ennx", "v1.00",  152,  6431 },
            { "MLB Vision",    "Pitch-timing CV script for MLB The Show. Frame-accurate contact window.",
              "py",   "v1.00",  198,  9210 },
            { "Input Sense",   "GPC3 input bridge — pipe Labs gamepad state into a Titan Two / Cronus.",
              "gpc3", "v2.00",  337, 11445 },
        };
        for (const auto& d : demos) {
            MarketScript s;
            s.name        = QString::fromLatin1(d.name);
            s.description = QString::fromLatin1(d.desc);
            s.type        = QString::fromLatin1(d.type);
            s.author      = QStringLiteral("labs");
            s.version     = QString::fromLatin1(d.version);
            s.likes       = d.likes;
            s.downloads   = d.downloads;
            s.isLocal     = true;
            s.isInstalled = false;
            m_all.append(s);
        }
    }

    // 2) Optional remote manifest URL stored in settings.
    const QString remoteUrl = m_settings ? m_settings->value(kSettingsRemoteUrl).toString()
                                         : QString();
    if (!remoteUrl.isEmpty()) {
        QNetworkRequest req((QUrl(remoteUrl)));
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        auto* reply = m_nam->get(req);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(4000, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() == QNetworkReply::NoError) parseJson(reply->readAll(), false);
        reply->deleteLater();
    }
}

void MarketplaceWidget::saveLocalManifest()
{
    const QString localPath = cvScriptsDir() + QStringLiteral("/marketplace.json");
    QJsonArray arr;
    for (const auto& s : m_all) {
        if (!s.isLocal) continue;     // only local entries get persisted by Publish
        QJsonObject o;
        o[QStringLiteral("slug")]        = s.slug;
        o[QStringLiteral("name")]        = s.name;
        o[QStringLiteral("description")] = s.description;
        o[QStringLiteral("type")]        = s.type;
        o[QStringLiteral("author")]      = s.author;
        o[QStringLiteral("version")]     = s.version;
        o[QStringLiteral("thumbnail")]   = s.thumbnailUrl;
        o[QStringLiteral("download")]    = s.downloadUrl;
        o[QStringLiteral("filename")]    = s.localFilename;
        o[QStringLiteral("likes")]       = s.likes;
        o[QStringLiteral("downloads")]   = s.downloads;
        o[QStringLiteral("is_private")]  = s.isPrivate;
        arr.append(o);
    }
    QFile f(localPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    }
}

void MarketplaceWidget::onSearchChanged(const QString& text)
{
    m_filterText = text.trimmed().toLower();
    rebuildGrid();
}

void MarketplaceWidget::onFilterChanged()
{
    m_filterType = m_typeBox->currentData().toString();
    m_filterSort = m_sortBox->currentData().toString();
    rebuildGrid();
}

void MarketplaceWidget::rebuildGrid()
{
    while (m_grid->count() > 0) {
        QLayoutItem* it = m_grid->takeAt(0);
        if (!it) break;
        if (auto* w = it->widget()) w->deleteLater();
        delete it;
    }
    QVector<MarketScript> filtered;
    for (const auto& s : m_all) {
        if (!m_filterType.isEmpty() && s.type != m_filterType) continue;
        if (!m_filterText.isEmpty()) {
            const QString hay = (s.name + " " + s.description + " " + s.author).toLower();
            if (!hay.contains(m_filterText)) continue;
        }
        filtered.append(s);
    }
    if (m_filterSort == QStringLiteral("newest")) {
        // No real timestamps yet; preserve insert order
    } else if (m_filterSort == QStringLiteral("alpha")) {
        std::sort(filtered.begin(), filtered.end(), [](const MarketScript& a, const MarketScript& b){
            return a.name.toLower() < b.name.toLower();
        });
    } else { // popular
        std::sort(filtered.begin(), filtered.end(), [](const MarketScript& a, const MarketScript& b){
            return a.downloads > b.downloads;
        });
    }
    if (filtered.isEmpty()) {
        m_emptyLbl->setText(m_all.isEmpty()
            ? QStringLiteral("No scripts yet. Click Publish Script to add one.")
            : QStringLiteral("No matches."));
        m_emptyLbl->setVisible(true);
        return;
    }
    m_emptyLbl->setVisible(false);
    constexpr int cols = 4;
    for (int i = 0; i < filtered.size(); ++i) {
        // Find the index in m_all for action callbacks.
        int origIndex = -1;
        for (int j = 0; j < m_all.size(); ++j) {
            if (m_all[j].name == filtered[i].name && m_all[j].author == filtered[i].author) {
                origIndex = j; break;
            }
        }
        QWidget* card = buildCard(filtered[i], origIndex);
        m_grid->addWidget(card, i / cols, i % cols);
    }
    for (int c = 0; c < cols; ++c) m_grid->setColumnStretch(c, 1);
}

QWidget* MarketplaceWidget::buildCard(const MarketScript& s, int index)
{
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("card"));
    card->setMinimumSize(240, 380);
    card->setMaximumHeight(420);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* col = new QVBoxLayout(card);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    // Thumbnail — bigger hero image area like the Helios cards.
    auto* thumb = new QLabel(card);
    thumb->setObjectName(QStringLiteral("cardThumb"));
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setFixedHeight(180);
    thumb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if (!s.thumbnailUrl.isEmpty() && s.thumbnailUrl.startsWith(QStringLiteral("file:"))) {
        QPixmap px(QUrl(s.thumbnailUrl).toLocalFile());
        if (!px.isNull()) {
            thumb->setPixmap(px.scaled(560, 160,
                                       Qt::KeepAspectRatioByExpanding,
                                       Qt::SmoothTransformation));
            thumb->setScaledContents(false);
        }
    }
    if (thumb->pixmap().isNull()) {
        // Big hero wordmark — Labs II style. Two-tone "Labs" + roman numeral.
        thumb->setText(QStringLiteral(
            "<div style='font-family:Segoe UI Variable Display;font-weight:800;"
            "letter-spacing:-1px;text-shadow:0 6px 24px rgba(0,0,0,0.65);'>"
              "<span style='font-size:62px;color:#FFFFFF;'>Labs</span>"
              "<span style='font-size:62px;color:#BFDBFE;'> II</span>"
            "</div>"));
        thumb->setTextFormat(Qt::RichText);
    }
    col->addWidget(thumb);

    auto* body = new QWidget(card);
    auto* bcol = new QVBoxLayout(body);
    bcol->setContentsMargins(12, 10, 12, 10);
    bcol->setSpacing(5);

    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(8);
    auto* title = new QLabel(s.name, body);
    title->setObjectName(QStringLiteral("cardTitle"));
    title->setWordWrap(true);
    auto* type = new QLabel(s.type.toUpper(), body);
    type->setObjectName(QStringLiteral("cardType"));
    type->setProperty("ttype", s.type);
    titleRow->addWidget(title, 1);
    if (s.isPrivate) {
        auto* lock = new QLabel(QStringLiteral("🔒 LOCKED"), body);
        lock->setStyleSheet(QStringLiteral(
            "color:#B45309;background:#FEF3C7;padding:3px 8px;border-radius:4px;"
            "font-size:10px;font-weight:700;font-family:'Cascadia Mono','Consolas';"));
        lock->setToolTip(QStringLiteral("Requires an access key from the creator."));
        titleRow->addWidget(lock, 0, Qt::AlignTop);
    }
    titleRow->addWidget(type, 0, Qt::AlignTop);
    bcol->addLayout(titleRow);

    auto* desc = new QLabel(s.description.isEmpty()
                            ? QStringLiteral("No description.") : s.description, body);
    desc->setObjectName(QStringLiteral("cardDesc"));
    desc->setWordWrap(true);
    desc->setMinimumHeight(60);
    desc->setMaximumHeight(80);
    desc->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    bcol->addWidget(desc, 1);

    QStringList metaParts;
    if (!s.version.isEmpty()) metaParts << s.version;
    if (!s.author.isEmpty())  metaParts << QStringLiteral("by %1").arg(s.author);
    auto* meta = new QLabel(metaParts.join(QStringLiteral(" · ")), body);
    meta->setObjectName(QStringLiteral("cardMeta"));
    bcol->addWidget(meta);

    // Format download count Helios-style: 1.2K, 26.6K, 1.4M.
    auto humanCount = [](qint64 n) -> QString {
        if (n >= 1'000'000)
            return QString::number(n / 1'000'000.0, 'f', 1) + QStringLiteral("M");
        if (n >= 1000)
            return QString::number(n / 1000.0, 'f', 1) + QStringLiteral("K");
        return QString::number(n);
    };

    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(10);
    auto* likes = new QLabel(QStringLiteral("♥ %1").arg(humanCount(s.likes)), body);
    likes->setObjectName(QStringLiteral("cardLikes"));
    actionRow->addWidget(likes);
    actionRow->addStretch();

    const bool needsKey = s.isPrivate && !s.isInstalled && accessKeyFor(s.slug).isEmpty();

    if (needsKey) {
        // Locked + no key on file. Replace the install button with an input
        // that captures the key and stores it under marketplace/keys/<slug>.
        // The grid rebuilds after save, which then renders the Install button.
        auto* keyEdit = new QLineEdit(body);
        keyEdit->setPlaceholderText(QStringLiteral("Access key…"));
        keyEdit->setEchoMode(QLineEdit::Password);
        keyEdit->setMinimumWidth(120);
        auto* save = new QPushButton(QStringLiteral("Save"), body);
        save->setObjectName(QStringLiteral("cardActionAccent"));
        const QString slug = s.slug;
        auto commit = [this, slug, keyEdit]() {
            const QString k = keyEdit->text().trimmed();
            if (k.isEmpty()) return;
            setAccessKeyFor(slug, k);
            rebuildGrid();
        };
        connect(save,    &QPushButton::clicked,    this, commit);
        connect(keyEdit, &QLineEdit::returnPressed, this, commit);
        actionRow->addWidget(keyEdit, 1);
        actionRow->addWidget(save);
    } else {
        auto* btn = new QPushButton(body);
        if (s.isInstalled) {
            btn->setText(QStringLiteral("%1  ↓  Run").arg(humanCount(s.downloads)));
            btn->setObjectName(QStringLiteral("cardActionAccent"));
            connect(btn, &QPushButton::clicked, this, [this, index]() { runScript(index); });
        } else {
            btn->setText(QStringLiteral("%1  ↓  Install").arg(humanCount(s.downloads)));
            btn->setObjectName(QStringLiteral("cardAction"));
            connect(btn, &QPushButton::clicked, this, [this, index]() { installScript(index); });
        }
        actionRow->addWidget(btn);
    }
    bcol->addLayout(actionRow);

    col->addWidget(body);
    return card;
}

void MarketplaceWidget::installScript(int index)
{
    if (index < 0 || index >= m_all.size()) return;
    MarketScript& s = m_all[index];

    auto resolveLocalName = [&]() {
        if (!s.localFilename.isEmpty()) return s.localFilename;
        const QString fromUrl = QFileInfo(QUrl(s.downloadUrl).path()).fileName();
        if (!fromUrl.isEmpty()) return fromUrl;
        return QString(s.name).replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), "_")
               + QStringLiteral(".") + s.type;
    };

    if (s.downloadUrl.startsWith(QStringLiteral("file:"))) {
        // Local file — just copy.
        const QString src = QUrl(s.downloadUrl).toLocalFile();
        const QString dst = cvScriptsDir() + "/" + resolveLocalName();
        QFile::remove(dst);
        if (QFile::copy(src, dst)) {
            s.isInstalled = true;
            s.localFilename = QFileInfo(dst).fileName();
            QMessageBox::information(this, QStringLiteral("Installed"),
                QStringLiteral("Saved to %1").arg(dst));
            emit scriptListChanged();
            saveLocalManifest();
            rebuildGrid();
        } else {
            QMessageBox::warning(this, QStringLiteral("Install failed"),
                QStringLiteral("Could not copy %1").arg(src));
        }
        return;
    }

    // Remote download. Private scripts append ?token=<stored-key>. Nginx
    // hands the token to the validator via auth_request; bad keys come back
    // as a 403, which we treat as "key invalid — clear it and re-prompt".
    QUrl url(s.downloadUrl);
    if (s.isPrivate) {
        const QString key = accessKeyFor(s.slug);
        if (key.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Install"),
                QStringLiteral("This script is locked. Enter an access key first."));
            return;
        }
        QUrlQuery q(url.query());
        q.removeQueryItem(QStringLiteral("token"));
        q.addQueryItem(QStringLiteral("token"), key);
        url.setQuery(q);
    }
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
        reply->deleteLater();
        if (index < 0 || index >= m_all.size()) return;
        MarketScript& s = m_all[index];
        if (reply->error() != QNetworkReply::NoError) {
            const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (s.isPrivate && (http == 401 || http == 403)) {
                clearAccessKeyFor(s.slug);
                QMessageBox::warning(this, QStringLiteral("Install failed"),
                    QStringLiteral("Access key was rejected. Enter a new one and try again."));
                rebuildGrid();
                return;
            }
            QMessageBox::warning(this, QStringLiteral("Install failed"),
                QStringLiteral("Download error: %1").arg(reply->errorString()));
            return;
        }
        const QByteArray bytes = reply->readAll();

        // Sniff the payload — a ZIP magic header (PK\x03\x04) means this is a
        // multi-file product (xDrive3K bundles UI/CV/bridges + fonts), not a
        // single .py drop. Fall through to the zip-install branch in that case.
        const bool isZip =
            bytes.size() >= 4 &&
            bytes[0] == 'P' && bytes[1] == 'K' &&
            quint8(bytes[2]) == 0x03 && quint8(bytes[3]) == 0x04;

        if (isZip) {
            // Stage to %TEMP% and shell out to bsdtar (Windows 10+ bundles
            // tar.exe in System32; it groks zip natively). Avoids pulling in
            // QuaZip / private QZipReader headers for a one-liner extraction.
            QString tempZip = QDir::temp().filePath(
                QStringLiteral("labs-install-%1.zip").arg(QString::number(QDateTime::currentMSecsSinceEpoch())));
            QFile tf(tempZip);
            if (!tf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QMessageBox::warning(this, QStringLiteral("Install failed"),
                    QStringLiteral("Cannot stage zip at %1").arg(tempZip));
                return;
            }
            tf.write(bytes);
            tf.close();

            QDir().mkpath(cvScriptsDir());
            QProcess tar;
            tar.setWorkingDirectory(cvScriptsDir());
            tar.start(QStringLiteral("tar"),
                      {QStringLiteral("-xf"), tempZip, QStringLiteral("-C"), cvScriptsDir()});
            const bool ok = tar.waitForFinished(30000) && tar.exitCode() == 0;
            QFile::remove(tempZip);
            if (!ok) {
                QMessageBox::warning(this, QStringLiteral("Install failed"),
                    QStringLiteral("Could not extract archive: %1")
                        .arg(QString::fromLocal8Bit(tar.readAllStandardError()).trimmed()));
                return;
            }

            // Entry point — manifest's "filename" is authoritative; if it's
            // missing, fall back to <slug>.py and let runScript surface the
            // error rather than guessing the wrong file.
            const QString entry = s.localFilename.isEmpty()
                ? (s.slug + QStringLiteral(".py"))
                : s.localFilename;
            s.isInstalled    = true;
            s.localFilename  = entry;
            s.downloads      = s.downloads + 1;
            emit scriptListChanged();
            saveLocalManifest();
            rebuildGrid();
            return;
        }

        const QString localName = s.localFilename.isEmpty()
            ? QFileInfo(QUrl(s.downloadUrl).path()).fileName()
            : s.localFilename;
        const QString dst = cvScriptsDir() + "/" + localName;
        QFile out(dst);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(this, QStringLiteral("Install failed"),
                QStringLiteral("Cannot write %1").arg(dst));
            return;
        }
        out.write(bytes);
        s.isInstalled    = true;
        s.localFilename  = localName;
        s.downloads      = s.downloads + 1;
        emit scriptListChanged();
        saveLocalManifest();
        rebuildGrid();
    });
}

void MarketplaceWidget::runScript(int index)
{
    if (index < 0 || index >= m_all.size()) return;
    const MarketScript& s = m_all[index];
    const QString path = cvScriptsDir() + "/" + s.localFilename;
    if (!QFile::exists(path)) {
        QMessageBox::warning(this, QStringLiteral("Run"),
            QStringLiteral("Script not found at %1").arg(path));
        return;
    }
    emit runRequested(path);
}

void MarketplaceWidget::onPublishClicked()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Publish a script"));
    dlg.setStyleSheet(this->styleSheet());
    dlg.setMinimumWidth(480);
    auto* form = new QFormLayout(&dlg);
    auto* nameEdit  = new QLineEdit(&dlg);
    auto* descEdit  = new QTextEdit(&dlg); descEdit->setMaximumHeight(80);
    auto* typeBox   = new QComboBox(&dlg);
    typeBox->addItems({QStringLiteral("py"), QStringLiteral("gpc3"),
                       QStringLiteral("ennx"), QStringLiteral("henv")});
    auto* authorEdit  = new QLineEdit(&dlg);
    auto* versionEdit = new QLineEdit(&dlg); versionEdit->setText(QStringLiteral("v1.00"));
    auto* privateBox  = new QCheckBox(QStringLiteral("Locked (require access key)"), &dlg);
    auto* fileEdit   = new QLineEdit(&dlg);
    auto* fileBrowse = new QPushButton(QStringLiteral("Browse…"), &dlg);
    fileBrowse->setObjectName(QStringLiteral("headerBtn"));
    connect(fileBrowse, &QPushButton::clicked, &dlg, [&]() {
        const QString p = QFileDialog::getOpenFileName(&dlg, QStringLiteral("Pick script file"),
            QString(), QStringLiteral("Scripts (*.py *.gpc *.gpc3 *.ennx *.henv);;All files (*.*)"));
        if (!p.isEmpty()) fileEdit->setText(p);
    });
    auto* fileRow = new QHBoxLayout();
    fileRow->addWidget(fileEdit, 1);
    fileRow->addWidget(fileBrowse);
    auto* thumbEdit  = new QLineEdit(&dlg);
    auto* thumbBrowse = new QPushButton(QStringLiteral("Browse…"), &dlg);
    thumbBrowse->setObjectName(QStringLiteral("headerBtn"));
    connect(thumbBrowse, &QPushButton::clicked, &dlg, [&]() {
        const QString p = QFileDialog::getOpenFileName(&dlg, QStringLiteral("Pick thumbnail"),
            QString(), QStringLiteral("Images (*.png *.jpg *.jpeg *.webp);;All files (*.*)"));
        if (!p.isEmpty()) thumbEdit->setText(p);
    });
    auto* thumbRow = new QHBoxLayout();
    thumbRow->addWidget(thumbEdit, 1);
    thumbRow->addWidget(thumbBrowse);

    form->addRow(QStringLiteral("Name"),     nameEdit);
    form->addRow(QStringLiteral("Description"), descEdit);
    form->addRow(QStringLiteral("Type"),     typeBox);
    form->addRow(QStringLiteral("Author"),   authorEdit);
    form->addRow(QStringLiteral("Version"),  versionEdit);
    form->addRow(QStringLiteral(""),         privateBox);
    form->addRow(QStringLiteral("Script file"),  fileRow);
    form->addRow(QStringLiteral("Thumbnail (optional)"), thumbRow);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), &dlg);
    cancel->setObjectName(QStringLiteral("headerBtn"));
    auto* submit = new QPushButton(QStringLiteral("Publish"), &dlg);
    submit->setObjectName(QStringLiteral("publishBtn"));
    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(submit, &QPushButton::clicked, &dlg, &QDialog::accept);
    btnRow->addWidget(cancel);
    btnRow->addWidget(submit);
    form->addRow(btnRow);

    if (dlg.exec() != QDialog::Accepted) return;

    const QString name = nameEdit->text().trimmed();
    const QString srcFile = fileEdit->text().trimmed();
    if (name.isEmpty() || srcFile.isEmpty() || !QFile::exists(srcFile)) {
        QMessageBox::warning(this, QStringLiteral("Publish"),
            QStringLiteral("Name and a valid script file are required."));
        return;
    }

    // Stage script + thumbnail under cv-scripts/published/<sanitised-name>/
    QString slug = name;
    slug.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), "-");
    const QString publishDir = cvScriptsDir() + "/published/" + slug;
    QDir().mkpath(publishDir);

    const QString fileName = QFileInfo(srcFile).fileName();
    QFile::remove(publishDir + "/" + fileName);
    QFile::copy(srcFile, publishDir + "/" + fileName);

    QString thumbStored;
    if (!thumbEdit->text().trimmed().isEmpty() && QFile::exists(thumbEdit->text().trimmed())) {
        const QString thumbName = QStringLiteral("thumbnail.") + QFileInfo(thumbEdit->text()).suffix();
        QFile::remove(publishDir + "/" + thumbName);
        QFile::copy(thumbEdit->text().trimmed(), publishDir + "/" + thumbName);
        thumbStored = QStringLiteral("file:///") + publishDir + "/" + thumbName;
    }

    // Add to m_all + persist local manifest. Also install it (copy to cv-scripts root).
    MarketScript ns;
    ns.slug          = slug.toLower();
    ns.name          = name;
    ns.description   = descEdit->toPlainText().trimmed();
    ns.type          = typeBox->currentText();
    ns.author        = authorEdit->text().trimmed();
    ns.version       = versionEdit->text().trimmed();
    ns.thumbnailUrl  = thumbStored;
    ns.downloadUrl   = QStringLiteral("file:///") + publishDir + "/" + fileName;
    ns.localFilename = fileName;
    ns.likes         = 0;
    ns.downloads     = 0;
    ns.isLocal       = true;
    ns.isInstalled   = true;
    ns.isPrivate     = privateBox->isChecked();

    // Also drop a copy into the live cv-scripts dir so it shows in the left rail.
    QFile::remove(cvScriptsDir() + "/" + fileName);
    QFile::copy(srcFile, cvScriptsDir() + "/" + fileName);

    m_all.prepend(ns);
    saveLocalManifest();
    emit scriptListChanged();
    rebuildGrid();

    QMessageBox::information(this, QStringLiteral("Published"),
        QStringLiteral("Saved to %1\nManifest entry persisted.").arg(publishDir));
}

} // namespace Labs
