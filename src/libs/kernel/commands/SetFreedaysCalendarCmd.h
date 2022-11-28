/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SETFREEDAYSCALENDARCMD_H
#define SETFREEDAYSCALENDARCMD_H

#include "plankernel_export.h"

#include "NamedCommand.h"


/// The main namespace
namespace KPlato
{

class Project;
class Calendar;

class PLANKERNEL_EXPORT SetFreedaysCalendarCmd : public NamedCommand
{
public:
    SetFreedaysCalendarCmd(Project *project, Calendar *value, const KUndo2MagicString& name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;

private:
    Project *m_project = nullptr;
    Calendar *m_oldvalue;
    Calendar *m_newvalue;
};

}

#endif
