/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_AccountsCommandTester_h
#define KPlato_AccountsCommandTester_h

#include <QObject>

namespace KPlato
{

class Task;
class Project;
class Resource;
class Calendar;
class ScheduleManager;

class AccountsCommandTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void addAccount();
    void removeAccount();
    void costPlace();

private:
    void printDebug(long id) const;
    void printSchedulingLog(const ScheduleManager &sm) const;

    Project *m_project;
    Calendar *m_calendar;
    Task *m_task;
    Resource *m_resource;

};

} //namespace KPlato

#endif
