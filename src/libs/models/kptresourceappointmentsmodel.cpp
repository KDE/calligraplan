/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007, 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptresourceappointmentsmodel.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptappointment.h"
#include "kptcommand.h"
#include "kpteffortcostmap.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdatetime.h"
#include "kptdebug.h"

#include <KFormat>

#include <QDate>
#include <QList>
#include <QLocale>
#include <QHash>

#include <KGanttGlobal>


namespace KPlato
{

class ItemData
{
public:
    explicit ItemData(const QString &name = QString()) : parent(nullptr), resource(nullptr), group(nullptr), appointment(nullptr), m_name(name) {}
    explicit ItemData(Resource *resource) : parent(nullptr), resource(resource), group(nullptr), appointment(nullptr) {}
    explicit ItemData(ResourceGroup *group) : parent(nullptr), resource(nullptr), group(group), appointment(nullptr) {}
    explicit ItemData(Appointment *a) : parent(nullptr), resource(nullptr), group(nullptr), appointment(a) {}
    ~ItemData() { qDeleteAll(children); }

    QString name() const {
        if (resource) {
            return resource->name();
        }
        if (group) {
            return group->name();
        }
        if (appointment) {
            return appointment->node()->node()->name();
        }
        return m_name;
    }
    int row() const { return parent ? parent->children.indexOf(this) : -1; }

    void setParent(ItemData *parent) {
        if (this->parent) {
            this->parent->children.removeAll(this);
        }
        this->parent = parent;
        if (parent && !parent->children.contains(this)) {
            parent->children.append(this);
        }
    }
    ItemData *parent;
    QList<const ItemData*> children;
    Resource *resource;
    ResourceGroup *group;
    Appointment *appointment;

private:
    QString m_name;

};

ResourceAppointmentsItemModel::ResourceAppointmentsItemModel(QObject *parent)
    : ItemModelBase(parent),
      m_rootItem(new ItemData()),
    m_group(nullptr),
    m_resource(nullptr),
    m_showInternal(true),
    m_showExternal(true)
{
}

ResourceAppointmentsItemModel::~ResourceAppointmentsItemModel()
{
    delete m_rootItem;
}

void ResourceAppointmentsItemModel::slotResourceGroupInserted(KPlato::ResourceGroup *group)
{
    connectSignals(group, true);
    refresh();
}

void ResourceAppointmentsItemModel::slotResourceGroupToBeRemoved(KPlato::Project *project, KPlato::ResourceGroup *parent, int row, KPlato::ResourceGroup *group)
{
    Q_UNUSED(project)
    Q_UNUSED(parent)
    Q_UNUSED(row)
    connectSignals(group, false);
}

void ResourceAppointmentsItemModel::slotResourceToBeInserted(Project *project, int row)
{
    Q_UNUSED(project)
    Q_UNUSED(row)
}

void ResourceAppointmentsItemModel::slotResourceInserted(Resource *resource)
{
    connectSignals(resource, true);
    refresh();
}

void ResourceAppointmentsItemModel::slotResourceToBeRemoved(Project *project, int row, Resource *resource)
{
    Q_UNUSED(project)
    connectSignals(resource, false);
}

void ResourceAppointmentsItemModel::slotResourceRemoved()
{
    refresh();
}

void ResourceAppointmentsItemModel::slotAppointmentToBeInserted(Resource *r, int row)
{
    Q_UNUSED(r);
    Q_UNUSED(row);
}

void ResourceAppointmentsItemModel::slotAppointmentInserted(Resource *r, Appointment *a)
{
    Q_UNUSED(r);
    Q_UNUSED(a);
    refresh();
}

void ResourceAppointmentsItemModel::slotAppointmentToBeRemoved(Resource *r, int row)
{
    Q_UNUSED(r);
    Q_UNUSED(row);
}

void ResourceAppointmentsItemModel::slotAppointmentRemoved()
{
    refresh();
}

void ResourceAppointmentsItemModel::slotAppointmentChanged(Resource *r, Appointment *a)
{
    Q_UNUSED(r)
    Q_UNUSED(a)
    refresh();
}

void ResourceAppointmentsItemModel::slotProjectCalculated(ScheduleManager *sm)
{
    if (sm == m_manager) {
        refresh();
    }
}

void ResourceAppointmentsItemModel::setShowInternalAppointments(bool show)
{
    if (m_showInternal == show) {
        return;
    }
    m_showInternal = show;
    refresh();
}

void ResourceAppointmentsItemModel::setShowExternalAppointments(bool show)
{
    if (m_showExternal == show) {
        return;
    }
    m_showExternal = show;
    refresh();
}

void ResourceAppointmentsItemModel::setProject(Project *project)
{
    Q_UNUSED(project)
    debugPlan;
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ResourceAppointmentsItemModel::projectDeleted);
        disconnect(m_project, &Project::defaultCalendarChanged, this, &ResourceAppointmentsItemModel::slotCalendarChanged);
        disconnect(m_project, &Project::projectCalculated, this, &ResourceAppointmentsItemModel::slotProjectCalculated);
        disconnect(m_project, &Project::scheduleManagerChanged, this, &ResourceAppointmentsItemModel::slotProjectCalculated);

        connect(m_project, &Project::resourceGroupAdded, this, &ResourceAppointmentsItemModel::slotResourceGroupInserted);
        disconnect(m_project, &Project::resourceGroupToBeRemoved, this, &ResourceAppointmentsItemModel::slotResourceGroupToBeRemoved);
        disconnect(m_project, &Project::resourceGroupRemoved, this, &ResourceAppointmentsItemModel::refresh);
        disconnect(m_project, &Project::resourceChanged, this, &ResourceAppointmentsItemModel::slotResourceChanged);
        disconnect(m_project, &Project::resourceToBeAdded, this, &ResourceAppointmentsItemModel::slotResourceToBeInserted);
        disconnect(m_project, &Project::resourceToBeRemoved, this, &ResourceAppointmentsItemModel::slotResourceToBeRemoved);
        disconnect(m_project, &Project::resourceAdded, this, &ResourceAppointmentsItemModel::slotResourceInserted);
        disconnect(m_project, &Project::resourceRemoved, this, &ResourceAppointmentsItemModel::slotResourceRemoved);

        const QList<Resource*> resources = m_project->resourceList();
        for (Resource *r : resources) {
            connectSignals(r, false);
        }
        const QList<ResourceGroup*> groups = m_project->resourceGroups();
        for (auto g : groups) {
            connectSignals(g, false);
        }
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &ResourceAppointmentsItemModel::projectDeleted);
        connect(m_project, &Project::defaultCalendarChanged, this, &ResourceAppointmentsItemModel::slotCalendarChanged);
        connect(m_project, &Project::projectCalculated, this, &ResourceAppointmentsItemModel::slotProjectCalculated);
        connect(m_project, &Project::scheduleManagerChanged, this, &ResourceAppointmentsItemModel::slotProjectCalculated);

        connect(m_project, &Project::resourceGroupAdded, this, &ResourceAppointmentsItemModel::slotResourceGroupInserted);
        connect(m_project, &Project::resourceGroupToBeRemoved, this, &ResourceAppointmentsItemModel::slotResourceGroupToBeRemoved);
        connect(m_project, &Project::resourceGroupRemoved, this, &ResourceAppointmentsItemModel::refresh);
        connect(m_project, &Project::resourceChanged, this, &ResourceAppointmentsItemModel::slotResourceChanged);
        connect(m_project, &Project::resourceToBeAdded, this, &ResourceAppointmentsItemModel::slotResourceToBeInserted);
        connect(m_project, &Project::resourceToBeRemoved, this, &ResourceAppointmentsItemModel::slotResourceToBeRemoved);
        connect(m_project, &Project::resourceAdded, this, &ResourceAppointmentsItemModel::slotResourceInserted);
        connect(m_project, &Project::resourceRemoved, this, &ResourceAppointmentsItemModel::slotResourceRemoved);

        const QList<Resource*> resources = m_project->resourceList();
        for (Resource *r : resources) {
            connectSignals(r, true);
        }
        const QList<ResourceGroup*> groups = m_project->resourceGroups();
        for (auto g : groups) {
            connectSignals(g, true);
        }
    }
    refresh();
}

