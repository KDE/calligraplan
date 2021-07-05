/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "Schedule.h"

#include "Project.h"

#include <kptschedule.h>

Scripting::Schedule::Schedule(Scripting::Project *project, KPlato::ScheduleManager *schedule, QObject *parent)
    : QObject(parent), m_project(project), m_schedule(schedule)
{
}

qlonglong Scripting::Schedule::id() const
{
    return (qlonglong)m_schedule ? m_schedule->scheduleId() : -1;
}

QString Scripting::Schedule::name() const
{
    return m_schedule ? m_schedule->name() : "";
}

bool Scripting::Schedule::isScheduled() const
{
    return m_schedule ? m_schedule->isScheduled() : false;
}

QDate Scripting::Schedule::startDate()
{
    return QDate(); //m_schedule->startTime().dateTime().date();
}

QDate Scripting::Schedule::endDate()
{
    return QDate(); //m_schedule->endTime().dateTime().date();
}

int Scripting::Schedule::childCount() const
{
    return m_schedule ? m_schedule->childCount() : 0;
}

QObject *Scripting::Schedule::childAt(int index)
{
    if (m_schedule && m_project) {
        return m_project->schedule(m_schedule->childAt(index));
    }
    return 0;
}
