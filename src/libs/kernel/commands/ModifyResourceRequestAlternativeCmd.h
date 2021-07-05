/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef MODIFYRESOURCEREQUESTALTERNATIVECMD_H
#define MODIFYRESOURCEREQUESTALTERNATIVECMD_H

#include "plankernel_export.h"

#include "NamedCommand.h"


/// The main namespace
namespace KPlato
{

class Resource;
class ResourceRequest;
class Appointment;

class PLANKERNEL_EXPORT ModifyResourceRequestAlternativeCmd : public NamedCommand
{
public:
    ModifyResourceRequestAlternativeCmd(ResourceRequest *request, const QList<ResourceRequest*> &value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ResourceRequest *m_request;
    QList<ResourceRequest*> m_oldvalue, m_newvalue;
};

}

#endif
