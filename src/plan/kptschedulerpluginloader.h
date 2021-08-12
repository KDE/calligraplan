/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTSCHEDULERPLUGINLOADER_H
#define KPTSCHEDULERPLUGINLOADER_H

#include <QObject>
 
/// The main namespace.
namespace KPlato
{

class SchedulerPlugin;

class SchedulerPluginLoader : public QObject
{
    Q_OBJECT
public:
    explicit SchedulerPluginLoader(QObject * parent);
    ~SchedulerPluginLoader() override;

    void loadAllPlugins();

Q_SIGNALS:
    void pluginLoaded(const QString &key, KPlato::SchedulerPlugin *plugin);
};

} //namespace KPlato

#endif
