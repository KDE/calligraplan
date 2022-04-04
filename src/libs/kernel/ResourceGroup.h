/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001 Thomas Zander zander @kde.org
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTRESOURCEGROUP_H
#define KPTRESOURCEGROUP_H

#include "plankernel_export.h"

#include "kptglobal.h"
#include "kptduration.h"
#include "kptdatetime.h"
#include "kptappointment.h"
#include "kptcalendar.h"

#include <KoXmlReaderForward.h>

#include <QHash>
#include <QString>
#include <QList>


/// The main namespace.
namespace KPlato
{

class Account;
class Risk;
class Effort;
class Appointment;
class Task;
class Node;
class Project;
class Resource;
class ResourceRequest;
class ResourceRequestCollection;
class Schedule;
class ResourceSchedule;
class Schedule;
class XMLLoaderObject;
class DateTimeInterval;

/**
  * This class represents a group of similar resources to be assigned to a task
  * e.g. The list of employees, computer resources, etc
  */

/* IDEA; lets create a resourceGroup that has the intelligence to import PIM schedules
 *  from the kroupware project and use the schedules to use the factory pattern to build
 *  Resources (probably a derived class) which returns values on getFirstAvailableTime
 *  and friends based on the schedules we got from the PIM projects.
 *  (Thomas Zander mrt-2003 by suggestion of Shaheed)
 */

class PLANKERNEL_EXPORT ResourceGroup : public QObject
{
    Q_OBJECT
public:
    /// Default constructor
    explicit ResourceGroup();
    explicit ResourceGroup(const ResourceGroup *group);
    ~ResourceGroup() override;

    QString id() const { return m_id; }
    void setId(const QString& id);

    Project *project() const;

    void setName(const QString& n);
    const QString &name() const { return m_name;}
    /// A free-text type
    void setType(const QString &type);
    QString type() const;

    QString coordinator() const;
    void setCoordinator(const QString &coordinator);

    bool isScheduled() const;

    /// Return true if any resource in this group is baselined
    bool isBaselined(long id = BASELINESCHEDULE) const;

    ResourceGroup *parentGroup() const;
    void setParentGroup(ResourceGroup *parent);

    int indexOf(ResourceGroup *group) const;
    int numChildGroups() const;
    void addChildGroup(ResourceGroup *group, int row = -1);
    ResourceGroup *childGroupAt(int i) const;
    QList<ResourceGroup*> childGroups() const;
    void removeChildGroup(ResourceGroup *group);

    /** Manage the resources in this list
     * <p>At some point we will have to look at not mixing types of resources
     * (e.g. you can't add a person to a list of computers
     *
     * <p>Risks must always be associated with a resource, so there is no option
     * to manipulate risks (@ref Risk) separately
         */
    void addResource(Resource *resource, Risk *risk = nullptr);
    void addResource(int index, Resource*, Risk*);
    Resource *takeResource(Resource *resource);
    QList<Resource*> resources() const { return m_resources; }
    int indexOf(const Resource *resource) const;
    Resource *resourceAt(int pos) const { return m_resources.value(pos); }
    int numResources() const { return m_resources.count(); }

    Risk* getRisk(int);

    /** Manage the dependent resources.  This is a list of the resource
     * groups that must have available resources for this resource to
     * perform the work
     * <p>see also @ref getRequiredResource, @ref getRequiredResource
         */
    void addRequiredResource(ResourceGroup*);
    /** Manage the dependent resources.  This is a list of the resource
     * groups that must have available resources for this resource to
     * perform the work
     * <p>see also @ref addRequiredResource, @ref getRequiredResource
         */
    ResourceGroup* getRequiredResource(int);
    /** Manage the dependent resources.  This is a list of the resource
     * groups that must have available resources for this resource to
     * perform the work
     * <p>see also @ref getRequiredResource, @ref addRequiredResource
         */
    void deleteRequiredResource(int);

    bool load(KoXmlElement &element, XMLLoaderObject &status);
    void save(QDomElement &element) const;

    void initiateCalculation(Schedule &sch);

    void addNode(Node *node) { m_nodes.append(node); }
    void clearNodes() { m_nodes.clear(); }

    Calendar *defaultCalendar() { return m_defaultCalendar; }

    int units() const;

    ResourceGroup *findId() const { return findId(m_id); }
    ResourceGroup *findId(const QString &id) const;
    void removeId() { return removeId(m_id); }
    void removeId(const QString &id);
    void insertId(const QString &id);

    Appointment appointmentIntervals() const;

    // m_project is set when the resourcegroup is added to the project,
    // and reset when the resourcegroup is removed from the project
    void setProject(Project *project);

    void copy(const ResourceGroup *group);

    DateTime startTime(long id) const;
    DateTime endTime(long id) const;

    void blockChanged(bool on = true);

    /// A resource can be local to this project, or
    /// defined externally and shared with other projects
    bool isShared() const;
    /// Set resource to be shared if on = true, or local if on = false
    void setShared(bool on);

#ifndef NDEBUG
    void printDebug(const QString& ident);
#endif

Q_SIGNALS:
    void dataChanged(KPlato::ResourceGroup *group);

    void groupToBeAdded(KPlato::Project *project, KPlato::ResourceGroup *parent, int row);
    void groupAdded(KPlato::ResourceGroup *child);
    void groupToBeRemoved(KPlato::Project *project, KPlato::ResourceGroup *parent, int row, KPlato::ResourceGroup *group);
    void groupRemoved();

    void resourceToBeAdded(KPlato::ResourceGroup *group, int row);
    void resourceAdded(KPlato::Resource *resource);
    void resourceToBeRemoved(KPlato::ResourceGroup *group, int row, KPlato::Resource *resource);
    void resourceRemoved();
    
protected:
    virtual void changed();

private:
    Project *m_project;
    QString m_id;   // unique id
    QString m_name;
    ResourceGroup *m_parent;
    QList<ResourceGroup*> m_childGroups;
    QList<Resource*> m_resources;
    QList<Risk*> m_risks;
    QList<ResourceGroup*> m_requires;

    QList<Node*> m_nodes; //The nodes that want resources from us

    Calendar *m_defaultCalendar;
    QString m_type;
    QString m_coordinator;

    bool m_blockChanged;
    bool m_shared;
};

} // namespace KPlato

PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::ResourceGroup *g);

#endif
