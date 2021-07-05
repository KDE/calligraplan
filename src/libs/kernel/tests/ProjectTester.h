/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_ProjectTester_h
#define KPlato_ProjectTester_h

#include <QObject>

#include "kptproject.h"
#include "kptdatetime.h"

namespace KPlato
{

class Task;

class ProjectTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testAddTask();
    void testTakeTask();
    void testTaskAddCmd();
    void testTaskDeleteCmd();

    void schedule();
    void scheduleFullday();
    void scheduleFulldayDstSpring();
    void scheduleFulldayDstFall();

    void scheduleWithExternalAppointments();

    void reschedule();

    void materialResource();
    void requiredResource();

    void resourceWithLimitedAvailability();
    void unavailableResource();
    void team();
    // NOTE: It's not *mandatory* to schedule in wbs order but users expect it, so we'll try.
    // This test can be removed if for some important reason this isn't possible.
    void inWBSOrder();

    void resourceConflictALAP();
    void resourceConflictMustStartOn();
    void resourceConflictMustFinishOn();
    void fixedInterval();
    void estimateDuration();

    void startStart();

    void scheduleTimeZone();
    
private:
    Project *m_project;
    Calendar *m_calendar;
    Task *m_task;
    QByteArray tz;
};

} //namespace KPlato

#endif
