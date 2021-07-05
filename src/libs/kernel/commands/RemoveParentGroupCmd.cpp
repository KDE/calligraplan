/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "RemoveParentGroupCmd.h"

#include "Resource.h"
#include "ResourceGroup.h"

using namespace KPlato;


RemoveParentGroupCmd::RemoveParentGroupCmd(Resource *resource, ResourceGroup *group, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_resource(resource)
    , m_group(group)
{
}

RemoveParentGroupCmd::~RemoveParentGroupCmd()
{
}

void RemoveParentGroupCmd::execute()
{
    m_resource->removeParentGroup(m_group);
}

void RemoveParentGroupCmd::unexecute()
{
    m_resource->addParentGroup(m_group);
}
