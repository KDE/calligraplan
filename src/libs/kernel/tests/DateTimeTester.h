/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_DateTimeTester_h
#define KPlato_DateTimeTester_h

#include <QObject>

#include "kptdatetime.h"

namespace KPlato
{

class DateTimeTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void subtractDay();
    void subtractHour();
    void subtractMinute();
    void subtractSecond();
    void subtractMillisecond();

    void addDay();
    void addHour();
    void addMinute();
    void addSecond();
    void addMillisecond();

    void timeZones();
};

} //namespace KPlato

#endif
