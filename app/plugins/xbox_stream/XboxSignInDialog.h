#pragma once

#include <QDialog>
#include <QByteArray>
#include <QString>

class QLabel;
class QProcess;
class QPushButton;

namespace Labs {

// Shows the Microsoft device-code sign-in flow. Spawns auth.js via
// "electron.exe --run-as-node" (ELECTRON_RUN_AS_NODE=1), parses the JSON
// progress lines it prints on stdout, and updates the UI.
//
// On success, tokens are persisted to `tokensFile` (chosen by the caller —
// typically per-user AppData/Local). Dialog accepts.

class XboxSignInDialog : public QDialog {
    Q_OBJECT
public:
    XboxSignInDialog(const QString& electronExe,
                     const QString& sidecarDir,
                     const QString& tokensFile,
                     QWidget* parent = nullptr);
    ~XboxSignInDialog() override;

private slots:
    void onStdout();
    void onStderr();
    void onFinished(int exitCode);
    void onCopyCode();
    void onOpenBrowser();

private:
    void setStatus(const QString& text);
    void handleProgressLine(const QByteArray& line);

    QLabel*      m_statusLabel = nullptr;
    QLabel*      m_urlLabel    = nullptr;
    QLabel*      m_codeLabel   = nullptr;
    QPushButton* m_copyBtn     = nullptr;
    QPushButton* m_openBtn     = nullptr;
    QPushButton* m_cancelBtn   = nullptr;

    QProcess*    m_proc        = nullptr;
    QByteArray   m_stdoutBuf;

    QString      m_verificationUri;
    QString      m_userCode;
};

} // namespace Labs
