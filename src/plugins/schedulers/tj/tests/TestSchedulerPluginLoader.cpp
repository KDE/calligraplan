/* This file is part of the KDE project
 * Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// clazy:excludeall=qstring-arg
#include "TestSchedulerPluginLoader.h"
 
#include "kptschedulerplugin.h"
#include "kptdebug.h"

#include <KoPluginLoader.h>

#include <QPluginLoader>
#include <QLocale>


namespace KPlato
{

TestSchedulerPluginLoader::TestSchedulerPluginLoader(QObject * parent)
  : QObject(parent)
{
}
 
TestSchedulerPluginLoader::~TestSchedulerPluginLoader()
{
    qInfo()<<Q_FUNC_INFO;
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


SchedulerPlugin *TestSchedulerPluginLoader::loadPlugin(const QString &dir, const QString &name_)
{
    debugPlan << "Load plugin:"<<dir<<name_;
    SchedulerPlugin *schedulerPlugin = nullptr;

    const QList<QPluginLoader *> offers = KoPluginLoader::pluginLoaders(dir);

    for(QPluginLoader *pluginLoader : offers) {
        KPluginFactory *factory = qobject_cast<KPluginFactory*>(pluginLoader->instance());
 
        if (!factory)
        {
            errorPlan << "KPluginFactory could not load the plugin:" << pluginLoader->fileName();
            continue;
        }
 
        QJsonObject json = pluginLoader->metaData().value("MetaData").toObject();
        json = json.value("KPlugin").toObject();
        const QString key = json.value(QLatin1String("Name")).toString(); // use unlocalized name as plugin identifier
        const QString name = readLocalValue(json, QLatin1String("Name")).toString();
        const QString comment = readLocalValue(json, QLatin1String("Description")).toString();

        if (key == name_) {
            debugPlan << "Load plugin:" << key << name << ", " << comment;
            schedulerPlugin = factory->create<SchedulerPlugin>(this);
            Q_ASSERT(schedulerPlugin);
            schedulerPlugin->setName(name);
            schedulerPlugin->setComment(comment);
            break;
        }
    }
    qDeleteAll(offers);
    return schedulerPlugin;
}

} //namespace KPlato
