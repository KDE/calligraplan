/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001 Thomas zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2004-2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    for (Resource *r : std::as_const(m_resources)) {
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
        Q_EMIT dataChanged(this);
        if (m_project) {
            Q_EMIT m_project->resourceGroupChanged(this);
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
    for (Resource *r : std::as_const(m_resources)) {
        r->setProject(project);
    }
}

bool ResourceGroup::isScheduled() const
{
    for (Resource *r : std::as_const(m_resources)) {
        if (r->isScheduled()) {
            return true;
        }
    }
    return false;
}

bool ResourceGroup::isBaselined(long id) const
{
    Q_UNUSED(id);
    for (const Resource *r : std::as_const(m_resources)) {
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
    Q_EMIT groupToBeAdded(project(), this, pos);
    m_childGroups.insert(pos, group);
    group->setParentGroup(this);
    Q_EMIT groupAdded(group);
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
    Q_EMIT groupToBeRemoved(project(), this, row, child);
    m_childGroups.removeOne(child);
    child->setParentGroup(nullptr);
    Q_EMIT groupRemoved();
}

void ResourceGroup::addResource(Resource* resource, Risk *risk)
{
    addResource(m_resources.count(), resource, risk);
}

void ResourceGroup::addResource(int index, Resource* resource, Risk*)
{
    if (!m_resources.contains(resource)) {
        int i = index == -1 ? m_resources.count() : index;
        Q_EMIT resourceToBeAdded(this, i);
        m_resources.insert(i, resource);
        Q_EMIT resourceAdded(resource);
    }
    resource->addParentGroup(this);
}

Resource *ResourceGroup::takeResource(Resource *resource)
{
    Resource *r = nullptr;
    int i = m_resources.indexOf(resource);
    if (i != -1) {
        Q_EMIT resourceToBeRemoved(this, i, resource);
        r = m_resources.takeAt(i);
        Q_EMIT resourceRemoved();
        r->removeParentGroup(this);
    }
    return r;
}

int ResourceGroup::indexOf(const Resource *resource) const
{
    return m_resources.indexOf(const_cast<Resource*>(resource)); //???
}

Risk* ResourceGroup::getRisk(int) {
    return nullptr;
}

void ResourceGroup::addRequiredResource(ResourceGroup*) {
}

ResourceGroup* ResourceGroup::getRequiredResource(int) {
    return nullptr;
}

void ResourceGroup::deleteRequiredResource(int) {
}
#if 0
bool ResourceGroup::load(KoXmlElement &element, XMLLoaderObject &status) {
    //debugPlan;
    setId(element.attribute(QStringLiteral("id")));
    m_name = element.attribute(QStringLiteral("name"));
    setType(element.attribute(QStringLiteral("type")));
    if (status.version() < QStringLiteral("0.7.0")) {
        m_shared = element.attribute(QStringLiteral("shared"), QStringLiteral("0")).toInt();
    } else {
        m_shared = element.attribute(QStringLiteral("origin"), QStringLiteral("local")) != QStringLiteral("local");
    }
    m_coordinator = element.attribute(QStringLiteral("coordinator"));

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("resource-group")) {
            ResourceGroup *child = new ResourceGroup();
            if (child->load(e, status)) {
                addChildGroup(child);
            } else {
                errorPlanXml<<"Failed to load ResourceGroup";
                delete child;
            }
        } else if (status.version() < QStringLiteral("0.7.0") && e.tagName() == QStringLiteral("resource")) {
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
#endif
void ResourceGroup::save(QDomElement &element)  const {
    //debugPlan;

    QDomElement me = element.ownerDocument().createElement(QStringLiteral("resource-group"));
    element.appendChild(me);

    me.setAttribute(QStringLiteral("id"), m_id);
    me.setAttribute(QStringLiteral("name"), m_name);
    me.setAttribute(QStringLiteral("type"), m_type);
    me.setAttribute(QStringLiteral("shared"), m_shared ? QStringLiteral("shared") : QStringLiteral("local"));
    me.setAttribute(QStringLiteral("coordinator"), m_coordinator);

    for (ResourceGroup *g : m_childGroups) {
        g->save(me);
    }
}

void ResourceGroup::initiateCalculation(Schedule &sch) {
    Q_UNUSED(sch)
    clearNodes();
}

int ResourceGroup::units() const {
    int u = 0;
    for (const Resource *r : std::as_const(m_resources)) {
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
    for (Resource *r : std::as_const(m_resources)) {
        a += r->appointmentIntervals();
    }
    return a;
}

DateTime ResourceGroup::startTime(long id) const
{
    DateTime dt;
    for (Resource *r : std::as_const(m_resources)) {
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
    for (Resource *r : std::as_const(m_resources)) {
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
