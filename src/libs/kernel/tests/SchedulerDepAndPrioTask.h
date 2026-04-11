/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2026 Mickael Sergent <miko53@free.fr>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATO_SCHEDULERDEPANDPRIOTASK_H
#define KPLATO_SCHEDULERDEPANDPRIOTASK_H

#include <QObject>
#include "kptproject.h"
#include "Resource.h"

namespace KPlato
{
class Task;

class SchedulerDepAndPrioTask : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();
    void test1ScheduleWithDependsInForward();
    void test2ScheduleWithDependsAndPriorityInForward();
    void test3ScheduleWithDependsAndPriorityInForward();
    void test4ScheduleWithDependsInBackward();
    void test5ScheduleWithDependsAndPriorityInBackward();
    void test6ScheduleWithDependsAndPriorityInBackward();

private:
    Project *p1;
    Resource *m;
    Resource *n;
    Task *t1;
    Task *ta;
    Task *tb;
    Task *tc;
    Task *t2;
    Task *td;
    Task *te;
    Task *tf;
};

} //namespace KPlato
#endif /* KPLATO_SCHEDULERDEPANDPRIOTASK_H */