void ResourceAppointmentsItemModel::connectSignals(ResourceGroup *group, bool enable)
{
    if (enable) {
        connect(group, &ResourceGroup::dataChanged, this, &ResourceAppointmentsItemModel::refresh);
        connect(group, &ResourceGroup::groupAdded, this, &ResourceAppointmentsItemModel::slotResourceGroupInserted);
        connect(group, &ResourceGroup::groupToBeRemoved, this, &ResourceAppointmentsItemModel::slotResourceGroupToBeRemoved);
        connect(group, &ResourceGroup::groupRemoved, this, &ResourceAppointmentsItemModel::refresh);
        connect(group, &ResourceGroup::resourceAdded, this, &ResourceAppointmentsItemModel::refresh);
        connect(group, &ResourceGroup::resourceRemoved, this, &ResourceAppointmentsItemModel::refresh);
    } else {
        disconnect(group, &ResourceGroup::dataChanged, this, &ResourceAppointmentsItemModel::refresh);
        disconnect(group, &ResourceGroup::groupAdded, this, &ResourceAppointmentsItemModel::slotResourceGroupInserted);
        disconnect(group, &ResourceGroup::groupToBeRemoved, this, &ResourceAppointmentsItemModel::slotResourceGroupToBeRemoved);
        disconnect(group, &ResourceGroup::groupRemoved, this, &ResourceAppointmentsItemModel::refresh);
        disconnect(group, &ResourceGroup::resourceAdded, this, &ResourceAppointmentsItemModel::refresh);
        disconnect(group, &ResourceGroup::resourceRemoved, this, &ResourceAppointmentsItemModel::refresh);
    }
    const QList<ResourceGroup*> groups = group->childGroups();
    for (auto g : groups) {
        connectSignals(g, enable);
    }

}

void ResourceAppointmentsItemModel::connectSignals(Resource *resource, bool enable)
{
    if (enable) {
        connect(resource, &Resource::externalAppointmentToBeAdded, this, &ResourceAppointmentsItemModel::slotAppointmentToBeInserted, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(resource, &Resource::externalAppointmentAdded, this, &ResourceAppointmentsItemModel::slotAppointmentInserted, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(resource, &Resource::externalAppointmentToBeRemoved, this, &ResourceAppointmentsItemModel::slotAppointmentToBeRemoved, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(resource, &Resource::externalAppointmentRemoved, this, &ResourceAppointmentsItemModel::slotAppointmentRemoved, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(resource, &Resource::externalAppointmentChanged, this, &ResourceAppointmentsItemModel::slotAppointmentChanged, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    } else {
        disconnect(resource, &Resource::externalAppointmentToBeAdded, this, &ResourceAppointmentsItemModel::slotAppointmentToBeInserted);
        disconnect(resource, &Resource::externalAppointmentAdded, this, &ResourceAppointmentsItemModel::slotAppointmentInserted);
        disconnect(resource, &Resource::externalAppointmentToBeRemoved, this, &ResourceAppointmentsItemModel::slotAppointmentToBeRemoved);
        disconnect(resource, &Resource::externalAppointmentRemoved, this, &ResourceAppointmentsItemModel::slotAppointmentRemoved);
        disconnect(resource, &Resource::externalAppointmentChanged, this, &ResourceAppointmentsItemModel::slotAppointmentChanged);
    }
}

QDate ResourceAppointmentsItemModel::startDate() const
{
    if (m_project && m_manager) {
        return m_project->startTime(id()).date();
    }
    return QDate::currentDate();
}

QDate ResourceAppointmentsItemModel::endDate() const
{
    if (m_project && m_manager) {
        return m_project->endTime(id()).date();
    }
    return QDate::currentDate();
}

void ResourceAppointmentsItemModel::setScheduleManager(ScheduleManager *sm)
{
    if (sm == m_manager) {
        return;
    }
    debugPlan<<sm;
    m_manager = sm;
    refresh();
}

long ResourceAppointmentsItemModel::id() const
{
    return m_manager == nullptr ? -1 : m_manager->scheduleId();
}

Qt::ItemFlags ResourceAppointmentsItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = ItemModelBase::flags(index);
    return flags &= ~Qt::ItemIsEditable;
}


QModelIndex ResourceAppointmentsItemModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid() || m_project == nullptr || m_manager == nullptr) {
        return QModelIndex();
    }
    if (idx.internalPointer() == nullptr) {
        return QModelIndex();
    }
    const auto item = static_cast<ItemData*>(idx.internalPointer());
    return createIndex(item->parent->row(), 0, item->parent);
}

Resource *ResourceAppointmentsItemModel::parent(const Appointment *a) const
{
    if (a == nullptr || m_project == nullptr) {
        return nullptr;
    }
    const QList<Resource*> resources = m_project->resourceList();
    for (Resource *r : resources) {
        if (r->appointments(id()).contains(const_cast<Appointment*>(a))) {
            return r;
        }
        if (r->externalAppointmentList().contains(const_cast<Appointment*>(a))) {
            return r;
        }
    }
    return nullptr;
}

QModelIndex ResourceAppointmentsItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr) {
        return QModelIndex();
    }
    const auto item = parent.isValid() ? static_cast<ItemData*>(parent.internalPointer()) : m_rootItem;
    return createIndex(row, column, (void*)item->children.value(row));
}

void ResourceAppointmentsItemModel::addResource(Resource *resource, ItemData *parentItem)
{
    if (resource->type() == Resource::Type_Team) {
        return;
    }
    auto ritem = new ItemData(resource);
    ritem->setParent(parentItem);
    const auto appointments = resource->appointments(id());
    for (int row = 0; row < appointments.count(); ++row) {
        const auto a = appointments.at(row);
        auto aitem = new ItemData(a);
        aitem->setParent(ritem);
    }
}

void ResourceAppointmentsItemModel::addGroup(ResourceGroup *group, ItemData *parentItem)
{
    auto gitem = new ItemData(group);
    gitem->setParent(parentItem);
    const auto resources = group->resources();
    for (auto r : resources) {
        addResource(r, gitem);
    }
    const auto groups = group->childGroups();
    for (auto g : groups) {
        addGroup(g, gitem);
    }
}

void ResourceAppointmentsItemModel::refresh()
{
    beginResetModel();
    refreshData();
    delete m_rootItem;
    m_rootItem = new ItemData();
    auto projectItem = new ItemData(i18n("Project"));
    projectItem->setParent(m_rootItem);
    const auto resources = m_project->resourceList();
    for (int i = 0; i < resources.count(); ++i) {
        const auto r = resources.at(i);
        addResource(r, projectItem);
    }
    const auto groups = m_project->resourceGroups();
    for (auto g : groups) {
        addGroup(g, m_rootItem);
    }
    endResetModel();
    Q_EMIT refreshed();
}

