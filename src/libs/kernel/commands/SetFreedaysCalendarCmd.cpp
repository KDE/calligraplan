/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "SetFreedaysCalendarCmd.h"
#include "kptproject.h"
#include "kptcalendar.h"


using namespace KPlato;


SetFreedaysCalendarCmd::SetFreedaysCalendarCmd(Project *project, Calendar *newvalue, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_project(project)
    , m_newvalue(newvalue)
{
}

void SetFreedaysCalendarCmd::execute()
{
    m_project->setFreedaysCalendar(m_newvalue);
}

void SetFreedaysCalendarCmd::unexecute()
{
    m_project->setFreedaysCalendar(m_oldvalue);
}
