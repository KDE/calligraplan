/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "RemoveAlternativeResourceCmd.h"

#include "Resource.h"
#include "ResourceGroup.h"

using namespace KPlato;


RemoveAlternativeResourceCmd::RemoveAlternativeResourceCmd(ResourceRequest *request, Resource *m_resource, const KUndo2MagicString& name);
    : NamedCommand(name)
    , m_request(request)
    , m_resource(resource)
{
}

RemoveAlternativeResourceCmd::~RemoveAlternativeResourceCmd()
{
}

void RemoveAlternativeResourceCmd::execute()
{
    m_request->addAlternativeResource(m_resource);
}

void RemoveAlternativeResourceCmd::unexecute()
{
    m_request->removeAlternativeResource(m_resource);
}
