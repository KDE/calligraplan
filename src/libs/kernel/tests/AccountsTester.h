/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_AccountsTester_h
#define KPlato_AccountsTester_h

#include "kptproject.h"

namespace KPlato
{

class Task;

class AccountsTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();
    void defaultAccount();
    void costPlaces();

    void startupDefault();
    void startupAccount();
    
    void shutdownAccount();

    void subaccounts();

    void deleteAccount();

private:
    Project *project;
    Task *t;
    Resource *r;
    ScheduleManager *sm;
    Account *topaccount;
    
    QDate today;
    QDate tomorrow;
    QDate yesterday;
    QDate nextweek;
    QTime t1;
    QTime t2;
    int length;
};

}

#endif
