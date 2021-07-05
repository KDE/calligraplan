/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_PerformanceTester_h
#define KPlato_PerformanceTester_h

#include <QObject>

#include "kptdatetime.h"
#include "kptproject.h"
#include "kptduration.h"

namespace KPlato
{
class Task;

class PerformanceTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();
    void bcwsPrDayTask();
    void bcwpPrDayTask();
    void acwpPrDayTask();
    void bcwsMilestone();
    void bcwpMilestone();
    void acwpMilestone();

    void bcwsPrDayTaskMaterial();
    void bcwpPrDayTaskMaterial();
    void acwpPrDayTaskMaterial();

    void bcwsPrDayProject();
    void bcwpPrDayProject();
    void acwpPrDayProject();

private:
    Project *p1;
    Resource *r1;
    Resource *r2; // material
    Resource *r3; // material
    Task *s1;
    Task *t1;
    Task *s2;
    Task *m1;
};

} //namespace KPlato

#endif
