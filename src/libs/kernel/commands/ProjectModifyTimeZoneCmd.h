/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTPROJECTMODIFYTIMEZONECMD_H
#define KPTPROJECTMODIFYTIMEZONECMD_H

#include "plankernel_export.h"

#include "NamedCommand.h"

#include <kundo2command.h>

#include <QTimeZone>

/// The main namespace
namespace KPlato
{

class Project;

class PLANKERNEL_EXPORT ProjectModifyTimeZoneCmd : public NamedCommand
{
public:
    ProjectModifyTimeZoneCmd(Project &, const QTimeZone &tz, const KUndo2MagicString& name = KUndo2MagicString());
    ~ProjectModifyTimeZoneCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Project &m_project;
    QTimeZone m_oldtz;
    QTimeZone m_newtz;
};

}

#endif
