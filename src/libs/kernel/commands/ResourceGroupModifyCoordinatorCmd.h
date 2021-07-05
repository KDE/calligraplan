/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef RESOURCEGROUPMODIFYCOORDINATORCMD_H
#define RESOURCEGROUPMODIFYCOORDINATORCMD_H

#include "plankernel_export.h"

#include "NamedCommand.h"

namespace KPlato
{

class ResourceGroup;

class PLANKERNEL_EXPORT ResourceGroupModifyCoordinatorCmd : public NamedCommand
{
    public:
        ResourceGroupModifyCoordinatorCmd(ResourceGroup *group, const QString &value, const KUndo2MagicString& name = KUndo2MagicString());
        void execute() override;
        void unexecute() override;

    private:
        ResourceGroup *m_group;
        QString m_newvalue;
        QString m_oldvalue;
};

}

#endif
