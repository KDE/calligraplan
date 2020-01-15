/* This file is part of the KDE project
 * Copyright (C) 2020 Dag Andersen <danders@get2net.dk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
