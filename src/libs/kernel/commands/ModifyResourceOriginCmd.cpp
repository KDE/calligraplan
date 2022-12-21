/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ModifyResourceOriginCmd.h"
#include "Resource.h"


using namespace KPlato;


ModifyResourceOriginCmd::ModifyResourceOriginCmd(Resource *resource, bool newvalue, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_resource(resource)
    , m_newvalue(newvalue)
{
}

void ModifyResourceOriginCmd::execute()
{
    m_resource->setShared(m_newvalue);
}

void ModifyResourceOriginCmd::unexecute()
{
    m_resource->setShared(!m_newvalue);
}
