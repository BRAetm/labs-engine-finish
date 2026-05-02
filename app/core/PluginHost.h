#pragma once

#include "LabsCore.h"

#include <QObject>
#include <QString>
#include <QVector>

class QLibrary;

namespace Labs {

class IPlugin;

class LABSCORE_API PluginHost : public QObject {
    Q_OBJECT
public:
    explicit PluginHost(QObject* parent = nullptr);
    ~PluginHost() override;

    int loadAll(const QString& pluginDir);
    void unloadAll();

    const QVector<IPlugin*>& plugins() const { return m_plugins; }

private:
    // Heap-allocated QLibrary per plugin. We must keep the QLibrary alive
    // for the entire lifetime of the IPlugin* — the plugin's vtable + code
    // live in the DLL. A stack-local QLibrary in loadAll() would let Qt
    // unload the DLL eagerly, leaving every plugin pointer dangling.
    struct Record {
        IPlugin*  plugin = nullptr;
        QLibrary* lib    = nullptr;
    };

    QVector<Record>   m_records;
    QVector<IPlugin*> m_plugins;  // mirrors m_records[i].plugin for plugins() accessor
};

} // namespace Labs
