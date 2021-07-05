/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTREMOVERESOURCECMD_H
#define KPTREMOVERESOURCECMD_H

#include "plankernel_export.h"

#include "AddResourceCmd.h"
#include "MacroCommand.h"


/// The main namespace
namespace KPlato
{

class Resource;
class ResourceRequest;
class Appointment;

class PLANKERNEL_EXPORT RemoveResourceCmd : public AddResourceCmd
{
public:
    RemoveResourceCmd(Resource *resource, const KUndo2MagicString& name = KUndo2MagicString());
    ~RemoveResourceCmd() override;
    void execute() override;
    void unexecute() override;

private:
    QList<ResourceRequest*> m_requests;
    QList<Appointment*> m_appointments;
    MacroCommand m_preCmd;
    MacroCommand m_postCmd;
};

}

#endif
