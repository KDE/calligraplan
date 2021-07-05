/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
