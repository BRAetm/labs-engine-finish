#pragma once

#include <QString>
#include <windows.h>

namespace Labs {

class ProcessMonitor {
public:
    void setPattern(const QString& pattern);
    HWND findWindow() const;

private:
    QString m_pattern;

    static bool matchesGlob(const QString& text, const QString& pattern);

    struct EnumState {
        const ProcessMonitor* self;
        HWND result;
    };
    static BOOL CALLBACK enumProc(HWND hwnd, LPARAM lParam);
};

} // namespace Labs