void ResourceAppointmentsItemModel::refreshData()
{
    //debugPlan<<"Schedule id: "<<id<<'\n';
    QDate start;
    QDate end;
    QHash<const Appointment*, EffortCostMap> ec;
    QHash<const Appointment*, EffortCostMap> extEff;
    const QList<Resource*> resources = m_project->resourceList();
    for (Resource *r : resources) {
        const QList<Appointment*> appointments = r->appointments(id());
        for (Appointment* a : appointments) {
            QDate s = a->startTime().date();
            QDate e = a->endTime().date();
            ec[ a ] = a->plannedPrDay(s, e);
            if (! start.isValid() || s < start) {
                start = s;
            }
            if (! end.isValid() || e > end) {
                end = e;
            }
            //debugPlan<<a->node()->node()->name()<<": "<<s<<e<<": "<<m_effortMap[ a ].totalEffort().toDouble(Duration::Unit_h);
        }
        // add external appointments
        const QList<Appointment*> appointments2 = r->externalAppointmentList();
        for (Appointment* a : appointments2) {
            extEff[ a ] = a->plannedPrDay(startDate(), endDate());
        }
    }
    m_effortMap.clear();
    m_externalEffortMap.clear();
    m_effortMap = ec;
    m_externalEffortMap = extEff;
    return;
}

int ResourceAppointmentsItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 3 + startDate().daysTo(endDate());
}

int ResourceAppointmentsItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == nullptr) {
        return 0;
    }
    const auto item = parent.isValid() ? static_cast<const ItemData*>(parent.internalPointer()) : m_rootItem;
    return item->children.count();
}

QVariant ResourceAppointmentsItemModel::total(const ItemData *item, int role) const
{
    if (!m_project) {
        return QVariant();
    }
    if (item->resource) {
        return total(item->resource, role);
    }
    if (item->group) {
        return total(item->group, role);
    }
    if (item->appointment) {
        return total(item->appointment, role);
    }
    if (role == Qt::DisplayRole) {
        double tot = 0.0;
        const auto resources = m_project->resourceList();
        for (const auto r : resources) {
            if (r->type() != Resource::Type_Team) {
                tot += total(r, Qt::EditRole).toDouble();
            }
        }
        return QLocale().toString(tot, 'f', 1);
    }
    return QVariant();
}

QVariant ResourceAppointmentsItemModel::total(const ItemData *item, const QDate &date, int role) const
{
    if (!m_project) {
        return QVariant();
    }
    if (item->resource) {
        return total(item->resource, date, role);
    }
    if (item->group) {
        return total(item->group, date, role);
    }
    if (item->appointment) {
        return assignment(item->appointment, date, role);
    }
    if (role == Qt::DisplayRole) {
        double tot = 0.0;
        const auto resources = m_project->resourceList();
        for (const auto r : resources) {
            if (r->type() != Resource::Type_Team) {
                tot += total(r, date, Qt::EditRole).toDouble();
            }
        }
        return QLocale().toString(tot, 'f', 1);
    }
    return QVariant();
}

QVariant ResourceAppointmentsItemModel::total(const ResourceGroup *group, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            const double tot = total(group, Qt::EditRole).toDouble();
            return QLocale().toString(tot, 'f', 1);
        }
    case Qt::EditRole: {
        double tot = 0.0;
        const auto groups = group->childGroups();
        for (const auto g : groups) {
            tot += total(g, Qt::EditRole).toDouble();
        }
        const auto resources = group->resources();
        for (const auto r : resources) {
            tot += total(r, Qt::EditRole).toDouble();
        }
        return tot;
    }
    case Qt::TextAlignmentRole:
        return (int)(Qt::AlignRight|Qt::AlignVCenter);
    }
    return QVariant();
}

QVariant ResourceAppointmentsItemModel::total(const ResourceGroup *group, const QDate &date, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            double tot = 0.0;
            const auto resources = group->resources();
            for (const auto r : resources) {
                tot += total(r, date, Qt::EditRole).toDouble();
            }
            return QLocale().toString(tot, 'f', 1);
        }
        case Qt::TextAlignmentRole:
            return (int)(Qt::AlignRight|Qt::AlignVCenter);
        default:
            break;
    }
    return QVariant();
}

QVariant ResourceAppointmentsItemModel::total(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            Duration d;
            if (m_showInternal) {
                const QList<Appointment*> lst = res->appointments(id());
                for (Appointment *a : lst) {
                    if (m_effortMap.contains(a)) {
                        d += m_effortMap[ a ].totalEffort();
                    }
                }
            }
            if (m_showExternal) {
                const QList<Appointment*> lst = res->externalAppointmentList();
                for (Appointment *a : lst) {
                    if (m_externalEffortMap.contains(a)) {
                        d += m_externalEffortMap[ a ].totalEffort();
                    }
                }
            }
            return QLocale().toString(d.toDouble(Duration::Unit_h), 'f', 1);
        }
        case Qt::EditRole: {
            Duration d;
            if (m_showInternal) {
                const QList<Appointment*> lst = res->appointments(id());
                for (Appointment *a : lst) {
                    if (m_effortMap.contains(a)) {
                        d += m_effortMap[ a ].totalEffort();
                    }
                }
            }
            if (m_showExternal) {
                const QList<Appointment*> lst = res->externalAppointmentList();
                for (Appointment *a : lst) {
                    if (m_externalEffortMap.contains(a)) {
                        d += m_externalEffortMap[ a ].totalEffort();
                    }
                }
            }
            return d.toDouble(Duration::Unit_h);
        }
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::TextAlignmentRole:
            return (int)(Qt::AlignRight|Qt::AlignVCenter);
    }
    return QVariant();
}

QVariant ResourceAppointmentsItemModel::total(const Resource *res, const QDate &date, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            Duration d;
            if (m_showInternal) {
                const QList<Appointment*> lst = res->appointments(id());
                for (Appointment *a : lst) {
                    if (m_effortMap.contains(a)) {
                        d += m_effortMap[ a ].effortOnDate(date);
                    }
                }
            }
            if (m_showExternal) {
                const QList<Appointment*> lst = res->externalAppointmentList();
                for (Appointment *a : lst) {
                    if (m_externalEffortMap.contains(a)) {
                        d += m_externalEffortMap[ a ].effortOnDate(date);
                    }
                }
            }
            QString ds = QLocale().toString(d.toDouble(Duration::Unit_h), 'f', 1);
            Duration avail = res->effort(nullptr, DateTime(date, QTime(0,0,0)), Duration(1.0, Duration::Unit_d));
            QString avails = QLocale().toString(avail.toDouble(Duration::Unit_h), 'f', 1);
            return QStringLiteral("%1(%2)").arg(ds).arg(avails);
        }
        case Qt::EditRole: {
            Duration d;
            if (m_showInternal) {
                const QList<Appointment*> lst = res->appointments(id());
                for (Appointment *a : lst) {
                    if (m_effortMap.contains(a)) {
                        d += m_effortMap[ a ].effortOnDate(date);
                    }
                }
            }
            if (m_showExternal) {
                const QList<Appointment*> lst = res->externalAppointmentList();
                for (Appointment *a : lst) {
                    if (m_externalEffortMap.contains(a)) {
                        d += m_externalEffortMap[ a ].effortOnDate(date);
                    }
                }
            }
            return d.toDouble(Duration::Unit_h);
        }
        case Qt::ToolTipRole:
            return i18n("The total booking on %1, along with the maximum hours for the resource", QLocale().toString(date, QLocale::ShortFormat));
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::TextAlignmentRole:
            return (int)(Qt::AlignRight|Qt::AlignVCenter);
        case Qt::BackgroundRole: {
            if (date.isValid() && res->calendar() && res->calendar()->state(date) != CalendarDay::Working) {
                QColor c(0xf0f0f0);
                return QVariant::fromValue(c);
                //return QVariant(Qt::cyan);
            }
            break;
        }
        case Role::Maximum:
            return res->effort(nullptr, DateTime(date, QTime(0,0,0)), Duration(1.0, Duration::Unit_d)).toDouble(Duration::Unit_h);
    }
    return QVariant();
}

