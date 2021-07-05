/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "AddParentGroupCmd.h"

#include "Resource.h"
#include "ResourceGroup.h"

using namespace KPlato;


AddParentGroupCmd::AddParentGroupCmd(Resource *resource, ResourceGroup *group, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_resource(resource)
    , m_group(group)
{
}

AddParentGroupCmd::~AddParentGroupCmd()
{
}

void AddParentGroupCmd::execute()
{
    m_resource->addParentGroup(m_group);
}

void AddParentGroupCmd::unexecute()
{
    m_group->takeResource(m_resource);
}
