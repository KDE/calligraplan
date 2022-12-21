/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ModifyResourceGroupOriginCmd.h"
#include "ResourceGroup.h"


using namespace KPlato;


ModifyResourceGroupOriginCmd::ModifyResourceGroupOriginCmd(ResourceGroup *group, bool newvalue, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_group(group)
    , m_newvalue(newvalue)
{
}

void ModifyResourceGroupOriginCmd::execute()
{
    m_group->setShared(m_newvalue);
}

void ModifyResourceGroupOriginCmd::unexecute()
{
    m_group->setShared(!m_newvalue);
}
