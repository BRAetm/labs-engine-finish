#include "ProcessMonitor.h"

#include <psapi.h>
#include <QFileInfo>

namespace Labs {

void ProcessMonitor::setPattern(const QString& pattern)
{
    m_pattern = pattern;
}

HWND ProcessMonitor::findWindow() const
{
    if (m_pattern.isEmpty())
        return nullptr;

    EnumState state{ this, nullptr };
    EnumWindows(enumProc, reinterpret_cast<LPARAM>(&state));
    return state.result;
}

// static
bool ProcessMonitor::matchesGlob(const QString& text, const QString& pattern)
{
    // Classic recursive glob: * matches any sequence, ? matches one char.
    // Both comparisons are case-insensitive (process names on Windows vary).
    const QString t = text.toLower();
    const QString p = pattern.toLower();

    int ti = 0, pi = 0;
    int starPi = -1, starTi = -1;

    while (ti < t.size()) {
        if (pi < p.size() && (p[pi] == t[ti] || p[pi] == QLatin1Char('?'))) {
            ++ti; ++pi;
        } else if (pi < p.size() && p[pi] == QLatin1Char('*')) {
            starPi = pi++;
            starTi = ti;
        } else if (starPi != -1) {
            pi = starPi + 1;
            ti = ++starTi;
        } else {
            return false;
        }
    }
    while (pi < p.size() && p[pi] == QLatin1Char('*'))
        ++pi;
    return pi == p.size();
}

// static
BOOL CALLBACK ProcessMonitor::enumProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd))
        return TRUE;

    auto* state = reinterpret_cast<EnumState*>(lParam);

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid)
        return TRUE;

    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc)
        return TRUE;

    wchar_t buf[MAX_PATH] = {};
    DWORD   len = MAX_PATH;
    bool    matched = false;

    if (QueryFullProcessImageNameW(proc, 0, buf, &len)) {
        QString exeName = QFileInfo(QString::fromWCharArray(buf)).fileName();
        matched = ProcessMonitor::matchesGlob(exeName, state->self->m_pattern);
    }
    CloseHandle(proc);

    if (matched) {
        state->result = hwnd;
        return FALSE;
    }
    return TRUE;
}

} // namespace Labs
