/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTADDALTERNATIVERESOURCECMD_H
#define KPTADDALTERNATIVERESOURCECMD_H

#include "plankernel_export.h"

#include "NamedCommand.h"


/// The main namespace
namespace KPlato
{

class Resource;
class ResourceGroup;

class PLANKERNEL_EXPORT RemoveAlternativeResourceCmd : public NamedCommand
{
public:
    RemoveAlternativeResourceCmd(ResourceRequest *request, Resource *resource, const KUndo2MagicString& name = KUndo2MagicString());
    ~RemoveAlternativeResourceCmd() override;
    void execute() override;
    void unexecute() override;

protected:
    ResourceRequest *m_request;
    Resource *m_resource;
};

}

#endif
