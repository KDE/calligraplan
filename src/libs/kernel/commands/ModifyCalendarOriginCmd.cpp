/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ModifyCalendarOriginCmd.h"
#include "Resource.h"


using namespace KPlato;


ModifyCalendarOriginCmd::ModifyCalendarOriginCmd(Calendar *calendar, bool newvalue, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_calendar(calendar)
    , m_newvalue(newvalue)
{
}

void ModifyCalendarOriginCmd::execute()
{
    m_calendar->setShared(m_newvalue);
}

void ModifyCalendarOriginCmd::unexecute()
{
    m_calendar->setShared(!m_newvalue);
}
