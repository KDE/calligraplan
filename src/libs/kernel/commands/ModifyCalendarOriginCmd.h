/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef ModifyCalendarOriginCmd_H
#define ModifyCalendarOriginCmd_H

#include "plankernel_export.h"

#include "NamedCommand.h"


/// The main namespace
namespace KPlato
{

class Calendar;

class PLANKERNEL_EXPORT ModifyCalendarOriginCmd : public NamedCommand
{
public:
    ModifyCalendarOriginCmd(Calendar *calendar, bool newvalue, const KUndo2MagicString& name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;

private:
    Calendar *m_calendar = nullptr;
    bool m_newvalue;
};

}

#endif
