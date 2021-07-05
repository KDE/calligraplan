/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_CalendarTester_h
#define KPlato_CalendarTester_h

#include <QObject>

namespace KPlato
{

class CalendarTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSingleDay();
    void testWeekdays();
    void testCalendarWithParent();
    void testTimezone();
    void workIntervals();
    void workIntervalsFullDays();
    void dstSpring();
};

} //namespace KPlato

#endif
