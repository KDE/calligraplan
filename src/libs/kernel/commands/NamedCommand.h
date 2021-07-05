/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTNAMEDCOMMAND_H
#define KPTNAMEDCOMMAND_H

#include "plankernel_export.h"

#include <kundo2command.h>

#include <QHash>


/// The main namespace
namespace KPlato
{

class Schedule;

class PLANKERNEL_EXPORT NamedCommand : public KUndo2Command
{
public:
    explicit NamedCommand(const KUndo2MagicString& name)
        : KUndo2Command(name)
    {}
    void redo() override { execute(); }
    void undo() override { unexecute(); }

    virtual void execute() = 0;
    virtual void unexecute() = 0;

protected:
    /// Set all scheduled in the m_schedules map to their original scheduled state
    void setSchScheduled();
    /// Set all schedules in the m_schedules map to scheduled state @p state
    void setSchScheduled(bool state);
    /// Add a schedule to the m_schedules map along with its current scheduled state
    void addSchScheduled(Schedule *sch);

    QHash<Schedule*, bool> m_schedules;

};

}

#endif
