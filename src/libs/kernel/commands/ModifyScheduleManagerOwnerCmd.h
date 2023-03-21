/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTMODIFYSCHEDULEMANAGEROWNERCMD_H
#define KPTMODIFYSCHEDULEMANAGEROWNERCMD_H

#include "plankernel_export.h"

#include "NamedCommand.h"
#include "kptschedule.h"

/// The main namespace
namespace KPlato
{

class ScheduleManager;

class PLANKERNEL_EXPORT ModifyScheduleManagerOwnerCmd : public NamedCommand
{
public:
    ModifyScheduleManagerOwnerCmd(ScheduleManager *manager, ScheduleManager::Owner value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager *m_manager;
    ScheduleManager::Owner m_oldvalue;
    ScheduleManager::Owner m_newvalue;
};

}

#endif
