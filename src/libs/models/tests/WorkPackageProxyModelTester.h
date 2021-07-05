/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_WorkPackageProxyModelTester_h
#define KPlato_WorkPackageProxyModelTester_h

#include <QObject>

#include "kptworkpackagemodel.h"
#include "kptproject.h"

namespace KPlato
{

class Task;
class ScheduleManager;

class WorkPackageProxyModelTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testInsert();
    void testRemove();
    void testMove();
    void testMoveChild();

private:
    Task *addTask(Node *parent, int after);
    void removeTask(Node *task);
    void moveTask(Node *task, Node *newParent, int newPos);

private:
    WorkPackageProxyModel m_model;
    Project project;
    ScheduleManager *sm;

};

} //namespace KPlato

#endif
