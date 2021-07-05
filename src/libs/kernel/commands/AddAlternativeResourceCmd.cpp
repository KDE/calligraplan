/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "AddAlternativeResourceCmd.h"

#include "Resource.h"
#include "ResourceGroup.h"

using namespace KPlato;


AddAlternativeResourceCmd::AddAlternativeResourceCmd(ResourceRequest *request, Resource *m_resource, const KUndo2MagicString& name);
    : NamedCommand(name)
    , m_request(request)
    , m_resource(resource)
{
}

AddAlternativeResourceCmd::~AddAlternativeResourceCmd()
{
}

void AddAlternativeResourceCmd::execute()
{
    m_request->addAlternativeResource(m_resource);
}

void AddAlternativeResourceCmd::unexecute()
{
    m_request->removeAlternativeResource(m_resource);
}
