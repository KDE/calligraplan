/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPLATO_RESOURCEGROUPMODELTESTER_H
#define KPLATO_RESOURCEGROUPMODELTESTER_H

#include <QObject>

#include "ResourceGroupItemModel.h"

#include "kptproject.h"
#include "kptdatetime.h"

namespace KPlato
{

class Task;

class ResourceGroupModelTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();
    
    void groups();
    void childGroups();
    void resources();

private:
    Project *m_project;
    Calendar *m_calendar;
    Task *m_task;
    Resource *m_resource;

    ResourceGroupItemModel m_model;

};

} //namespace KPlato

#endif
