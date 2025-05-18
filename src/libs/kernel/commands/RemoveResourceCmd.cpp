/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "RemoveResourceCmd.h"
#include "RemoveParentGroupCmd.h"
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
        const auto schedules = m_project->schedules();
        for (Schedule * s : schedules) {
            Schedule *rs = resource->findSchedule(s->id());
            if (rs && ! rs->isDeleted()) {
                debugPlan<<s->name();
                addSchScheduled(s);
            }
        }
        const auto resources = m_project->resourceList();
        for (auto r : resources) {
            auto required = r->requiredIds();
            if (required.contains(resource->id())) {
                required.removeAll(resource->id());
                m_preCmd.addCommand(new ModifyRequiredResourcesCmd(r, required));
            }
            auto teams = r->teamMemberIds();
            if (teams.contains(resource->id())) {
                m_preCmd.addCommand(new RemoveResourceTeamCmd(r, resource->id()));
            }
        }
    }
    if (resource->account()) {
        m_postCmd.addCommand(new ResourceModifyAccountCmd(*resource, resource->account(), nullptr));
    }
    for (ResourceRequest *r : std::as_const(m_requests)) {
        m_preCmd.addCommand(new RemoveResourceRequestCmd(r));
        //debugPlan<<"Remove request for"<<r->resource()->name();
    }
    const auto parentGroups = resource->parentGroups();
    for (ResourceGroup *g : parentGroups) {
        m_preCmd.addCommand(new RemoveParentGroupCmd(resource, g));
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