QVariant ResourceAppointmentsItemModel::total(const Appointment *a, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            Duration d;
            if (m_effortMap.contains(a)) {
                d = m_effortMap[ a ].totalEffort();
            } else if (m_externalEffortMap.contains(a)) {
                d = m_externalEffortMap[ a ].totalEffort();
            }
            return QLocale().toString(d.toDouble(Duration::Unit_h), 'f', 1);
        }
        case Qt::ToolTipRole: {
            if (m_effortMap.contains(a)) {
                Node *n = a->node()->node();
                const auto start = a->startTime();
                return xi18nc("@info:tooltip", "%1: %2<nl/>%3: %4<nl/>Timezone: %5<nl/>Available: %6",
                            n->wbsCode(),
                            n->name(),
                            QLocale().toString(a->startTime(), QLocale::ShortFormat),
                            KFormat().formatSpelloutDuration((a->endTime() - a->startTime()).milliseconds()),
                            QLatin1String(start.timeZone().id()),
                            a->resource()->resource()->units()
                );
            } else if (m_externalEffortMap.contains(a)) {
                return i18n("Total booking by the external project");
            }
            return QVariant();
        }
        case Qt::EditRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::TextAlignmentRole:
            return (int)(Qt::AlignRight|Qt::AlignVCenter);
        case Qt::ForegroundRole:
            if (m_externalEffortMap.contains(a)) {
                return QColor(Qt::blue);
            }
            break;
    }
    return QVariant();
}


QVariant ResourceAppointmentsItemModel::assignment(const Appointment *a, const QDate &date, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            Duration d;
            if (m_effortMap.contains(a)) {
                if (date < m_effortMap[ a ].startDate() || date > m_effortMap[ a ].endDate()) {
                    return QVariant();
                }
                d = m_effortMap[ a ].effortOnDate(date);
                return QLocale().toString(d.toDouble(Duration::Unit_h), 'f', 1);
            } else  if (m_externalEffortMap.contains(a)) {
                if (date < m_externalEffortMap[ a ].startDate() || date > m_externalEffortMap[ a ].endDate()) {
                    return QVariant();
                }
                d = m_externalEffortMap[ a ].effortOnDate(date);
                return QLocale().toString(d.toDouble(Duration::Unit_h), 'f', 1);
            }
            return QVariant();
        }
        case Qt::EditRole: {
            Duration d;
            if (m_effortMap.contains(a)) {
                if (date < m_effortMap[ a ].startDate() || date > m_effortMap[ a ].endDate()) {
                    return QVariant();
                }
                d = m_effortMap[ a ].effortOnDate(date);
                return d.toDouble(Duration::Unit_h);
            } else  if (m_externalEffortMap.contains(a)) {
                if (date < m_externalEffortMap[ a ].startDate() || date > m_externalEffortMap[ a ].endDate()) {
                    return QVariant();
                }
                d = m_externalEffortMap[ a ].effortOnDate(date);
                return d.toDouble(Duration::Unit_h);
            }
            return QVariant();
        }
        case Qt::ToolTipRole: {
            if (m_effortMap.contains(a)) {
                return i18n("Booking by this task on %1", QLocale().toString(date, QLocale::ShortFormat));
            } else if (m_externalEffortMap.contains(a)) {
                return i18n("Booking by external project on %1",QLocale().toString(date, QLocale::ShortFormat));
            }
            return QVariant();
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::TextAlignmentRole:
            return (int)(Qt::AlignRight|Qt::AlignVCenter);
        case Qt::ForegroundRole:
            if (m_externalEffortMap.contains(a)) {
                return QColor(Qt::blue);
            }
            break;
        case Qt::BackgroundRole: {
            Resource *r = parent(a);
            if (r && r->calendar() && r->calendar()->state(date) != CalendarDay::Working) {
                QColor c(0xf0f0f0);
                return QVariant::fromValue(c);
                //return QVariant(Qt::cyan);
            }
            break;
        }
    }
    return QVariant();
}

QVariant ResourceAppointmentsItemModel::data(const QModelIndex &index, int role) const
{
    if (m_project == nullptr) {
        return QVariant();
    }
    QVariant result;
    if (index.column() >= columnCount()) {
        debugPlan<<"invalid display value column "<<index;
        return result;
    }
    if (! index.isValid()) {
        debugPlan<<"Invalid index:"<<index;
        return result;
    }
    if (role == Qt::TextAlignmentRole) {
        return headerData(index.column(), Qt::Horizontal, role);
    }
    if (index.column() == 0) {
        if (role == Qt::DisplayRole) {
            return static_cast<const ItemData*>(index.internalPointer())->name();
        }
        return QVariant();
    }
    if (index.column() == 1) {
        return total(static_cast<const ItemData*>(index.internalPointer()), role);
    }
    QDate d = startDate().addDays(index.column() - 2);
    return total(static_cast<const ItemData*>(index.internalPointer()), d, role);
}

bool ResourceAppointmentsItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (! index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    if ((flags(index) &Qt::ItemIsEditable) == 0 || role != Qt::EditRole) {
        return false;
    }
    Resource *r = resource(index);
    if (r) {
        switch (index.column()) {
            default:
                qWarning("data: invalid display value column %d", index.column());
                break;
        }
        return false;
    }
    return false;
}

QVariant ResourceAppointmentsItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section) {
                case 0: return i18n("Name");
                case 1: return i18n("Total");
                default: {
                    //debugPlan<<section<<", "<<startDate()<<endDate();
                    if (section < columnCount()) {
                        QDate d = startDate().addDays(section - 2);
                        if (d <= endDate()) {
                            return d;
                        }
                    }
                    return QVariant();
                }
            }
        } else if (role == Qt::ToolTipRole) {
            switch (section) {
                case 0: return i18n("Name");
                case 1: return i18n("The total hours booked");
                default: {
                    //debugPlan<<section<<", "<<startDate()<<endDate();
                    QDate d = startDate().addDays(section - 2);
                    return i18n("Bookings on %1", QLocale().toString(d, QLocale::ShortFormat));
                }
                return QVariant();
            }
        } else if (role == Qt::TextAlignmentRole) {
            switch (section) {
                case 0: return QVariant();
                default: return (int)(Qt::AlignRight|Qt::AlignVCenter);
            }
        }
    }
    if (role == Qt::ToolTipRole) {
        switch (section) {
            default: return QVariant();
        }
    }
    return ItemModelBase::headerData(section, orientation, role);
}

Node *ResourceAppointmentsItemModel::node(const QModelIndex &index) const
{
    Appointment *a = appointment(index);
    if (a == nullptr) {
        return nullptr;
    }
    return a->node()->node();
}

Appointment *ResourceAppointmentsItemModel::appointment(const QModelIndex &index) const
{
    if (m_project == nullptr || m_manager == nullptr || !index.isValid()) {
        return nullptr;
    }
    return static_cast<const ItemData*>(index.internalPointer())->appointment;
}

Appointment *ResourceAppointmentsItemModel::externalAppointment(const QModelIndex &index) const
{
    if (m_project == nullptr || m_manager == nullptr) {
        return nullptr;
    }
    const QList<Resource*> resources = m_project->resourceList();
    for (Resource *r : resources) {
        const QList<Appointment*> appointments = r->externalAppointmentList();
        for (Appointment *a : appointments) {
            if (a == index.internalPointer()) {
                return a;
            }
        }
    }
    return nullptr;
}

