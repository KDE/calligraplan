/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTREMOVEPARENTGROUPCMD_H
#define KPTREMOVEPARENTGROUPCMD_H

#include "plankernel_export.h"

#include "NamedCommand.h"


/// The main namespace
namespace KPlato
{

class Resource;
class ResourceGroup;

class PLANKERNEL_EXPORT RemoveParentGroupCmd : public NamedCommand
{
public:
    RemoveParentGroupCmd(Resource *resource, ResourceGroup *group, const KUndo2MagicString& name = KUndo2MagicString());
    ~RemoveParentGroupCmd() override;
    void execute() override;
    void unexecute() override;

protected:
    Resource *m_resource;
    ResourceGroup *m_group;
};

}

#endif
