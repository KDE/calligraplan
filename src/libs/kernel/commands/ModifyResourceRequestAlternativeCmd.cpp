/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ModifyResourceRequestAlternativeCmd.h"

#include <kptresourcerequest.h>


using namespace KPlato;

ModifyResourceRequestAlternativeCmd::ModifyResourceRequestAlternativeCmd(ResourceRequest *request, const QList<ResourceRequest*> &value, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_request(request)
    , m_newvalue(value)
{
    m_oldvalue = request->alternativeRequests();
}
void ModifyResourceRequestAlternativeCmd::execute()
{
    m_request->setAlternativeRequests(m_newvalue);
}
void ModifyResourceRequestAlternativeCmd::unexecute()
{
    m_request->setAlternativeRequests(m_oldvalue);
}
