/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ResourceGroupModifyCoordinatorCmd.h"

#include "ResourceGroup.h"

using namespace KPlato;

ResourceGroupModifyCoordinatorCmd::ResourceGroupModifyCoordinatorCmd(ResourceGroup *group, const QString &value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_group(group),
    m_newvalue(value)
{
    m_oldvalue = group->coordinator();
}

void ResourceGroupModifyCoordinatorCmd::execute()
{
    m_group->setCoordinator(m_newvalue);
}

void ResourceGroupModifyCoordinatorCmd::unexecute()
{
    m_group->setCoordinator(m_oldvalue);
}
