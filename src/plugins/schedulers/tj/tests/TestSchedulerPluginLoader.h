/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef TESTSCHEDULERPLUGINLOADER_H
#define TESTSCHEDULERPLUGINLOADER_H

#include <QObject>
 
#include "kptschedulerplugin_macros.h"
 

/// The main namespace.
namespace KPlato
{

class SchedulerPlugin;

class TestSchedulerPluginLoader : public QObject
{
    Q_OBJECT
public:
    explicit TestSchedulerPluginLoader(QObject * parent);
    ~TestSchedulerPluginLoader() override;

    SchedulerPlugin *loadPlugin(const QString &dir, const QString &name);
};

} //namespace KPlato

#endif
