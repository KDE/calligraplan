/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
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
#include "AddResourceCmd.h"

#include "kptproject.h"
#include "Resource.h"
#include "ResourceGroup.h"

using namespace KPlato;


AddResourceCmd::AddResourceCmd(Project *project, Resource *resource, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_project(project)
    , m_resource(resource)
    , m_group(nullptr)
{
    m_index = project->resourceList().indexOf(resource);
    m_mine = true;
}

AddResourceCmd::AddResourceCmd(ResourceGroup *group, Resource *resource, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_project(group->project())
    , m_resource(resource)
    , m_group(group)
{
    Q_ASSERT(m_project);
    m_index = m_project->resourceList().indexOf(resource);
    m_mine = true;
}

AddResourceCmd::~AddResourceCmd()
{
    if (m_mine) {
        //debugPlan<<"delete:"<<m_resource;
        delete m_resource;
    }
}

void AddResourceCmd::execute()
{
    m_project->addResource(m_resource, m_index);
    m_mine = false;
    if (m_group) {
        m_resource->addParentGroup(m_group);
    }
}

void AddResourceCmd::unexecute()
{
    if (m_group) {
        m_group->takeResource(m_resource);
    }
    m_project->takeResource(m_resource);
    m_mine = true;
}
