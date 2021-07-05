/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_WorkInfoCacheTester_h
#define KPlato_WorkInfoCacheTester_h

#include <QObject>

namespace KPlato
{

class WorkInfoCacheTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void basics();
    void addAfter();
    void addBefore();
    void addMiddle();
    void fullDay();
    void timeZone();
    void doubleTimeZones();
};

} //namespace KPlato

#endif
