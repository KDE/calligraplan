/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
