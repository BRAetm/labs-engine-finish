#include "WebView2Widget.h"

#include <QCloseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QDebug>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
#include "WebView2.h"

#include <atomic>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace Labs {

struct WebView2Widget::Impl {
    QString                              userDataFolder;
    QString                              initialUrl;
    QString                              userAgent;
    ComPtr<ICoreWebView2Controller>      controller;
    ComPtr<ICoreWebView2>                webview;
    ComPtr<ICoreWebView2Environment>     environment;
    HWND                                 hwnd = nullptr;
    std::atomic<bool>                    ready { false };
    std::atomic<bool>                    creating { false };
    QString                              pendingNavigate;  // queued nav before ready
    bool                                 gamepadEnabled = true;  // visual-only flag for now
};

WebView2Widget::WebView2Widget(const QString& userDataFolder,
                               const QString& initialUrl,
                               const QString& userAgent,
                               QWidget* parent)
    : QWidget(parent), m_impl(new Impl)
{
    m_impl->userDataFolder = userDataFolder;
    m_impl->initialUrl     = initialUrl;
    m_impl->userAgent      = userAgent;

    // Force a native HWND for the widget so WebView2 has something to anchor to.
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setAttribute(Qt::WA_PaintOnScreen, false);
    setStyleSheet(QStringLiteral("background:#070A14;"));
    // Small minimum so multi-tile grids fit narrower windows. The WebView2
    // itself rescales to fill whatever the widget actually gets.
    setMinimumSize(220, 140);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

WebView2Widget::~WebView2Widget()
{
    if (m_impl) {
        if (m_impl->controller) {
            try { m_impl->controller->Close(); } catch (...) {}
        }
        delete m_impl;
        m_impl = nullptr;
    }
}

bool WebView2Widget::isReady() const
{
    return m_impl && m_impl->ready.load();
}

void WebView2Widget::navigate(const QString& url)
{
    if (!m_impl) return;
    if (!m_impl->ready.load() || !m_impl->webview) {
        m_impl->pendingNavigate = url;
        return;
    }
    m_impl->webview->Navigate(reinterpret_cast<LPCWSTR>(url.utf16()));
}

void WebView2Widget::setGamepadEnabled(bool enabled)
{
    // No-op for the gamepad gating. Stored only so callers can query the
    // intended state — actual isolation is deferred to per-session ViGEm pads.
    if (!m_impl) return;
    m_impl->gamepadEnabled = enabled;
}

void WebView2Widget::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    if (!m_impl) return;
    if (m_impl->ready.load() || m_impl->creating.exchange(true)) return;

    m_impl->hwnd = reinterpret_cast<HWND>(winId());
    if (!m_impl->hwnd) {
        qWarning() << "[WebView2Widget] no HWND on show";
        m_impl->creating.store(false);
        return;
    }

    // Resolve user-data folder to a wide string. WebView2 takes a writable
    // directory; cookies, cache, login state all live here. Per account.
    const std::wstring udf = m_impl->userDataFolder.toStdWString();

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        udf.c_str(),
        nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env || !m_impl) {
                    qWarning() << "[WebView2Widget] env create failed hr=" << QString::number(result, 16);
                    if (m_impl) m_impl->creating.store(false);
                    return result;
                }
                m_impl->environment = env;
                return env->CreateCoreWebView2Controller(
                    m_impl->hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT r, ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (FAILED(r) || !ctrl || !m_impl) {
                                qWarning() << "[WebView2Widget] controller create failed hr=" << QString::number(r, 16);
                                if (m_impl) m_impl->creating.store(false);
                                return r;
                            }
                            m_impl->controller = ctrl;
                            ComPtr<ICoreWebView2> wv;
                            ctrl->get_CoreWebView2(&wv);
                            m_impl->webview = wv;

                            // Apply UA override if given.
                            if (!m_impl->userAgent.isEmpty()) {
                                ComPtr<ICoreWebView2_2> wv2;
                                if (SUCCEEDED(wv.As(&wv2))) {
                                    ComPtr<ICoreWebView2Settings> s;
                                    wv->get_Settings(&s);
                                    ComPtr<ICoreWebView2Settings2> s2;
                                    if (SUCCEEDED(s.As(&s2))) {
                                        s2->put_UserAgent(reinterpret_cast<LPCWSTR>(m_impl->userAgent.utf16()));
                                    }
                                }
                            }

                            // Bound the WebView2 to the current widget rect.
                            RECT rc; GetClientRect(m_impl->hwnd, &rc);
                            m_impl->controller->put_Bounds(rc);
                            m_impl->controller->put_IsVisible(TRUE);

                            // No JS gamepad gating. The shim approach was
                            // fragile (WebView2 async timing + xCloud's
                            // gamepad detection didn't react to synthetic
                            // disconnect events reliably). Controllers work
                            // in all open tiles for now. Per-tile isolation
                            // gets done properly via per-session virtual
                            // ViGEm pads later — the tile focus button
                            // stays as a visual indicator until then.
                            m_impl->ready.store(true);
                            m_impl->creating.store(false);

                            // Initial navigation. Honors a queued navigate() if one came
                            // in while we were async-creating.
                            const QString nav = !m_impl->pendingNavigate.isEmpty()
                                                ? m_impl->pendingNavigate
                                                : m_impl->initialUrl;
                            if (!nav.isEmpty()) {
                                wv->Navigate(reinterpret_cast<LPCWSTR>(nav.utf16()));
                            }
                            return S_OK;
                        }
                    ).Get());
            }
        ).Get());

    if (FAILED(hr)) {
        qWarning() << "[WebView2Widget] CreateCoreWebView2EnvironmentWithOptions failed hr=" << QString::number(hr, 16);
        m_impl->creating.store(false);
    }
}

void WebView2Widget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    if (m_impl && m_impl->controller && m_impl->hwnd) {
        RECT rc; GetClientRect(m_impl->hwnd, &rc);
        m_impl->controller->put_Bounds(rc);
    }
}

void WebView2Widget::closeEvent(QCloseEvent* e)
{
    if (m_impl && m_impl->controller) {
        try { m_impl->controller->Close(); } catch (...) {}
        m_impl->controller.Reset();
        m_impl->webview.Reset();
        m_impl->ready.store(false);
    }
    QWidget::closeEvent(e);
}

} // namespace Labs