QModelIndex ResourceAppointmentsItemModel::createAppointmentIndex(int row, int col, void *ptr) const
{
    return createIndex(row, col, ptr);
}

QModelIndex ResourceAppointmentsItemModel::createExternalAppointmentIndex(int row, int col, void *ptr) const
{
    if (m_project == nullptr || m_manager == nullptr) {
        return QModelIndex();
    }
    QModelIndex i = createIndex(row, col, ptr);
    //debugPlan<<i;
    return i;
}

Resource *ResourceAppointmentsItemModel::resource(const QModelIndex &index) const
{
    if (m_project == nullptr || !index.isValid()) {
        return nullptr;
    }
    return static_cast<const ItemData*>(index.internalPointer())->resource;
}

QModelIndex ResourceAppointmentsItemModel::createResourceIndex(int row, int col, Resource *ptr) const
{
    return createIndex(row, col, ptr);
}

void ResourceAppointmentsItemModel::slotCalendarChanged(Calendar*)
{
    const QList<Resource*> resources = m_project->resourceList();
    for (Resource *r : resources) {
        if (r->calendar(true) == nullptr) {
            slotResourceChanged(r);
        }
    }
}

void ResourceAppointmentsItemModel::slotResourceChanged(Resource *res)
{
    int row = m_project->indexOf(res);
    Q_EMIT dataChanged(createResourceIndex(row, 0, res), createResourceIndex(row, columnCount() - 1, res));
}

//-------------------------------------------------------
class Q_DECL_HIDDEN ResourceAppointmentsRowModel::Private
{
public:
    Private(Private *par=nullptr, void *p=nullptr, KPlato::ObjectType t=OT_None) : parent(par), ptr(p), type(t), internalCached(false), externalCached(false), intervalRow(-1)
    {}
    ~Private()
    {
        qDeleteAll(intervals);
    }

    QVariant data(int column, long id = -1, int role = Qt::DisplayRole) const;

    Private *parent;
    void *ptr;
    KPlato::ObjectType type;
    bool internalCached;
    bool externalCached;

    Private *intervalAt(int row) const;
    // used by interval
    AppointmentInterval interval;

protected:
    QVariant resourceData(int column, long id, int role) const;
    QVariant appointmentData(int column, int role) const;
    QVariant externalData(int column, int role) const;
    QVariant intervalData(int column, int role) const;

private:
    // used by resource
    Appointment internal;
    Appointment external;

    // used by appointment
    int intervalRow;
    mutable QMap<int, Private*> intervals;
};

QVariant ResourceAppointmentsRowModel::Private::data(int column, long id, int role) const
{
    if (role == Role::ObjectType) {
        return (int)type;
    }
    switch (type) {
        case OT_Resource: return resourceData(column, id, role);
        case OT_Appointment: return appointmentData(column, role);
        case OT_External: return externalData(column, role);
        case OT_Interval: return intervalData(column, role);
        default: break;
    }
    return QVariant();
}

QVariant ResourceAppointmentsRowModel::Private::resourceData(int column, long id, int role) const
{
    KPlato::Resource *r = static_cast<KPlato::Resource*>(ptr);
    if (role == Qt::DisplayRole) {
        switch (column) {
            case ResourceAppointmentsRowModel::Name: return r->name();
            case ResourceAppointmentsRowModel::Type: return r->typeToString(true);
            case ResourceAppointmentsRowModel::StartTime: return QStringLiteral(" ");
            case ResourceAppointmentsRowModel::EndTime: return QStringLiteral(" ");
            case ResourceAppointmentsRowModel::Load: return QStringLiteral(" ");
        }
    } else if (role == Role::Maximum) {
        return r->units(); //TODO: Maximum Load
    } else if (role == Role::InternalAppointments) {
        if (! internalCached) {
            Resource *r = static_cast<Resource*>(ptr);
            const_cast<Private*>(this)->internal.clear();
            const QList<Appointment*> appointments = r->appointments(id);
            for (Appointment *a : appointments) {
                const_cast<Private*>(this)->internal += *a;
            }
            const_cast<Private*>(this)->internalCached = true;
        }
        return QVariant::fromValue((void*)(&internal));
    } else if (role == Role::ExternalAppointments) {
        if (! externalCached) {
            Resource *r = static_cast<Resource*>(ptr);
            const_cast<Private*>(this)->external.clear();
            const QList<Appointment*> appointments = r->externalAppointmentList();
            for (Appointment *a : appointments) {
                Appointment e;
                e.setIntervals(a->intervals(r->startTime(id), r->endTime(id)));
                const_cast<Private*>(this)->external += e;
            }
            const_cast<Private*>(this)->externalCached = true;
        }
        return QVariant::fromValue((void*)(&external));
    }
    return QVariant();
}

QVariant ResourceAppointmentsRowModel::Private::appointmentData(int column, int role) const
{
    KPlato::Appointment *a = static_cast<KPlato::Appointment*>(ptr);
    if (role == Qt::DisplayRole) {
        switch (column) {
            case ResourceAppointmentsRowModel::Name: return a->node()->node()->name();
            case ResourceAppointmentsRowModel::Type: return a->node()->node()->typeToString(true);
            case ResourceAppointmentsRowModel::StartTime: return QLocale().toString(a->startTime(), QLocale::ShortFormat);
            case ResourceAppointmentsRowModel::EndTime: return QLocale().toString(a->endTime(), QLocale::ShortFormat);
            case ResourceAppointmentsRowModel::Load: return QStringLiteral(" ");
        }
    } else if (role == Qt::ToolTipRole) {
        const auto n = a->node()->node();
        const auto start = a->startTime();
        return xi18nc("@info:tooltip", "%1: %2<nl/>%3: %4<nl/>Timezone: %5",
                                n->wbsCode(),
                                n->name(),
                                QLocale().toString(start, QLocale::ShortFormat),
                                KFormat().formatSpelloutDuration((a->endTime() - start).milliseconds()),
                                QLatin1String(start.timeZone().id())
                            );
    } else if (role == Role::Maximum) {
        return a->resource()->resource()->units(); //TODO: Maximum Load
    }
    return QVariant();
}

QVariant ResourceAppointmentsRowModel::Private::externalData(int column, int role) const
{
    KPlato::Appointment *a = static_cast<KPlato::Appointment*>(ptr);
    if (role == Qt::DisplayRole) {
        switch (column) {
            case ResourceAppointmentsRowModel::Name: return a->auxcilliaryInfo();
            case ResourceAppointmentsRowModel::Type: return i18n("Project");
            case ResourceAppointmentsRowModel::StartTime: return QLocale().toString(a->startTime(), QLocale::ShortFormat);
            case ResourceAppointmentsRowModel::EndTime: return QLocale().toString(a->endTime(), QLocale::ShortFormat);
            case ResourceAppointmentsRowModel::Load: return QStringLiteral(" ");
        }
    } else if (role == Qt::ForegroundRole) {
        return QColor(Qt::blue);
    } else if (role == Role::Maximum) {
        KPlato::Resource *r = static_cast<KPlato::Resource*>(parent->ptr);
        return r->units(); //TODO: Maximum Load
    }
    return QVariant();
}

ResourceAppointmentsRowModel::Private *ResourceAppointmentsRowModel::Private::intervalAt(int row) const
{
    Q_ASSERT(type == OT_Appointment || type == OT_External);
    Private *p = intervals.value(row);
    if (p) {
        return p;
    }
    Appointment *a = static_cast<Appointment*>(ptr);
    p = new Private(const_cast<Private*>(this), nullptr, OT_Interval);
    p->intervalRow = row;
    p->interval = a->intervalAt(row);
    intervals.insert(row, p);
    return p;
}


