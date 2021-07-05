/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_TaskJuggler_h
#define KPlato_TaskJuggler_h

#include <QObject>

namespace TJ {
    class Project;
}

namespace KPlato
{

class TaskJuggler : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void list();
    void projectTest();
    void oneTask();
    void oneResource();
    void allocation();
    void dependency();
    void scheduleResource();

    void scheduleDependencies();
    void scheduleConstraints();
    void resourceConflict();
    void units();

private:
    TJ::Project *project;
};

} //namespace KPlato

#endif
