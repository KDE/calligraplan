/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "AddAlternativeRequestCmd.h"

#include "kptresourcerequest.h"

using namespace KPlato;


AddAlternativeResourceCmd::AddAlternativeResourceCmd(ResourceRequest *request, ResourceRequest *alternative, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_request(request)
    , m_alternative(alternative)
{
}

AddAlternativeResourceCmd::~AddAlternativeResourceCmd()
{
}

void AddAlternativeResourceCmd::execute()
{
    m_request->addAlternativeRequest(m_alternative);
}

void AddAlternativeResourceCmd::unexecute()
{
    m_request->removeAlternativeRequest(m_alternative);
}
