#include "PluginHost.h"
#include "IPlugin.h"

#include <QDir>
#include <QLibrary>

namespace Labs {

using CreatePluginFn = IPlugin* (*)();

PluginHost::PluginHost(QObject* parent) : QObject(parent) {}

PluginHost::~PluginHost() { unloadAll(); }

int PluginHost::loadAll(const QString& pluginDir)
{
    QDir dir(pluginDir);
    if (!dir.exists()) return 0;

    const auto entries = dir.entryInfoList({"*.dll"}, QDir::Files);
    int loaded = 0;
    for (const auto& info : entries) {
        // Heap-allocate. The QLibrary must outlive the IPlugin* so the DLL's
        // code/vtable stays mapped. A stack-local lib gets eagerly unloaded
        // by Qt when it goes out of scope here, which would dangle every
        // plugin pointer.
        QLibrary* lib = new QLibrary(info.absoluteFilePath());
        if (!lib->load()) {
            qWarning().nospace() << "PluginHost: failed to load " << info.fileName()
                                  << " — " << lib->errorString();
            delete lib;
            continue;
        }

        auto fn = reinterpret_cast<CreatePluginFn>(lib->resolve("createPlugin"));
        if (!fn) {
            qWarning().nospace() << "PluginHost: " << info.fileName()
                                  << " has no createPlugin export";
            lib->unload();
            delete lib;
            continue;
        }

        IPlugin* p = nullptr;
        try {
            p = fn();
        } catch (const std::exception& e) {
            qWarning().nospace() << "PluginHost: " << info.fileName()
                                  << " threw during createPlugin: " << e.what();
            lib->unload();
            delete lib;
            continue;
        } catch (...) {
            qWarning().nospace() << "PluginHost: " << info.fileName()
                                  << " threw unknown exception during createPlugin";
            lib->unload();
            delete lib;
            continue;
        }
        if (p) {
            m_records.push_back(Record{ p, lib });
            m_plugins.push_back(p);
            ++loaded;
        } else {
            // createPlugin returned null — drop the DLL too, no client.
            lib->unload();
            delete lib;
        }
    }
    return loaded;
}

void PluginHost::unloadAll()
{
    // Reverse order: tear down later-loaded plugins first. Then for each
    // record, destroy the plugin object (its dtor uses code in the DLL),
    // and ONLY THEN unload the DLL. Destroying after unload would jump
    // into an unmapped page.
    for (int i = m_records.size() - 1; i >= 0; --i) {
        Record& r = m_records[i];
        if (r.plugin) {
            try { r.plugin->shutdown(); }
            catch (...) { /* swallow — keep tearing down */ }
            delete r.plugin;
            r.plugin = nullptr;
        }
        if (r.lib) {
            r.lib->unload();
            delete r.lib;
            r.lib = nullptr;
        }
    }
    m_records.clear();
    m_plugins.clear();
}

} // namespace Labs
