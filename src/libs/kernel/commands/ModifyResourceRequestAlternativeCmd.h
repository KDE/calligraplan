/* This file is part of the KDE project
 * Copyright (C) 2020 Dag Andersen <danders@get2net.dk>
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

#ifndef MODIFYRESOURCEREQUESTALTERNATIVECMD_H
#define MODIFYRESOURCEREQUESTALTERNATIVECMD_H

#include "plankernel_export.h"

#include "NamedCommand.h"


/// The main namespace
namespace KPlato
{

class Resource;
class ResourceRequest;
class Appointment;

class PLANKERNEL_EXPORT ModifyResourceRequestAlternativeCmd : public NamedCommand
{
public:
    ModifyResourceRequestAlternativeCmd(ResourceRequest *request, const QList<ResourceRequest*> &value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ResourceRequest *m_request;
    QList<ResourceRequest*> m_oldvalue, m_newvalue;
};

}

#endif