QVariant ResourceAppointmentsRowModel::Private::intervalData(int column, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (column) {
            case ResourceAppointmentsRowModel::Name: return QVariant();
            case ResourceAppointmentsRowModel::Type: return i18n("Interval");
            case ResourceAppointmentsRowModel::StartTime: return QLocale().toString(interval.startTime(), QLocale::ShortFormat);
            case ResourceAppointmentsRowModel::EndTime: return QLocale().toString(interval.endTime(), QLocale::ShortFormat);
            case ResourceAppointmentsRowModel::Load: return interval.load();
        }
    } else if (role == Qt::ToolTipRole) {
        Appointment *a = static_cast<Appointment*>(parent->ptr);
        if (a && a->node() && a->node()->node()) {
            const auto n = a->node()->node();
            const auto start = interval.startTime();
            const auto end = interval.endTime();
            return xi18nc("@info:tooltip", "%1: %2<nl/>%3: %4<nl/>Timezone: %5<nl/>Assigned: %6<nl/>Available: %7",
                           n->wbsCode(),
                           n->name(),
                           QLocale().toString(start, QLocale::ShortFormat),
                           KFormat().formatSpelloutDuration((end - start).milliseconds()),
                           QLatin1String(start.timeZone().id()),
                           interval.load(),
                           a->resource()->resource()->units()
            );
        }
    } else if (role == Role::Maximum) {
        return parent->appointmentData(column, role);
    }
    return QVariant();
}

int ResourceAppointmentsRowModel::sortRole(int column) const
{
    switch (column) {
        case ResourceAppointmentsRowModel::StartTime:
        case ResourceAppointmentsRowModel::EndTime:
            return Qt::EditRole;
        default:
            break;
    }
    return Qt::DisplayRole;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, KPlato::ObjectType t)
{
  switch(t){
    case KPlato::OT_None:           dbg << "None"; break;
    case KPlato::OT_ResourceGroup:  dbg << "Group"; break;
    case KPlato::OT_Resource:       dbg << "Resource"; break;
    case KPlato::OT_Appointment:    dbg << "Appointment"; break;
    case KPlato::OT_External:       dbg << "External"; break;
    case KPlato::OT_Interval:       dbg << "Interval"; break;
    default: dbg << "Unknown";
  }
  return dbg;
}
QDebug operator<<(QDebug dbg, const ResourceAppointmentsRowModel::Private& s)
{
    dbg <<&s;
    return dbg;
}
QDebug operator<<(QDebug dbg, const ResourceAppointmentsRowModel::Private* s)
{
    if (s == nullptr) {
        dbg<<"ResourceAppointmentsRowModel::Private[ ("<<(void*)s<<") ]";
    } else {
        dbg << "ResourceAppointmentsRowModel::Private[ ("<<(void*)s<<") Type="<<s->type<<" parent=";
        switch(s->type) {
        case KPlato::OT_ResourceGroup:
            dbg<<static_cast<ResourceGroup*>(s->ptr)->project()<<static_cast<ResourceGroup*>(s->ptr)->project()->name();
            dbg<<" ptr="<<static_cast<ResourceGroup*>(s->ptr)<<static_cast<ResourceGroup*>(s->ptr)->name();
            break;
        case KPlato::OT_Resource:
            dbg<<static_cast<ResourceGroup*>(s->parent->ptr)<<static_cast<ResourceGroup*>(s->parent->ptr)->name();
            dbg<<" ptr="<<static_cast<Resource*>(s->ptr)<<static_cast<Resource*>(s->ptr)->name();
            break;
        case KPlato::OT_Appointment:
        case KPlato::OT_External:
            dbg<<static_cast<Resource*>(s->parent->ptr)<<static_cast<Resource*>(s->parent->ptr)->name();
            dbg<<" ptr="<<static_cast<Appointment*>(s->ptr);
            break;
        case KPlato::OT_Interval:
            dbg<<static_cast<Appointment*>(s->parent->ptr)<<" ptr="<<static_cast<AppointmentInterval*>(s->ptr);
            break;
        default:
            dbg<<s->parent<<" ptr="<<s->ptr;
            break;
        }
        dbg<<" ]";
    }

    return dbg;
}
#endif

ResourceAppointmentsRowModel::ResourceAppointmentsRowModel(QObject *parent)
    : ItemModelBase(parent),
    m_schedule(nullptr)
{
}

ResourceAppointmentsRowModel::~ResourceAppointmentsRowModel()
{
    qDeleteAll(m_datamap);
}

void ResourceAppointmentsRowModel::setProject(Project *project)
{
    Q_UNUSED(project)
    beginResetModel();
    //debugPlan<<project;
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ResourceAppointmentsRowModel::projectDeleted);
        disconnect(m_project, &Project::projectCalculated, this, &ResourceAppointmentsRowModel::slotProjectCalculated);

        disconnect(m_project, &Project::resourceToBeAdded, this, &ResourceAppointmentsRowModel::slotResourceToBeInserted);
        disconnect(m_project, &Project::resourceAdded, this, &ResourceAppointmentsRowModel::slotResourceInserted);
        disconnect(m_project, &Project::resourceToBeRemoved, this, &ResourceAppointmentsRowModel::slotResourceToBeRemoved);
        disconnect(m_project, &Project::resourceRemoved, this, &ResourceAppointmentsRowModel::slotResourceRemoved);

        const auto resourceList = m_project->resourceList();
        for (Resource *r : resourceList) {
            connectSignals(r, false);
        }
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &ResourceAppointmentsRowModel::projectDeleted);
        connect(m_project, &Project::projectCalculated, this, &ResourceAppointmentsRowModel::slotProjectCalculated);

        connect(m_project, &Project::resourceToBeAdded, this, &ResourceAppointmentsRowModel::slotResourceToBeInserted);
        connect(m_project, &Project::resourceAdded, this, &ResourceAppointmentsRowModel::slotResourceInserted);
        connect(m_project, &Project::resourceToBeRemoved, this, &ResourceAppointmentsRowModel::slotResourceToBeRemoved);
        connect(m_project, &Project::resourceRemoved, this, &ResourceAppointmentsRowModel::slotResourceRemoved);

        const auto resourceList = m_project->resourceList();
        for (Resource *r : resourceList) {
            connectSignals(r, true);
        }
    }
    endResetModel();
}

