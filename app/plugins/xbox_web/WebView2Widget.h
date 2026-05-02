#pragma once

#include <QWidget>
#include <QString>

namespace Labs {

// Single QWidget that hosts a Microsoft Edge WebView2 control via the native
// WebView2 SDK. The WebView2 binds to the widget's HWND on first show and
// resizes with the widget. One WebView2 per QWidget — for multi-session,
// instantiate multiple of these (each with its own user-data folder so
// cookies / sign-in state are isolated per account).
class WebView2Widget : public QWidget {
    Q_OBJECT
public:
    // userDataFolder: per-account profile directory (cookies, cache, login state).
    //                 Created if missing; isolating accounts means one folder each.
    // initialUrl:     URL to navigate to when the WebView2 finishes initialising.
    // userAgent:      override UA string (optional). Leave empty for default.
    explicit WebView2Widget(const QString& userDataFolder,
                            const QString& initialUrl,
                            const QString& userAgent = QString(),
                            QWidget* parent = nullptr);
    ~WebView2Widget() override;

    bool isReady() const;
    void navigate(const QString& url);

    // Gate the page's `navigator.getGamepads()` result. When false, returns
    // an empty list — the xCloud page sees no gamepad and ignores input.
    // When true (the default after construction), the real gamepad list passes
    // through unchanged. Used to make controller input follow tile focus.
    void setGamepadEnabled(bool enabled);

protected:
    void showEvent(QShowEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private:
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace Labs
