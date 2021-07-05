/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTADDRESOURCECMD_H
#define KPTADDRESOURCECMD_H

#include "plankernel_export.h"

#include "NamedCommand.h"


/// The main namespace
namespace KPlato
{

class Project;
class Resource;
class ResourceGroup;

class PLANKERNEL_EXPORT AddResourceCmd : public NamedCommand
{
public:
    AddResourceCmd(Project *project, Resource *resource, const KUndo2MagicString& name = KUndo2MagicString());
    AddResourceCmd(ResourceGroup *group, Resource *resource, const KUndo2MagicString& name = KUndo2MagicString());
    ~AddResourceCmd() override;
    void execute() override;
    void unexecute() override;

protected:
    Project *m_project;
    Resource *m_resource;
    ResourceGroup *m_group;
    int m_index;
    bool m_mine;
};

}

#endif
