/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef ModifyResourceOriginCmd_H
#define ModifyResourceOriginCmd_H

#include "plankernel_export.h"

#include "NamedCommand.h"


/// The main namespace
namespace KPlato
{

class Resource;

class PLANKERNEL_EXPORT ModifyResourceOriginCmd : public NamedCommand
{
public:
    ModifyResourceOriginCmd(Resource *resource, bool newvalue, const KUndo2MagicString& name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource = nullptr;
    bool m_newvalue;
};

}

#endif
