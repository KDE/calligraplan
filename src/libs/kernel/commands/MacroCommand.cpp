/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
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

// clazy:excludeall=qstring-arg
#include "MacroCommand.h"
#include "kptdebug.h"

#include <QApplication>


using namespace KPlato;

MacroCommand::~MacroCommand()
{
    while (! cmds.isEmpty()) {
        delete cmds.takeLast();
    }
}

void MacroCommand::addCommand(KUndo2Command *cmd)
{
    cmds.append(cmd);
}

void MacroCommand::execute()
{
    foreach (KUndo2Command *c, cmds) {
        c->redo();
    }
}

void MacroCommand::unexecute()
{
    for (int i = cmds.count() - 1; i >= 0; --i) {
        cmds.at(i)->undo();
    }
}
