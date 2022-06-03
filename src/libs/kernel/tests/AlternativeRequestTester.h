/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_AlternativeRequestTester_h
#define KPlato_AlternativeRequestTester_h

#include <QObject>

namespace KPlato
{

class Project;
class Task;
class Resource;
class ScheduleManager;

class AlternativeRequestTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void scheduleForwardUnavailable();
    void scheduleForwardBooked();

private:
    Project *project;
    Task *t1;
    Task *t2;
    Resource *r1;
    Resource *r2;
    ScheduleManager *manager;
};

} //namespace KPlato

#endif
