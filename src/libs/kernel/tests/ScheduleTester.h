/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_ScheduleTester_h
#define KPlato_ScheduleTester_h

#include <QObject>
#include <QDate>
#include <QTime>

#include "kptschedule.h"
#include "kptdatetime.h"

namespace KPlato
{


class ScheduleTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    
    void available();
    void busy();

private:
    ResourceSchedule resourceSchedule;
    NodeSchedule nodeSchedule;

    QDate date;
    QTime t1;
    QTime t2;
    QTime t3;

};

} //namespace KPlato

#endif
