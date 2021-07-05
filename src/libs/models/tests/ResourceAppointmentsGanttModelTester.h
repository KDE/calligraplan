/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_ResourceModelTester_h
#define KPlato_ResourceModelTester_h

#include <QObject>

#include "kptresourceappointmentsmodel.h"

#include "kptproject.h"
#include "kptdatetime.h"

namespace KPlato
{

class Task;

class ResourceModelTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    
    void internalAppointments();
    void externalAppointments();
    void externalOverbook();

private:
    void printDebug(long id) const;
    void printSchedulingLog(const ScheduleManager &sm) const;

    Project *m_project;
    Calendar *m_calendar;
    Task *m_task;
    Resource *m_resource;

    ResourceAppointmentsGanttModel m_model;

};

} //namespace KPlato

#endif
