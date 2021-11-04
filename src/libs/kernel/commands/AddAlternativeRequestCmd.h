/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTADDALTERNATIVEREQUESTCMD_H
#define KPTADDALTERNATIVEREQUESTCMD_H

#include "plankernel_export.h"

#include "NamedCommand.h"


/// The main namespace
namespace KPlato
{

class ResourceRequest;

class PLANKERNEL_EXPORT AddAlternativeResourceCmd : public NamedCommand
{
public:
    AddAlternativeResourceCmd(ResourceRequest *request, ResourceRequest *alternative, const KUndo2MagicString& name = KUndo2MagicString());
    ~AddAlternativeResourceCmd() override;
    void execute() override;
    void unexecute() override;

protected:
    ResourceRequest *m_request;
    ResourceRequest *m_alternative;
};

}

#endif
