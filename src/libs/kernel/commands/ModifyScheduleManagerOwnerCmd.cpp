/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ModifyScheduleManagerOwnerCmd.h"

using namespace KPlato;


ModifyScheduleManagerOwnerCmd::ModifyScheduleManagerOwnerCmd(ScheduleManager *manager, ScheduleManager::Owner value, const KUndo2MagicString &name)
    : NamedCommand(name)
    , m_manager(manager)
    , m_oldvalue(manager->owner())
    , m_newvalue(value)
{
}

void ModifyScheduleManagerOwnerCmd::execute()
{
    m_manager->setOwner(m_newvalue);
}

void ModifyScheduleManagerOwnerCmd::unexecute()
{
    m_manager->setOwner(m_oldvalue);
}
