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
    : QObject(0),
    m_blockChanged(false),
    m_shared(false)
{
    m_project = 0;
    m_type = Type_Work;
    //debugPlan<<"("<<this<<")";
}

ResourceGroup::ResourceGroup(const ResourceGroup *group)
    : QObject(0) 
{
    m_project = 0;
    copy(group);
}

ResourceGroup::~ResourceGroup() {
    //debugPlan<<"("<<this<<")";
    if (findId() == this) {
        removeId(); // only remove myself (I may be just a working copy)
    }
    for (Resource *r : m_resources) {
        r->removeParentGroup(this);
    }
    //debugPlan<<"("<<this<<")";
}

void ResourceGroup::copy(const ResourceGroup *group)
{
    //m_project = group->m_project; //Don't copy
    m_id = group->m_id;
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

void ResourceGroup::setType(Type type)
{
     m_type = type;
     changed();
}

void ResourceGroup::setType(const QString &type)
{
    if (type == "Work")
        setType(Type_Work);
    else if (type == "Material")
        setType(Type_Material);
    else
        setType(Type_Work);
}

QString ResourceGroup::typeToString(bool trans) const {
    return typeToStringList(trans).at(m_type);
}

QStringList ResourceGroup::typeToStringList(bool trans) {
    // keep these in the same order as the enum!
    return QStringList() 
            << (trans ? i18n("Work") : QString("Work"))
            << (trans ? i18n("Material") : QString("Material"));
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
    m_shared = element.attribute("shared", "0").toInt();

    if (status.version() < "0.7.0") {
        KoXmlNode n = element.firstChild();
        for (; ! n.isNull(); n = n.nextSibling()) {
            if (! n.isElement()) {
                continue;
            }
            KoXmlElement e = n.toElement();
            if (e.tagName() == "resource") {
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
    }
    return true;
}

void ResourceGroup::save(QDomElement &element)  const {
    //debugPlan;

    QDomElement me = element.ownerDocument().createElement("resource-group");
    element.appendChild(me);

    me.setAttribute("id", m_id);
    me.setAttribute("name", m_name);
    me.setAttribute("type", typeToString());
    me.setAttribute("shared", m_shared);
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

ResourceGroup *ResourceGroup::findId(const QString &id) const {
    return m_project ? m_project->findResourceGroup(id) : 0;
}

bool ResourceGroup::removeId(const QString &id) { 
    return m_project ? m_project->removeResourceGroupId(id): false;
}

void ResourceGroup::insertId(const QString &id) { 
    //debugPlan;
    if (m_project)
        m_project->insertResourceGroupId(id, this);
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
