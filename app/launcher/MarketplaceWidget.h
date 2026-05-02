#pragma once

#include <QWidget>
#include <QString>
#include <QVector>

class QGridLayout;
class QLineEdit;
class QComboBox;
class QPushButton;
class QScrollArea;
class QLabel;
class QNetworkAccessManager;

namespace Labs {

class SettingsManager;

struct MarketScript {
    QString name;
    QString description;
    QString type;          // py, gpc3, ennx, henv, etc.
    QString author;
    QString version;
    QString thumbnailUrl;  // http(s) or file:
    QString downloadUrl;   // http(s) → script file
    QString localFilename; // resolved name in cv-scripts/ once installed
    qint64  likes      = 0;
    qint64  downloads  = 0;
    bool    isLocal    = false;  // true if it lives only in the local manifest (not yet published)
    bool    isInstalled = false; // true if downloadUrl already saved to cv-scripts/
};

class MarketplaceWidget : public QWidget {
    Q_OBJECT
public:
    explicit MarketplaceWidget(SettingsManager* settings, QWidget* parent = nullptr);
    ~MarketplaceWidget() override;

    QString cvScriptsDir() const;

signals:
    void runRequested(const QString& scriptPath);   // user clicked "Run" on an installed card
    void scriptListChanged();                       // installed scripts changed; engine should refresh left rail

public slots:
    void refresh();           // reload manifest + redraw grid
    void onPublishClicked();  // opens publish-form dialog

private slots:
    void onSearchChanged(const QString& text);
    void onFilterChanged();

private:
    void loadManifest();
    void saveLocalManifest();
    void rebuildGrid();
    QWidget* buildCard(const MarketScript& s, int index);
    void installScript(int index);
    void runScript(int index);

    SettingsManager*       m_settings = nullptr;
    QNetworkAccessManager* m_nam = nullptr;

    QLineEdit*    m_search   = nullptr;
    QComboBox*    m_typeBox  = nullptr;
    QComboBox*    m_sortBox  = nullptr;
    QPushButton*  m_publish  = nullptr;
    QPushButton*  m_refresh  = nullptr;
    QScrollArea*  m_scroll   = nullptr;
    QGridLayout*  m_grid     = nullptr;
    QLabel*       m_emptyLbl = nullptr;

    QVector<MarketScript> m_all;
    QString m_filterText;
    QString m_filterType;
    QString m_filterSort;
};

} // namespace Labs
