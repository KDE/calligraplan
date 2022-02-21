/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ProjectModifyTimeZoneCmd.h"

#include "kptproject.h"

using namespace KPlato;

ProjectModifyTimeZoneCmd::ProjectModifyTimeZoneCmd(Project &project, const QTimeZone &tz, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_project(project)
    , m_oldtz(project.timeZone())
    , m_newtz(tz)
{
}

ProjectModifyTimeZoneCmd::~ProjectModifyTimeZoneCmd()
{
}

void ProjectModifyTimeZoneCmd::execute()
{
    m_project.setTimeZone(m_newtz);
}

void ProjectModifyTimeZoneCmd::unexecute()
{
    m_project.setTimeZone(m_oldtz);
}
