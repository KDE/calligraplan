/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008, 2011 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "Resource.h"

#include "Project.h"
#include "ScriptingDebug.h"

#include "kptresource.h"
#include "kptappointment.h"
#include "kptdatetime.h"
#include "kptcommand.h"

Scripting::Resource::Resource(Scripting::Project *project, KPlato::Resource *resource, QObject *parent)
    : QObject(parent), m_project(project), m_resource(resource)
{
}

QObject *Scripting::Resource::project()
{
    return m_project;
}

QVariant Scripting::Resource::type()
{
    return m_resource->typeToString();
}

QString Scripting::Resource::id() const
{
    return m_resource->id();
}

QVariantList Scripting::Resource::appointmentIntervals(qlonglong schedule) const
{
    KPlato::Appointment app = m_resource->appointmentIntervals(schedule);
    QVariantList lst;
    const auto intervals = app.intervals().map().values();
    for (const KPlato::AppointmentInterval &ai : intervals) {
        lst << QVariant(QVariantList() << ai.startTime().toString() << ai.endTime().toString() << ai.load());
    }
    return lst;
}

void Scripting::Resource::addExternalAppointment(const QVariant &id, const QString &name, const QVariantList &lst)
{
    m_project->addExternalAppointment(this, id, name, lst);
}

QVariantList Scripting::Resource::externalAppointments() const
{
    KPlato::AppointmentIntervalList ilst = m_resource->externalAppointments();
    QVariantList lst;
    const auto intervals = ilst.map().values();
    for (const KPlato::AppointmentInterval &ai ; intervals) {
        lst << QVariant(QVariantList() << ai.startTime().toString() << ai.endTime().toString() << ai.load());
    }
    return lst;
}

void Scripting::Resource::clearExternalAppointments(const QString &id)
{
    m_project->clearExternalAppointments(this, id);
}

int Scripting::Resource::childCount() const
{
    return kplatoResource()->type() == KPlato::Resource::Type_Team ? kplatoResource()->teamMembers().count() : 0;
}

QObject *Scripting::Resource::childAt(int index) const
{
    if (kplatoResource()->type() == KPlato::Resource::Type_Team) {
        return m_project->resource(kplatoResource()->teamMembers().value(index));
    }
    return 0;
}

void Scripting::Resource::setChildren(const QList<QObject*> &children)
{
    debugPlanScripting<<"setTeamMembers:"<<children;
    KPlato::Resource *team = kplatoResource();
    // atm. only teams have children
    if (team->type() != KPlato::Resource::Type_Team) {
        return;
    }
    KPlato::MacroCommand *cmd = new KPlato::MacroCommand(kundo2_i18n("Set resource team members"));
    const auto ids = team->teamMemberIds();
    for (const QString &id : ids ) {
       cmd->addCommand(new KPlato::RemoveResourceTeamCmd(team, id));
    }
    for (QObject *o : children) {
        Resource *r = qobject_cast<Resource*>(o);
        if (r && r->kplatoResource()) {
            cmd->addCommand(new KPlato::AddResourceTeamCmd(team, r->kplatoResource()->id()));
        }
    }
    if (! cmd->isEmpty()) {
        m_project->addCommand(cmd);
    }
    debugPlanScripting<<"setTeamMembers:"<<team->teamMembers();
}
