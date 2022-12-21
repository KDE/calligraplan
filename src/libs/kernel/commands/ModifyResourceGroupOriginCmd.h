/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef ModifyResourceGroupOriginCmd_H
#define ModifyResourceGroupOriginCmd_H

#include "plankernel_export.h"

#include "NamedCommand.h"


/// The main namespace
namespace KPlato
{

class ResourceGroup;

class PLANKERNEL_EXPORT ModifyResourceGroupOriginCmd : public NamedCommand
{
public:
    ModifyResourceGroupOriginCmd(KPlato::ResourceGroup *group, bool newvalue, const KUndo2MagicString& name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;

private:
    ResourceGroup *m_group = nullptr;
    bool m_newvalue;
};

}

#endif
