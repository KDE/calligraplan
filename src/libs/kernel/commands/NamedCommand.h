/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
