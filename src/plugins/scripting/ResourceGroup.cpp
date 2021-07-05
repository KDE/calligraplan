/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ResourceGroup.h"

#include "Project.h"

#include <kptresource.h>

Scripting::ResourceGroup::ResourceGroup(Scripting::Project *project, KPlato::ResourceGroup *group, QObject *parent)
    : QObject(parent), m_project(project), m_group(group)
{
}

QObject *Scripting::ResourceGroup::project()
{
    return m_project;
}

QString Scripting::ResourceGroup::id()
{
    return m_group->id();
}

QString Scripting::ResourceGroup::type()
{
    return m_group->typeToString();
}

int Scripting::ResourceGroup::resourceCount() const
{
    return m_group->numResources();
}

QObject *Scripting::ResourceGroup::resourceAt(int index) const
{
    return m_project->resource(m_group->resourceAt(index));
}

int Scripting::ResourceGroup::childCount() const
{
    return m_group->numResources();
}

QObject *Scripting::ResourceGroup::childAt(int index) const
{
    return m_project->resource(m_group->resourceAt(index));
}
