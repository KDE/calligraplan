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

#ifndef KPTMACROCOMMAND_H
#define KPTMACROCOMMAND_H

#include "plankernel_export.h"

#include <kundo2command.h>

namespace KPlato
{
    
class PLANKERNEL_EXPORT MacroCommand : public KUndo2Command
{
public:
    explicit MacroCommand(const KUndo2MagicString& name = KUndo2MagicString())
        : KUndo2Command(name)
    {}
    ~MacroCommand() override;

    void addCommand(KUndo2Command *cmd);

    void redo() override { execute(); }
    void undo() override { unexecute(); }

    virtual void execute();
    virtual void unexecute();

    bool isEmpty() const { return cmds.isEmpty(); }

protected:
    QList<KUndo2Command*> cmds;
};

}

#endif
