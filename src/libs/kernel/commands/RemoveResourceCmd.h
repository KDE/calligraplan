/* This file is part of the KDE project
 * Copyright (C) 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2011 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2016 Dag Andersen <dag.andersen@kdemail.net>
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

#ifndef KPTREMOVERESOURCECMD_H
#define KPTREMOVERESOURCECMD_H

#include "plankernel_export.h"

#include "AddResourceCmd.h"
#include "MacroCommand.h"


/// The main namespace
namespace KPlato
{

class Resource;
class ResourceRequest;
class Appointment;

class PLANKERNEL_EXPORT RemoveResourceCmd : public AddResourceCmd
{
public:
    RemoveResourceCmd(Resource *resource, const KUndo2MagicString& name = KUndo2MagicString());
    ~RemoveResourceCmd() override;
    void execute() override;
    void unexecute() override;

private:
    QList<ResourceRequest*> m_requests;
    QList<Appointment*> m_appointments;
    MacroCommand m_preCmd;
    MacroCommand m_postCmd;
};

}

#endif
