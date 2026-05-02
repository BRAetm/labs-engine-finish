#pragma once

#include <QMainWindow>
#include <QPointer>

class QLabel;
class QPushButton;
class QStackedWidget;
class QWidget;

namespace Labs {

class SettingsManager;
class MarketplaceWidget;
class SidebarEntry;

// Launcher-library layout (Steam / 2K Launcher style):
//   ┌──────────┬─────────────────────────────────┐
//   │ sidebar  │ hero (one of N stacked pages)   │
//   │ - Engine │  big title + description +      │
//   │ - Zen    │  action button OR embedded view │
//   │ - Market │                                 │
//   └──────────┴─────────────────────────────────┘
//
// Sidebar entries are clickable rows with a small tinted Labs glyph + name.
// Selection drives a QStackedWidget in the hero area.
class HubWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit HubWindow(QWidget* parent = nullptr);
    ~HubWindow() override;

private slots:
    void onLaunchLabsEngine();
    void selectApp(int index);

private:
    QWidget* buildSidebar();
    QWidget* buildHeroStack();
    QWidget* buildAppHero(const QString& title,
                          const QString& subtitle,
                          const QString& description,
                          const QString& actionText,
                          const QString& badge,
                          bool enabled,
                          const QString& accentHex,
                          bool nativeBrand,
                          std::function<void()> onAction);
    QWidget* buildMarketplaceHero();

    QString resolveLabsEngineExe() const;

    SettingsManager*           m_settings   = nullptr;
    QStackedWidget*            m_heroStack  = nullptr;
    QList<SidebarEntry*>       m_sidebarEntries;
    int                        m_selectedIndex = 0;
    QPointer<MarketplaceWidget> m_marketplace;
};

} // namespace Labs
