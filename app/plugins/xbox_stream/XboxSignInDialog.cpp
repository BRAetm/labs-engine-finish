#include "XboxSignInDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace Labs {

XboxSignInDialog::XboxSignInDialog(const QString& electronExe,
                                   const QString& sidecarDir,
                                   const QString& tokensFile,
                                   QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Sign in to Xbox"));
    setModal(true);
    setMinimumWidth(480);

    auto* root = new QVBoxLayout(this);

    auto* intro = new QLabel(tr("Sign in with your Microsoft / Xbox account to stream your console or Game Pass library."), this);
    intro->setWordWrap(true);
    root->addWidget(intro);

    m_statusLabel = new QLabel(tr("Requesting device code…"), this);
    m_statusLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #8af;"));
    root->addWidget(m_statusLabel);

    m_urlLabel = new QLabel(this);
    m_urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    m_urlLabel->setOpenExternalLinks(true);
    root->addWidget(m_urlLabel);

    m_codeLabel = new QLabel(this);
    m_codeLabel->setStyleSheet(QStringLiteral("font-family: Consolas, monospace; font-size: 18pt; font-weight: 600; letter-spacing: 4px; padding: 6px;"));
    m_codeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_codeLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_codeLabel);

    auto* btnRow = new QHBoxLayout;
    m_openBtn   = new QPushButton(tr("Open in browser"), this);
    m_copyBtn   = new QPushButton(tr("Copy code"),       this);
    m_cancelBtn = new QPushButton(tr("Cancel"),          this);
    m_openBtn->setEnabled(false);
    m_copyBtn->setEnabled(false);
    btnRow->addWidget(m_openBtn);
    btnRow->addWidget(m_copyBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    root->addLayout(btnRow);

    connect(m_openBtn,   &QPushButton::clicked, this, &XboxSignInDialog::onOpenBrowser);
    connect(m_copyBtn,   &QPushButton::clicked, this, &XboxSignInDialog::onCopyCode);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    // Spawn auth.js with ELECTRON_RUN_AS_NODE so we don't need a separate
    // node.exe in the install — Electron runs the script in node mode.
    m_proc = new QProcess(this);
    m_proc->setWorkingDirectory(sidecarDir);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("ELECTRON_RUN_AS_NODE"), QStringLiteral("1"));
    m_proc->setProcessEnvironment(env);
    m_proc->setProgram(electronExe);
    m_proc->setArguments({
        QStringLiteral("auth.js"),
        QStringLiteral("auth"),
        QStringLiteral("--tokens-file=%1").arg(tokensFile),
    });

    connect(m_proc, &QProcess::readyReadStandardOutput, this, &XboxSignInDialog::onStdout);
    connect(m_proc, &QProcess::readyReadStandardError,  this, &XboxSignInDialog::onStderr);
    connect(m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        onFinished(code);
    });

    m_proc->start();
    if (!m_proc->waitForStarted(5000)) {
        setStatus(tr("Failed to launch sign-in helper: %1").arg(m_proc->errorString()));
        m_cancelBtn->setText(tr("Close"));
    }
}

XboxSignInDialog::~XboxSignInDialog()
{
    if (m_proc) {
        // Disconnect from `this` BEFORE teardown. ~QObject destroys children
        // in declaration order, so m_statusLabel (declared first) dies before
        // m_proc (declared later).  ~QProcess then flushes a final finished()
        // signal that re-enters our onFinished() lambda → setStatus(text) →
        // m_statusLabel->setText(...), which dereferences a freed widget
        // (rax = 0xddddddddddde055 in dump LabsEngine.exe.40796.dmp).
        m_proc->disconnect(this);
        if (m_proc->state() != QProcess::NotRunning) {
            m_proc->terminate();
            if (!m_proc->waitForFinished(1500)) m_proc->kill();
        }
    }
}

void XboxSignInDialog::setStatus(const QString& text)
{
    m_statusLabel->setText(text);
}

void XboxSignInDialog::onStdout()
{
    m_stdoutBuf.append(m_proc->readAllStandardOutput());
    int nl;
    while ((nl = m_stdoutBuf.indexOf('\n')) != -1) {
        const QByteArray line = m_stdoutBuf.left(nl).trimmed();
        m_stdoutBuf.remove(0, nl + 1);
        if (!line.isEmpty()) handleProgressLine(line);
    }
}

void XboxSignInDialog::onStderr()
{
    const QByteArray chunk = m_proc->readAllStandardError();
    for (const auto& l : chunk.split('\n')) {
        const auto t = l.trimmed();
        if (!t.isEmpty()) qInfo().noquote() << "[xbox-auth]" << t;
    }
}

void XboxSignInDialog::handleProgressLine(const QByteArray& line)
{
    QJsonParseError err {};
    const auto doc = QJsonDocument::fromJson(line, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;
    const QJsonObject o = doc.object();
    const QString type = o.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("device-code")) {
        m_verificationUri = o.value(QStringLiteral("verificationUri")).toString();
        m_userCode        = o.value(QStringLiteral("userCode")).toString();
        m_urlLabel->setText(tr("1. Open: <a href=\"%1\">%1</a>").arg(m_verificationUri));
        m_codeLabel->setText(m_userCode);
        setStatus(tr("2. Enter the code above, then sign in. Waiting…"));
        m_openBtn->setEnabled(!m_verificationUri.isEmpty());
        m_copyBtn->setEnabled(!m_userCode.isEmpty());
    } else if (type == QStringLiteral("progress")) {
        setStatus(o.value(QStringLiteral("message")).toString());
    } else if (type == QStringLiteral("done")) {
        const bool ok = o.value(QStringLiteral("ok")).toBool();
        if (ok) {
            setStatus(tr("Signed in. Tokens saved."));
            accept();
        } else {
            setStatus(tr("Sign-in failed: %1").arg(o.value(QStringLiteral("error")).toString()));
            m_cancelBtn->setText(tr("Close"));
        }
    }
}

void XboxSignInDialog::onFinished(int exitCode)
{
    if (result() == QDialog::Accepted) return;
    if (exitCode != 0) {
        setStatus(tr("Sign-in helper exited with code %1.").arg(exitCode));
        m_cancelBtn->setText(tr("Close"));
    }
}

void XboxSignInDialog::onCopyCode()
{
    if (!m_userCode.isEmpty()) {
        QApplication::clipboard()->setText(m_userCode);
        m_copyBtn->setText(tr("Copied!"));
    }
}

void XboxSignInDialog::onOpenBrowser()
{
    if (!m_verificationUri.isEmpty())
        QDesktopServices::openUrl(QUrl(m_verificationUri));
}

} // namespace Labs
