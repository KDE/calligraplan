/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_ReScheduleTester_h
#define KPlato_ReScheduleTester_h

#include <QTest>

namespace KPlato
{
class Project;
class Resource;

class ReScheduleTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void completionPerTask();
    void completionPerResource();
    void reschedulePerTask();
    void reschedulePerResource();
    void rescheduleTaskLength();

private:
    Project *m_project;
    Resource *resource1;
    Resource *resource2;
};

} //namespace KPlato

#endif
