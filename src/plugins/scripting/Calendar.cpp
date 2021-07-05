/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "Calendar.h"

#include "Project.h"

#include "kptcalendar.h"

Scripting::Calendar::Calendar(Scripting::Project *project, KPlato::Calendar *calendar, QObject *parent)
    : QObject(parent), m_project(project), m_calendar(calendar)
{
}

QObject *Scripting::Calendar::project()
{
    return m_project;
}

QString Scripting::Calendar::id() const
{
    return m_calendar->id();
}

QString Scripting::Calendar::name() const
{
    return m_calendar->name();
}

int Scripting::Calendar::childCount() const
{
    return m_calendar->childCount();
}

QObject *Scripting::Calendar::childAt(int index)
{
    KPlato::Calendar *c = m_calendar->childAt(index);
    return c == 0 ? 0 : m_project->calendar(c);
}
