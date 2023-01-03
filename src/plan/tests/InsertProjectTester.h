/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_InsertProjectTester_h
#define KPlato_InsertProjectTester_h

#include <QObject>

namespace KPlato
{

class MainDocument;
class Account;
class ResourceGroup;
class Resource;
class Task;
class Relation;
class Calendar;

class InsertProjectTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testAccount();
    void testCalendar();
    void testDefaultCalendar();
    void testResourceGroup();
    void testResource();
    void testTeamResource();
    void testResourceAccount();
    void testResourceCalendar();
    void testTask();
    void testResourceRequest();
    void testTeamResourceRequest();
    void testDependencies();
    void testExistingResourceAccount();
    void testExistingResourceCalendar();
    void testExistingResourceRequest();
    void testExistingRequiredResourceRequest();
    void testExistingTeamResourceRequest();

private:
    Account *addAccount(MainDocument &part, Account *parent = nullptr, const QString &name = QString());
    Calendar *addCalendar(MainDocument &part, Calendar *parent = nullptr, const QString &id = QString());
    ResourceGroup *addResourceGroup(MainDocument &part, ResourceGroup *parent = nullptr, const QString &id = QString());
    Resource *addResource(MainDocument &part, ResourceGroup *g = nullptr);
    Task *addTask(MainDocument &part);
    void addResourceRequest(MainDocument &part);
    Relation *addDependency(MainDocument &part, Task *t1, Task *t2);
};

}

#endif
