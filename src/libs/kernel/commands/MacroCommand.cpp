/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    if (m_busyCursorEnabled) {
        QApplication::setOverrideCursor(Qt::BusyCursor);
    }
    for (KUndo2Command *c : std::as_const(cmds)) {
        c->redo();
    }
    if (m_busyCursorEnabled) {
        QApplication::restoreOverrideCursor();
    }
}

void MacroCommand::unexecute()
{
    if (m_busyCursorEnabled) {
        QApplication::setOverrideCursor(Qt::BusyCursor);
    }
    for (int i = cmds.count() - 1; i >= 0; --i) {
        cmds.at(i)->undo();
    }
    if (m_busyCursorEnabled) {
        QApplication::restoreOverrideCursor();
    }
}
