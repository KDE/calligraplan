/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_AppointmentIntervalTester_h
#define KPlato_AppointmentIntervalTester_h

#include <QObject>

namespace KPlato
{

class AppointmentIntervalTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void interval();
    void addInterval();
    void addAppointment();
    void addTangentIntervals();
    void subtractList();
    void subtractListMidnight();

};

}

#endif
