/* This file is part of the KDE project
 * Copyright (C) 2001 Thomas zander <zander@kde.org>
 * Copyright (C) 2004-2007, 2012 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2016 Dag Andersen <danders@get2net.dk>
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
#include "ResourceGroup.h"
#include "Resource.h"
#include "kptresourcerequest.h"

#include "kptlocale.h"
#include "kptaccount.h"
#include "kptappointment.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptdatetime.h"
#include "kptcalendar.h"
#include "kpteffortcostmap.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"
#include "kptdebug.h"

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QLocale>


using namespace KPlato;


ResourceGroup::ResourceGroup()
    : QObject(nullptr)
    , m_blockChanged(false)
    , m_shared(false)
{
    m_project = nullptr;
    m_parent = nullptr;
    //debugPlan<<"("<<this<<")";
}

ResourceGroup::ResourceGroup(const ResourceGroup *group)
    : QObject(nullptr)
{
    m_project = nullptr;
    m_parent = nullptr;
    copy(group);
}

ResourceGroup::~ResourceGroup() {
    //debugPlan<<"("<<this<<")";
    if (m_parent) {
        removeId();
        m_parent->removeChildGroup(this);
    } else if (m_project) {
        m_project->takeResourceGroup(this); // also removes id
    }
    for (Resource *r : m_resources) {
        r->removeParentGroup(this);
    }
    qDeleteAll(m_childGroups);
    //debugPlan<<"("<<this<<")";
}

void ResourceGroup::copy(const ResourceGroup *group)
{
    //m_project = group->m_project; //Don't copy
    //m_parent = group->m_parent; //Don't copy
    // m_id = group->m_id;  //Don't copy
    m_type = group->m_type;
    m_name = group->m_name;
}

void ResourceGroup::blockChanged(bool on)
{
    m_blockChanged = on;
}

void ResourceGroup::changed() {
    if (!m_blockChanged) {
        emit dataChanged(this);
        if (m_project) {
            emit m_project->resourceGroupChanged(this);
        }
    }
}

void ResourceGroup::setId(const QString& id) {
    //debugPlan<<id;
    m_id = id;
}

void ResourceGroup::setName(const QString& n)
{
    m_name = n.trimmed();
    changed();
}

QString ResourceGroup::type() const
{
    return m_type;
}

void ResourceGroup::setType(const QString &type)
{
    m_type = type;
}

QString ResourceGroup::coordinator() const
{
    return m_coordinator;
}

void ResourceGroup::setCoordinator(const QString &coordinator)
{
    m_coordinator = coordinator;
}

Project *ResourceGroup::project() const
{
    return m_parent ? m_parent->project() : m_project;
}

void ResourceGroup::setProject(Project *project)
{
    if (project != m_project) {
        if (m_project) {
            removeId();
        }
    }
    m_project = project;
    foreach (Resource *r, m_resources) {
        r->setProject(project);
    }
}

bool ResourceGroup::isScheduled() const
{
    foreach (Resource *r, m_resources) {
        if (r->isScheduled()) {
            return true;
        }
    }
    return false;
}

bool ResourceGroup::isBaselined(long id) const
{
    Q_UNUSED(id);
    foreach (const Resource *r, m_resources) {
        if (r->isBaselined()) {
            return true;
        }
    }
    return false;
}

ResourceGroup *ResourceGroup::parentGroup() const
{
    return m_parent;
}

void ResourceGroup::setParentGroup(ResourceGroup *parent)
{
    m_parent = parent;
}

int ResourceGroup::indexOf(ResourceGroup *group) const
{
    return m_childGroups.indexOf(group);
}

int ResourceGroup::numChildGroups() const
{
    return m_childGroups.count();
}

void ResourceGroup::addChildGroup(ResourceGroup *group, int row)
{
    Q_ASSERT(!m_childGroups.contains(group));
    int pos = row < 0 ? m_childGroups.count() : row;
    emit groupToBeAdded(project(), this, pos);
    m_childGroups.insert(pos, group);
    group->setParentGroup(this);
    emit groupAdded(group);
}

ResourceGroup *ResourceGroup::childGroupAt(int row) const
{
    return m_childGroups.value(row);
}

QList<ResourceGroup*> ResourceGroup::childGroups() const
{
    return m_childGroups;
}

void ResourceGroup::removeChildGroup(ResourceGroup *child)
{
    int row = m_childGroups.indexOf(child);
    emit groupToBeRemoved(project(), this, row, child);
    m_childGroups.removeOne(child);
    child->setParentGroup(nullptr);
    emit groupRemoved();
}

void ResourceGroup::addResource(Resource* resource, Risk *risk)
{
    addResource(m_resources.count(), resource, risk);
}

void ResourceGroup::addResource(int index, Resource* resource, Risk*)
{
    if (!m_resources.contains(resource)) {
        int i = index == -1 ? m_resources.count() : index;
        emit resourceToBeAdded(this, i);
        m_resources.insert(i, resource);
        emit resourceAdded(resource);
    }
    resource->addParentGroup(this);
}

Resource *ResourceGroup::takeResource(Resource *resource)
{
    Resource *r = 0;
    int i = m_resources.indexOf(resource);
    if (i != -1) {
        emit resourceToBeRemoved(this, i, resource);
        r = m_resources.takeAt(i);
        emit resourceRemoved();
        r->removeParentGroup(this);
    }
    return r;
}

int ResourceGroup::indexOf(const Resource *resource) const
{
    return m_resources.indexOf(const_cast<Resource*>(resource)); //???
}

Risk* ResourceGroup::getRisk(int) {
    return 0L;
}

void ResourceGroup::addRequiredResource(ResourceGroup*) {
}

ResourceGroup* ResourceGroup::getRequiredResource(int) {
    return 0L;
}

void ResourceGroup::deleteRequiredResource(int) {
}

bool ResourceGroup::load(KoXmlElement &element, XMLLoaderObject &status) {
    //debugPlan;
    setId(element.attribute("id"));
    m_name = element.attribute("name");
    setType(element.attribute("type"));
    if (status.version() < "07.0") {
        m_shared = element.attribute("shared", "0").toInt();
    } else {
        m_shared = element.attribute("origin", "local") != "local";
    }
    m_coordinator = element.attribute("coordinator");

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == "resource-group") {
            ResourceGroup *child = new ResourceGroup();
            if (child->load(e, status)) {
                addChildGroup(child);
            } else {
                errorPlanXml<<"Faild to load ResourceGroup";
                delete child;
            }
        } else if (status.version() < "0.7.0" && e.tagName() == "resource") {
            // Load the resource
            Resource *child = new Resource();
            if (child->load(e, status)) {
                child->addParentGroup(this);
            } else {
                // TODO: Complain about this
                delete child;
            }
        }
    }
    return true;
}

void ResourceGroup::save(QDomElement &element)  const {
    //debugPlan;

    QDomElement me = element.ownerDocument().createElement("resource-group");
    element.appendChild(me);

    me.setAttribute("id", m_id);
    me.setAttribute("name", m_name);
    me.setAttribute("type", m_type);
    me.setAttribute("shared", m_shared ? "shared" : "local");
    me.setAttribute("coordinator", m_coordinator);

    for (ResourceGroup *g : m_childGroups) {
        g->save(me);
    }
}

void ResourceGroup::saveWorkPackageXML(QDomElement &element, const QList<Resource*> &lst) const
{
    QDomElement me = element.ownerDocument().createElement("resource-group");
    element.appendChild(me);

    me.setAttribute("id", m_id);
    me.setAttribute("name", m_name);

    foreach (Resource *r, m_resources) {
        if (lst.contains(r)) {
            r->save(me);
        }
    }
}

void ResourceGroup::initiateCalculation(Schedule &sch) {
    clearNodes();
}

int ResourceGroup::units() const {
    int u = 0;
    foreach (const Resource *r, m_resources) {
        u += r->units();
    }
    return u;
}

ResourceGroup *ResourceGroup::findId(const QString &id) const
{
    Project *p = project();
    return p ? p->findResourceGroup(id) : nullptr;
}

void ResourceGroup::removeId(const QString &id)
{
    Project *p = project();
    if (p) {
        p->removeResourceGroupId(id);
    }
}

void ResourceGroup::insertId(const QString &id) { 
    //debugPlan;
    Project *p = project();
    if (p) {
        p->insertResourceGroupId(id, this);
    }
}

Appointment ResourceGroup::appointmentIntervals() const {
    Appointment a;
    foreach (Resource *r, m_resources) {
        a += r->appointmentIntervals();
    }
    return a;
}

DateTime ResourceGroup::startTime(long id) const
{
    DateTime dt;
    foreach (Resource *r, m_resources) {
        DateTime t = r->startTime(id);
        if (! dt.isValid() || t < dt) {
            dt = t;
        }
    }
    return dt;
}

DateTime ResourceGroup::endTime(long id) const
{
    DateTime dt;
    foreach (Resource *r, m_resources) {
        DateTime t = r->endTime(id);
        if (! dt.isValid() || t > dt) {
            dt = t;
        }
    }
    return dt;
}

bool ResourceGroup::isShared() const
{
    return m_shared;
}

void ResourceGroup::setShared(bool on)
{
    m_shared = on;
}

QDebug operator<<(QDebug dbg, const KPlato::ResourceGroup *g)
{
    dbg.nospace().noquote()<<"ResourceGroup[";
    if (g) {
        dbg<<'('<<(void*)g<<')'<<g->name()<<','<<g->id()<<','<<g->numChildGroups();
    } else {
        dbg<<(void*)g;
    }
    dbg<<']';
    return dbg.space().quote();
}
