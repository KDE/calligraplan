/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPLATO_RESOURCEMODELTESTER_H
#define KPLATO_RESOURCEMODELTESTER_H

#include <QObject>

#include "ResourceItemModel.h"

namespace KPlato
{

class Resource;
class Calendar;
class Project;

class ResourceModelTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void resources();
    void groups();
    void teams();
    void groupsAndTeams();

private:
    QModelIndex index(Resource *r);

    Project *m_project;
    Calendar *m_calendar;
    Resource *m_resource;

    ResourceItemModel *m_model;

};

} //namespace KPlato

#endif
