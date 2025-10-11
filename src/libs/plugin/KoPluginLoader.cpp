/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2007 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2016 Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "KoPluginLoader.h"

#include <KConfig>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KPluginMetaData>

#include <QCoreApplication>
#include <QJsonObject>
#include <QPluginLoader>
#include <QLoggingCategory>
#include <QDebug>


const QLoggingCategory &PLUGIN_LOG()
{
    static const QLoggingCategory category("calligra.lib.plugin");
    return category;
}

#define debugPlugin qCDebug(PLUGIN_LOG)
#define warnPlugin qCWarning(PLUGIN_LOG)


class KoPluginLoaderImpl : public QObject
{
Q_OBJECT
public:
    QStringList loadedDirectories;
};



Q_GLOBAL_STATIC(KoPluginLoaderImpl, pluginLoaderInstance)


void KoPluginLoader::load(const QString & directory, const PluginsConfig &config, QObject* owner)
{
    // Don't load the same plugins again
    if (pluginLoaderInstance->loadedDirectories.contains(directory)) {
        return;
    }
    pluginLoaderInstance->loadedDirectories << directory;

    const QList<QPluginLoader *> offers = KoPluginLoader::pluginLoaders(directory);
    QList<QPluginLoader *> plugins;
    bool configChanged = false;
    QList<QString> blacklist; // what we will save out afterwards
    if (config.whiteList && config.blacklist && config.group) {
        debugPlugin << "Loading" << directory << "with checking the config";
        KConfigGroup configGroup(KSharedConfig::openConfig(), QString::fromLatin1(config.group));
        QList<QString> whiteList = configGroup.readEntry(config.whiteList, config.defaults);
        QList<QString> knownList;

        // if there was no list of defaults; all plugins are loaded.
        const bool firstStart = !config.defaults.isEmpty() && !configGroup.hasKey(config.whiteList);
        knownList = configGroup.readEntry(config.blacklist, knownList);
        if (firstStart) {
            configChanged = true;
        }
        for (QPluginLoader *loader : offers) {
            QJsonObject json = loader->metaData().value(QStringLiteral("MetaData")).toObject();
            json = json.value(QStringLiteral("KPlugin")).toObject();
            const QString pluginName = json.value(QStringLiteral("Id")).toString();
            if (pluginName.isEmpty()) {
                warnPlugin << "Loading plugin" << loader->fileName() << "failed, has no X-KDE-PluginInfo-Name.";
                continue;
            }
            if (whiteList.contains(pluginName)) {
                plugins.append(loader);
            } else if (!firstStart && !knownList.contains(pluginName)) { // also load newly installed plugins.
                plugins.append(loader);
                configChanged = true;
            } else {
                blacklist << pluginName;
            }
        }
    } else {
        plugins = offers;
    }

    QMap<QString, QPluginLoader *> serviceNames;
    for (QPluginLoader *loader : std::as_const(plugins)) {
        if (serviceNames.contains(loader->fileName())) { // duplicate
            QJsonObject json2 = loader->metaData().value(QStringLiteral("MetaData")).toObject();
            QVariant pluginVersion2 = json2.value(QStringLiteral("X-Flake-PluginVersion")).toVariant();
            if (pluginVersion2.isNull()) { // just take the first one found...
                continue;
            }
            QPluginLoader *currentLoader = serviceNames.value(loader->fileName());
            QJsonObject json = currentLoader->metaData().value(QStringLiteral("MetaData")).toObject();
            QVariant pluginVersion = json.value(QStringLiteral("X-Flake-PluginVersion")).toVariant();
            if (!(pluginVersion.isNull() || pluginVersion.toInt() < pluginVersion2.toInt())) {
                continue; // replace the old one with this one, since its newer.
            }
        }
        serviceNames.insert(loader->fileName(), loader);
    }

    QList<QString> whiteList;
    const QList<QPluginLoader*> loaders = serviceNames.values();
    for (QPluginLoader *loader : loaders) {
        KPluginFactory *factory = qobject_cast<KPluginFactory *>(loader->instance());
        QObject *plugin = factory->create<QObject>(owner ? owner : pluginLoaderInstance, QVariantList());
        if (plugin) {
            QJsonObject json = loader->metaData().value(QStringLiteral("MetaData")).toObject();
            json = json.value(QStringLiteral("KPlugin")).toObject();
            const QString pluginName = json.value(QStringLiteral("Id")).toString();
            whiteList << pluginName;
            debugPlugin << "Loaded plugin" << loader->fileName() << owner;
            if (!owner) {
                delete plugin;
            }
        } else {
            warnPlugin << "Loading plugin" << loader->fileName() << "failed, " << loader->errorString();
        }
    }

    if (configChanged && config.whiteList && config.blacklist && config.group) {
        KConfigGroup configGroup(KSharedConfig::openConfig(), QString::fromLatin1(config.group));
        configGroup.writeEntry(config.whiteList, whiteList);
        configGroup.writeEntry(config.blacklist, blacklist);
    }

    qDeleteAll(offers);
}

QList<KPluginFactory *> KoPluginLoader::instantiatePluginFactories(const QString & directory)
{
    QList<KPluginFactory *> pluginFactories;

    const QList<QPluginLoader *> offers = KoPluginLoader::pluginLoaders(directory);

    for (QPluginLoader *pluginLoader : offers) {
        QObject* pluginInstance = pluginLoader->instance();
        if (!pluginInstance) {
            warnPlugin << "Loading plugin" << pluginLoader->fileName() << "failed, " << pluginLoader->errorString();
            continue;
        }
        KPluginFactory *factory = qobject_cast<KPluginFactory *>(pluginInstance);
        if (factory == nullptr) {
            warnPlugin << "Expected a KPluginFactory, got a" << pluginInstance->metaObject()->className();
            delete pluginInstance;
            continue;
        }

        pluginFactories.append(factory);
    }
    qDeleteAll(offers);

    return pluginFactories;
}

QList<QPluginLoader *> KoPluginLoader::pluginLoaders(const char *directory, const QString &mimeType)
{
    return pluginLoaders(QString::fromLatin1(directory), mimeType);
}

QList<QPluginLoader *> KoPluginLoader::pluginLoaders(const QString &directory, const QString &mimeType)
{
    QList<QPluginLoader *>list;
    const QVector<KPluginMetaData> plugins = KPluginMetaData::findPlugins(directory);
    for (const auto &metaData : plugins) {
        debugPlugin << "Trying to load" << metaData.fileName();
        if (!mimeType.isEmpty()) {
            QStringList mimeTypes = metaData.mimeTypes();
            mimeTypes += metaData.value(QStringLiteral("X-KDE-ExtraNativeMimeTypes")).split(QLatin1Char(':'));
            mimeTypes += metaData.value(QStringLiteral("X-KDE-NativeMimeType"));
            if (!mimeTypes.contains(mimeType)) {
                continue;
            }
        }
        list.append(new QPluginLoader(metaData.fileName()));
    }
    return list;
}
#include "KoPluginLoader.moc"
