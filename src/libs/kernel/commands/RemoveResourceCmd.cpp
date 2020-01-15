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
#include "RemoveResourceCmd.h"
#include "Resource.h"
#include "kptproject.h"
#include "kptaccount.h"
#include "kptappointment.h"
#include "kptresourcerequest.h"
#include "kptcommand.h"
#include "kptdebug.h"


using namespace KPlato;


RemoveResourceCmd::RemoveResourceCmd(Resource *resource, const KUndo2MagicString& name)
    : AddResourceCmd(resource->project(), resource, name)
{
    //debugPlan<<resource;
    m_mine = false;
    m_requests = m_resource->requests();

    if (m_project) {
        foreach (Schedule * s, m_project->schedules()) {
            Schedule *rs = resource->findSchedule(s->id());
            if (rs && ! rs->isDeleted()) {
                debugPlan<<s->name();
                addSchScheduled(s);
            }
        }
    }
    if (resource->account()) {
        m_postCmd.addCommand(new ResourceModifyAccountCmd(*resource, resource->account(), 0));
    }
    for (ResourceRequest *r : m_requests) {
        m_preCmd.addCommand(new RemoveResourceRequestCmd(r));
        //debugPlan<<"Remove request for"<<r->resource()->name();
    }
}

RemoveResourceCmd::~RemoveResourceCmd()
{
    while (!m_appointments.isEmpty())
        delete m_appointments.takeFirst();
}

void RemoveResourceCmd::execute()
{
    m_preCmd.redo();
    AddResourceCmd::unexecute();
    m_postCmd.redo();
    setSchScheduled(false);
}

void RemoveResourceCmd::unexecute()
{
    m_postCmd.undo();
    AddResourceCmd::execute();
    m_preCmd.undo();
    setSchScheduled();
}
