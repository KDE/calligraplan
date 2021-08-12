/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009, 2012 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
*/


// clazy:excludeall=qstring-arg
#include "kptschedulerpluginloader.h"
 
#include "kptschedulerplugin.h"
#include "kptdebug.h"

#include <KoPluginLoader.h>

#include <KPluginFactory>
#include <QPluginLoader>
#include <QLocale>


namespace KPlato
{

SchedulerPluginLoader::SchedulerPluginLoader(QObject * parent)
  : QObject(parent)
{
}
 
SchedulerPluginLoader::~SchedulerPluginLoader()
{
}

static
QJsonValue readLocalValue(const QJsonObject &json, const QString &key)
{
    // start with language_country
    const QString localeName = QLocale().name();

    QString localKey = key + QLatin1Char('[') + localeName + QLatin1Char(']');
    QJsonObject::ConstIterator it = json.constFind(localKey);
    if (it != json.constEnd()) {
        return it.value();
    }

    // drop _country
    const int separatorIndex = localeName.indexOf(QLatin1Char('_'));
    if (separatorIndex != -1) {
        const int localKeySeparatorIndex = key.length() + 1 + separatorIndex;
        localKey[localKeySeparatorIndex] = QLatin1Char(']');
        localKey.truncate(localKeySeparatorIndex + 1);
       it = json.constFind(localKey);
        if (it != json.constEnd()) {
            return it.value();
        }
    }

    // default to unlocalized value
    return json.value(key);
}


void SchedulerPluginLoader::loadAllPlugins()
{
    debugPlan << "Load all plugins";
    const QList<QPluginLoader *> offers = KoPluginLoader::pluginLoaders(QStringLiteral("calligraplan/schedulers"));

    for(QPluginLoader *pluginLoader : offers) {
        KPluginFactory *factory = qobject_cast<KPluginFactory*>(pluginLoader->instance());
 
        if (!factory)
        {
            errorPlan << "KPluginFactory could not load the plugin:" << pluginLoader->fileName();
            continue;
        }
 
        SchedulerPlugin *plugin = factory->create<SchedulerPlugin>(this);
 
        if (plugin) {
            QJsonObject json = pluginLoader->metaData().value("MetaData").toObject();
            json = json.value("KPlugin").toObject();
            const QString key = json.value(QLatin1String("Name")).toString(); // use unlocalized name as plugin identifier
            const QString name = readLocalValue(json, QLatin1String("Name")).toString();
            const QString comment = readLocalValue(json, QLatin1String("Description")).toString();

            debugPlan << "Load plugin:" << key << name << ", " << comment;
            plugin->setName(name);
            plugin->setComment(comment);
            Q_EMIT pluginLoaded(key, plugin);
        } else {
           debugPlan << "KPluginFactory could not create SchedulerPlugin:" << pluginLoader->fileName();
        }
    }
    qDeleteAll(offers);
}

} //namespace KPlato