void ResourceAppointmentsRowModel::connectSignals(Resource *resource, bool enable)
{
    if (enable) {
        connect(resource, &Resource::externalAppointmentToBeAdded, this, &ResourceAppointmentsRowModel::slotAppointmentToBeInserted, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(resource, &Resource::externalAppointmentAdded, this, &ResourceAppointmentsRowModel::slotAppointmentInserted, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(resource, &Resource::externalAppointmentToBeRemoved, this, &ResourceAppointmentsRowModel::slotAppointmentToBeRemoved, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(resource, &Resource::externalAppointmentRemoved, this, &ResourceAppointmentsRowModel::slotAppointmentRemoved, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(resource, &Resource::externalAppointmentChanged, this, &ResourceAppointmentsRowModel::slotAppointmentChanged, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    } else {
        disconnect(resource, &Resource::externalAppointmentToBeAdded, this, &ResourceAppointmentsRowModel::slotAppointmentToBeInserted);
        disconnect(resource, &Resource::externalAppointmentAdded, this, &ResourceAppointmentsRowModel::slotAppointmentInserted);
        disconnect(resource, &Resource::externalAppointmentToBeRemoved, this, &ResourceAppointmentsRowModel::slotAppointmentToBeRemoved);
        disconnect(resource, &Resource::externalAppointmentRemoved, this, &ResourceAppointmentsRowModel::slotAppointmentRemoved);
        disconnect(resource, &Resource::externalAppointmentChanged, this, &ResourceAppointmentsRowModel::slotAppointmentChanged);
    }
}

void ResourceAppointmentsRowModel::setScheduleManager(ScheduleManager *sm)
{
    debugPlan<<"ResourceAppointmentsRowModel::setScheduleManager:"<<sm;
    if (sm == nullptr || sm != m_manager || sm->expected() != m_schedule) {
        beginResetModel();
        m_manager = sm;
        m_schedule = sm ? sm->expected() : nullptr;
        qDeleteAll(m_datamap);
        m_datamap.clear();
        endResetModel();
    }
}

long ResourceAppointmentsRowModel::id() const
{
    return m_manager ? m_manager->scheduleId() : -1;
}

const QMetaEnum ResourceAppointmentsRowModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

int ResourceAppointmentsRowModel::columnCount(const QModelIndex & /*parent */) const
{
    return columnMap().keyCount();
}

int ResourceAppointmentsRowModel::rowCount(const QModelIndex & parent) const
{
    if (m_project == nullptr) {
        return 0;
    }
    if (! parent.isValid()) {
        return m_project->resourceCount();
    }
    if (m_manager == nullptr) {
        return 0;
    }
    if (Resource *r = resource(parent)) {
        return r->numAppointments(id()) + r->numExternalAppointments(); // number of tasks there are appointments with + external projects
    }
    if (Appointment *a = appointment(parent)) {
        return a->count(); // number of appointment intervals
    }
    return 0;
}

QVariant ResourceAppointmentsRowModel::data(const QModelIndex &index, int role) const
{
    //debugPlan<<index<<role;
    if (! index.isValid()) {
        return QVariant();
    }
    if (role == Qt::TextAlignmentRole) {
        return headerData(index.column(), Qt::Horizontal, role);
    }
    return static_cast<Private*>(index.internalPointer())->data(index.column(), id(), role);
}

QVariant ResourceAppointmentsRowModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        return QVariant();
    }
    if (role == Qt::DisplayRole) {
        switch (section) {
            case Name: return i18n("Name");
            case Type: return i18n("Type");
            case StartTime: return i18n("Start Time");
            case EndTime: return i18n("End Time");
            case Load: return xi18nc("@title:column noun", "Load");
        }
    }
    if (role == Qt::TextAlignmentRole) {
        switch (section) {
            case Name:
            case Type:
            case StartTime:
            case EndTime:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case Load:
                return (int)(Qt::AlignRight|Qt::AlignVCenter);
        }
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QModelIndex ResourceAppointmentsRowModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid() || m_project == nullptr) {
        warnPlan<<"No data "<<idx;
        return QModelIndex();
    }
    Resource *pr = resource(idx);
    if (pr) {
        return QModelIndex();
    }
    pr = parentResource(idx);
    if (pr) {
        // Appointment, parent is Resource
        return index(pr);
    }
    if (Appointment *a = parentAppointment(idx)) {
        // AppointmentInterval, parent is Appointment
        QModelIndex p;
        Private *pi = static_cast<Private*>(idx.internalPointer());
        if (pi->parent->type == OT_Appointment) {
            Q_ASSERT(a->resource()->id() == id());
            if (a->resource() && a->resource()->resource()) {
                Resource *r = a->resource()->resource();
                int row = r->indexOf(a, id());
                Q_ASSERT(row >= 0);
                p = const_cast<ResourceAppointmentsRowModel*>(this)->createAppointmentIndex(row, 0, r);
                //debugPlan<<"Parent:"<<p<<r->name();
                Q_ASSERT(p.isValid());
            }
        } else if (pi->parent->type == OT_External) {
            Resource *r = static_cast<Resource*>(pi->parent->parent->ptr);
            int row = r->externalAppointmentList().indexOf(a);
            Q_ASSERT(row >= 0);
            row += r->numAppointments(id());
            p = const_cast<ResourceAppointmentsRowModel*>(this)->createAppointmentIndex(row, 0, r);
        }
        return p;
    }
    return QModelIndex();
}

QModelIndex ResourceAppointmentsRowModel::index(Resource *r) const
{
    Q_ASSERT(r);
    if (m_project == nullptr || r == nullptr) {
        return QModelIndex();
    }
    QModelIndex idx = const_cast<ResourceAppointmentsRowModel*>(this)->createResourceIndex(m_project->indexOf(r), 0);
    Q_ASSERT(idx.isValid());
    return idx;
}

QModelIndex ResourceAppointmentsRowModel::index(Appointment *a) const
{
    if (m_project == nullptr || m_manager == nullptr || a == nullptr || a->resource()->resource() == nullptr) {
        return QModelIndex();
    }
    Resource *r = a->resource()->resource();
    return const_cast<ResourceAppointmentsRowModel*>(this)->createAppointmentIndex(r->indexOf(a, id()), 0, r);
}

QModelIndex ResourceAppointmentsRowModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || row < 0 || column < 0) {
        return QModelIndex();
    }
    if (!parent.isValid()) {
        if (row < m_project->resourceCount()) {
            return const_cast<ResourceAppointmentsRowModel*>(this)->createResourceIndex(row, column);
        }
        return QModelIndex();
    }
    if (m_manager == nullptr) {
        return QModelIndex();
    }
    if (Resource *r = resource(parent)) {
        int num = r->numAppointments(id()) + r->numExternalAppointments();
        if (row < num) {
            //debugPlan<<"Appointment: "<<r->appointmentAt(row, id)<<static_cast<Private*>(parent.internalPointer());
            return const_cast<ResourceAppointmentsRowModel*>(this)->createAppointmentIndex(row, column, r);
        }
        return QModelIndex();
    }
    if (Appointment *a = appointment(parent)) {
        int num = a->count();
        if (row < num) {
            //debugPlan<<"Appointment interval at: "<<row<<static_cast<Private*>(parent.internalPointer());
            return const_cast<ResourceAppointmentsRowModel*>(this)->createIntervalIndex(row, column, a);
        }
        return QModelIndex();
    }
    return QModelIndex();
}

QModelIndex ResourceAppointmentsRowModel::createResourceIndex(int row, int column)
{
    Resource *res = m_project->resourceAt(row);
    Private *p = m_datamap.value((void*)res);
    if (p == nullptr) {
        p = new Private(nullptr, res, OT_Resource);
        m_datamap.insert(res, p);
    }
    QModelIndex idx = createIndex(row, column, p);
    Q_ASSERT(idx.isValid());
    return idx;
}

QModelIndex ResourceAppointmentsRowModel::createAppointmentIndex(int row, int column, Resource *r)
{
    Private *p = nullptr;
    KPlato::ObjectType type;
    Appointment *a = nullptr;
    if (row < r->numAppointments(id())) {
        a = r->appointmentAt(row, id());
        type = OT_Appointment;
    } else {
        a = r->externalAppointmentList().value(row - r->numAppointments(id()));
        type = OT_External;
    }
    Q_ASSERT(a);
    p = m_datamap.value((void*)a);
    if (p == nullptr) {
        Private *pr = m_datamap.value(r);
        Q_ASSERT(pr);
        p = new Private(pr, a, type);
        m_datamap.insert(a, p);
    }
    QModelIndex idx = createIndex(row, column, p);
    Q_ASSERT(idx.isValid());
    return idx;
}

QModelIndex ResourceAppointmentsRowModel::createIntervalIndex(int row, int column, Appointment *a)
{
    AppointmentInterval i = a->intervalAt(row);
    Private *pr = m_datamap.value(a);
    Q_ASSERT(pr);
    Private *p = pr->intervalAt(row);
    Q_ASSERT(p);

    QModelIndex idx = createIndex(row, column, p);
    Q_ASSERT(idx.isValid());
    return idx;
}

void ResourceAppointmentsRowModel::slotResourceToBeInserted(Project *project, int row)
{
    Q_UNUSED(project)
    beginInsertRows(QModelIndex(), row, row);
}

void ResourceAppointmentsRowModel::slotResourceInserted(Resource *resource)
{
    connectSignals(resource, true);
    endInsertRows();
}

void ResourceAppointmentsRowModel::slotResourceToBeRemoved(Project *project, int row, Resource *resource)
{
    Q_UNUSED(project)
    beginRemoveRows(QModelIndex(), row, row);
    connectSignals(resource, false);

    Private *p = nullptr;
    const QList<Appointment*> appointments = resource->appointments(id());
    for (Appointment *a : appointments) {
        // remove appointment
        p = m_datamap.value(a);
        if (p) {
            m_datamap.remove(a);
            delete p;
        }
    }
    const QList<Appointment*> appointments2 = resource->externalAppointmentList();
    for (Appointment *a : appointments2) {
        // remove appointment
        p = m_datamap.value(a);
        if (p) {
            m_datamap.remove(a);
            delete p;
        }
    }
    // remove resource
    p = m_datamap.value((void*)resource);
    if (p) {
        m_datamap.remove(const_cast<Resource*>(resource));
        delete p;
    }
}

void ResourceAppointmentsRowModel::slotResourceRemoved()
{
    endRemoveRows();
}

void ResourceAppointmentsRowModel::slotAppointmentToBeInserted(Resource *r, int row)
{
    Q_UNUSED(r);
    Q_UNUSED(row);
    // external appointments only, (Internal handled in slotProjectCalculated)
    beginResetModel();
}

void ResourceAppointmentsRowModel::slotAppointmentInserted(Resource *r, Appointment *a)
{
    Q_UNUSED(a);
    // external appointments only, (Internal handled in slotProjectCalculated)
    Private *p = m_datamap.value(r);
    if (p) {
        p->externalCached = false;
    }
    endResetModel();
}

void ResourceAppointmentsRowModel::slotAppointmentToBeRemoved(Resource *r, int row)
{
    Q_UNUSED(row);
    beginResetModel();
    // external appointments only, (Internal handled in slotProjectCalculated)
    Private *p = m_datamap.value(r);
    if (p) {
        p->externalCached = false;
    }
}

void ResourceAppointmentsRowModel::slotAppointmentRemoved()
{
    // external appointments only, (Internal handled in slotProjectCalculated)
    endResetModel();
    
}

void ResourceAppointmentsRowModel::slotAppointmentChanged(Resource *r, Appointment *a)
{
    Q_UNUSED(r);
    Q_UNUSED(a);
    // external appointments only, (Internal handled in slotProjectCalculated)
    // will not happen atm
}

void ResourceAppointmentsRowModel::slotProjectCalculated(ScheduleManager *sm)
{
    if (sm == m_manager) {
        setScheduleManager(sm);
    }
}

Resource *ResourceAppointmentsRowModel::parentResource(const QModelIndex &index) const
{
    if (m_project == nullptr) {
        return nullptr;
    }
    Private *ch = static_cast<Private*>(index.internalPointer());
    if (ch && (ch->type == OT_Appointment || ch->type == OT_External)) {
        return static_cast<Resource*>(ch->parent->ptr);
    }
    return nullptr;
}

Resource *ResourceAppointmentsRowModel::resource(const QModelIndex &index) const
{
    if (m_project == nullptr) {
        return nullptr;
    }
    Private *p = static_cast<Private*>(index.internalPointer());
    if (p && p->type == OT_Resource) {
        return static_cast<Resource*>(p->ptr);
    }
    return nullptr;
}

Appointment *ResourceAppointmentsRowModel::parentAppointment(const QModelIndex &index) const
{
    if (m_project == nullptr || m_manager == nullptr) {
        return nullptr;
    }
    Private *ch = static_cast<Private*>(index.internalPointer());
    if (ch && ch->type == OT_Interval) {
        return static_cast<Appointment*>(ch->parent->ptr);
    }
    return nullptr;
}

Appointment *ResourceAppointmentsRowModel::appointment(const QModelIndex &index) const
{
    if (m_project == nullptr || m_manager == nullptr || ! index.isValid()) {
        return nullptr;
    }
    Private *p = static_cast<Private*>(index.internalPointer());
    if (p && (p->type == OT_Appointment || p->type == OT_External)) {
        return static_cast<Appointment*>(p->ptr);
    }
    return nullptr;
}

AppointmentInterval *ResourceAppointmentsRowModel::interval(const QModelIndex &index) const
{
    if (m_project == nullptr || m_manager == nullptr) {
        return nullptr;
    }
    Private *p = static_cast<Private*>(index.internalPointer());
    if (p && p->type == OT_Interval) {
        return &(p->interval);
    }
    return nullptr;
}

Node *ResourceAppointmentsRowModel::node(const QModelIndex &idx) const
{
    Appointment *a = appointment(idx);
    return (a && a->node() ? a->node()->node() : nullptr);
}


//---------------------------------------------
ResourceAppointmentsGanttModel::ResourceAppointmentsGanttModel(QObject *parent)
    : ResourceAppointmentsRowModel(parent)
{
}

ResourceAppointmentsGanttModel::~ResourceAppointmentsGanttModel()
{
}

QVariant ResourceAppointmentsGanttModel::data(const Resource *r, int column, int role) const
{
    Q_UNUSED(column);
    switch(role) {
        case KGantt::ItemTypeRole: return KGantt::TypeSummary;
        case KGantt::StartTimeRole: return r->startTime(id());
        case KGantt::EndTimeRole: return r->endTime(id());
    }
    return QVariant();
}

QVariant ResourceAppointmentsGanttModel::data(const Appointment *a, int column, int role) const
{
    Q_UNUSED(column);
    switch(role) {
        case KGantt::ItemTypeRole: return KGantt::TypeMulti;
        case KGantt::StartTimeRole: return a->startTime();
        case KGantt::EndTimeRole: return a->endTime();
    }
    return QVariant();
}

QVariant ResourceAppointmentsGanttModel::data(const AppointmentInterval *a, int column, int role) const
{
    Q_UNUSED(column);
    switch(role) {
        case KGantt::ItemTypeRole: return KGantt::TypeTask;
        case KGantt::StartTimeRole: return a->startTime();
        case KGantt::EndTimeRole: return a->endTime();
    }
    return QVariant();
}

QVariant ResourceAppointmentsGanttModel::data(const QModelIndex &index, int role) const
{
    //debugPlan<<index<<role;
    if (m_project == nullptr || ! index.isValid()) {
        return QVariant();
    }
    if (role == KGantt::ItemTypeRole ||
         role == KGantt::StartTimeRole ||
         role == KGantt::EndTimeRole ||
         role == KGantt::TaskCompletionRole)
    {
        if (Resource *r = resource(index)) {
            return data(r, index.column(), role);
        }
        if (m_manager == nullptr) {
            return QVariant();
        }
        if (Appointment *a = appointment(index)) {
            return data(a, index.column(), role);
        }
        if (AppointmentInterval *i = interval(index)) {
            return data(i, index.column(), role);
        }
        return QVariant();
    }
    return ResourceAppointmentsRowModel::data(index, role);
}

} // namespace KPlato
