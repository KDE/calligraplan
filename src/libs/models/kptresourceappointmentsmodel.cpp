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

ResourceAppointmentsItemModel::ResourceAppointmentsItemModel(QObject *parent)
    : ItemModelBase(parent),
    m_group(nullptr),
    m_resource(nullptr),
    m_showInternal(true),
    m_showExternal(true)
{
}

ResourceAppointmentsItemModel::~ResourceAppointmentsItemModel()
{
}

void ResourceAppointmentsItemModel::slotResourceToBeInserted(Project *project, int row)
{
    Q_UNUSED(project)
    beginInsertRows(QModelIndex(), row, row);
}

void ResourceAppointmentsItemModel::slotResourceInserted(Resource *resource)
{
    connectSignals(resource, true);
    endInsertRows();
    beginResetModel();
    refresh();
    endResetModel();
}

void ResourceAppointmentsItemModel::slotResourceToBeRemoved(Project *project, int row, Resource *resource)
{
    Q_UNUSED(project)
    connectSignals(resource, false);
    beginRemoveRows(QModelIndex(), row, row);
}

void ResourceAppointmentsItemModel::slotResourceRemoved()
{
    endRemoveRows();
    beginResetModel();
    refresh();
    endResetModel();
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
    beginResetModel();
    refreshData();
    endResetModel();
}

void ResourceAppointmentsItemModel::slotAppointmentToBeRemoved(Resource *r, int row)
{
    Q_UNUSED(r);
    Q_UNUSED(row);
}

void ResourceAppointmentsItemModel::slotAppointmentRemoved()
{
    beginResetModel();
    refreshData();
    endResetModel();
}

void ResourceAppointmentsItemModel::slotAppointmentChanged(Resource *r, Appointment *a)
{
    int row = rowNumber(r, a);
    Q_ASSERT(row >= 0);
    refreshData();
    Q_EMIT dataChanged(createExternalAppointmentIndex(row, 0, a), createExternalAppointmentIndex(row, columnCount() - 1, a));
}

void ResourceAppointmentsItemModel::slotProjectCalculated(ScheduleManager *sm)
{
    if (sm == m_manager) {
        beginResetModel();
        refreshData();
        endResetModel();
        Q_EMIT refreshed();
    }
}

int ResourceAppointmentsItemModel::rowNumber(Resource *res, Appointment *a) const
{
    int r = 0;
    if (m_showInternal) {
        r = res->appointments(id()).indexOf(a);
        if (r > -1) {
            return r;
        }
        r = res->numAppointments();
    }
    if (m_showExternal) {
        int rr = res->externalAppointmentList().indexOf(a);
        if (rr > -1) {
            return r + rr;
        }
    }
    return -1;
}

void ResourceAppointmentsItemModel::setShowInternalAppointments(bool show)
{
    if (m_showInternal == show) {
        return;
    }
    beginResetModel();
    m_showInternal = show;
    refreshData();
    endResetModel();
}

void ResourceAppointmentsItemModel::setShowExternalAppointments(bool show)
{
    if (m_showExternal == show) {
        return;
    }
    beginResetModel();
    m_showExternal = show;
    refreshData();
    endResetModel();
}

void ResourceAppointmentsItemModel::setProject(Project *project)
{
    Q_UNUSED(project)
    beginResetModel();
    debugPlan;
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ResourceAppointmentsItemModel::projectDeleted);
        disconnect(m_project, &Project::defaultCalendarChanged, this, &ResourceAppointmentsItemModel::slotCalendarChanged);
        disconnect(m_project, &Project::projectCalculated, this, &ResourceAppointmentsItemModel::slotProjectCalculated);
        disconnect(m_project, &Project::scheduleManagerChanged, this, &ResourceAppointmentsItemModel::slotProjectCalculated);

        disconnect(m_project, &Project::resourceChanged, this, &ResourceAppointmentsItemModel::slotResourceChanged);
        disconnect(m_project, &Project::resourceToBeAdded, this, &ResourceAppointmentsItemModel::slotResourceToBeInserted);
        disconnect(m_project, &Project::resourceToBeRemoved, this, &ResourceAppointmentsItemModel::slotResourceToBeRemoved);
        disconnect(m_project, &Project::resourceAdded, this, &ResourceAppointmentsItemModel::slotResourceInserted);
        disconnect(m_project, &Project::resourceRemoved, this, &ResourceAppointmentsItemModel::slotResourceRemoved);

        const QList<Resource*> resources = m_project->resourceList();
        for (Resource *r : resources) {
            connectSignals(r, false);
        }
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &ResourceAppointmentsItemModel::projectDeleted);
        connect(m_project, &Project::defaultCalendarChanged, this, &ResourceAppointmentsItemModel::slotCalendarChanged);
        connect(m_project, &Project::projectCalculated, this, &ResourceAppointmentsItemModel::slotProjectCalculated);
        connect(m_project, &Project::scheduleManagerChanged, this, &ResourceAppointmentsItemModel::slotProjectCalculated);

        connect(m_project, &Project::resourceChanged, this, &ResourceAppointmentsItemModel::slotResourceChanged);
        connect(m_project, &Project::resourceToBeAdded, this, &ResourceAppointmentsItemModel::slotResourceToBeInserted);
        connect(m_project, &Project::resourceToBeRemoved, this, &ResourceAppointmentsItemModel::slotResourceToBeRemoved);
        connect(m_project, &Project::resourceAdded, this, &ResourceAppointmentsItemModel::slotResourceInserted);
        connect(m_project, &Project::resourceRemoved, this, &ResourceAppointmentsItemModel::slotResourceRemoved);

        const QList<Resource*> resources = m_project->resourceList();
        for (Resource *r : resources) {
            connectSignals(r, true);
        }
    }
    refreshData();
    endResetModel();
    Q_EMIT refreshed();
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
    beginResetModel();
    debugPlan<<sm;
    m_manager = sm;
    refreshData();
    endResetModel();
    Q_EMIT refreshed();
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
        warnPlan<<"No data "<<idx;
        return QModelIndex();
    }
    if (idx.internalPointer() == nullptr) {
        return QModelIndex(); // resources is top level
    }
    QModelIndex p;
    if (! p.isValid() && m_showInternal) {
        Appointment *a = appointment(idx);
        if (a && a->resource() && a->resource()->resource()) {
            Resource *r = a->resource()->resource();
            int row = m_project->indexOf(r);
            p = createResourceIndex(row, 0, r);
            //debugPlan<<"Parent:"<<p<<r->name();
            Q_ASSERT(p.isValid());
        }
    }
    if (! p.isValid() && m_showExternal) {
        Appointment *a = externalAppointment(idx);
        Resource *r = parent(a);
        if (r) {
            int row = m_project->indexOf(r);
            p = createResourceIndex(row, 0, r);
        }
    }
    if (! p.isValid()) {
        //debugPlan<<"Parent:"<<p;
    }
    //debugPlan<<"Child :"<<idx;
    return p;
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
    if (!parent.isValid()) {
        if (row < m_project->resourceCount()) {
            return createResourceIndex(row, column, m_project->resourceAt(row));
        }
        return QModelIndex();
    }
    if (m_manager == nullptr) {
        return QModelIndex();
    }
    Resource *r = resource(parent);
    if (r && (m_showInternal || m_showExternal)) {
        int num = m_showInternal ? r->numAppointments(id()) : 0;
        if (row < num) {
            //debugPlan<<"Appointment: "<<r->appointmentAt(row, id);
            return createAppointmentIndex(row, column, r->appointmentAt(row, id()));
        }
        int extRow = row - num;
        //debugPlan<<"Appointment: "<<r->externalAppointmentList().value(extRow);
        Q_ASSERT(extRow >= 0 && extRow < r->externalAppointmentList().count());
        return createExternalAppointmentIndex(row, column, r->externalAppointmentList().value(extRow));
    }
    return QModelIndex();
}

QModelIndex ResourceAppointmentsItemModel::index(Resource *resource) const
{
    if (m_project == nullptr || resource == nullptr) {
        return QModelIndex();
    }
    int row = m_project->indexOf(resource);
    return row < 0 ? QModelIndex() : createIndex(row, 0, resource);
}

void ResourceAppointmentsItemModel::refresh()
{
    refreshData();
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
            //debugPlan<<r->name()<<a->auxcilliaryInfo()<<": "<<extEff[ a ].totalEffort().toDouble(Duration::Unit_h);
            //debugPlan<<r->name()<<a->auxcilliaryInfo()<<": "<<extEff[ a ].startDate()<<extEff[ a ].endDate();
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
    if (!parent.isValid()) {
        return m_project->resourceCount();
    }
    if (m_manager == nullptr) {
        return 0;
    }
    Resource *r = resource(parent);
    if (r) {
        int rows = m_showInternal ? r->numAppointments(id()) : 0;
        rows += m_showExternal ? r->numExternalAppointments() : 0;
        return rows;
    }
    return 0;
}

QVariant ResourceAppointmentsItemModel::name(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole: {
            QVariant value = res->name();
            return value;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceAppointmentsItemModel::name(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return node->name();
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceAppointmentsItemModel::name(const Appointment *app, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return app->auxcilliaryInfo();
        case Qt::ToolTipRole:
            return i18n("External project: %1", app->auxcilliaryInfo());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::ForegroundRole:
            if (m_externalEffortMap.contains(app)) {
                return QColor(Qt::blue);
            }
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
//             QString ds = QLocale().toString(d.toDouble(Duration::Unit_h), 'f', 1);
//             Duration avail = res->effort(nullptr, DateTime(date, QTime(0,0,0)), Duration(1.0, Duration::Unit_d));
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
    Resource *r = resource(index);
    if (r) {
        switch (index.column()) {
            case 0: result = name(r, role); break;
            case 1: result = total(r, role); break;
            default:
                QDate d = startDate().addDays(index.column() - 2);
                result = total(r, d, role);
                break;
        }
        return result;
    }
    if (m_manager == nullptr) {
        return QVariant();
    }
    Appointment *a = appointment(index);
    if (a) {
        switch (index.column()) {
            case 0: result = name(a->node()->node(), role); break;
            case 1: result = total(a, role); break;
            default: {
                QDate d = startDate().addDays(index.column()-2);
                result = assignment(a, d, role);
                break;
            }
        }
        return result;
    }
    a = externalAppointment(index);
    if (a) {
        //debugPlan<<"external"<<a->auxcilliaryInfo()<<index;
        switch (index.column()) {
            case 0: result = name(a, role); break;
            case 1: result = total(a, role); break;
            default: {
                QDate d = startDate().addDays(index.column()-2);
                result = assignment(a, d, role);
                break;
            }
        }
        return result;
    }
    debugPlan<<"Could not find ptr:"<<index;
    return QVariant();
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
    if (m_project == nullptr || m_manager == nullptr) {
        return nullptr;
    }
    const QList<Resource*> resources = m_project->resourceList();
    for (Resource *r : resources) {
        const QList<Appointment*> appointments = r->appointments(id());
        for (Appointment *a : appointments) {
            if (a == index.internalPointer()) {
                return a;
            }
        }
    }
    return nullptr;
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
    if (m_project == nullptr) {
        return nullptr;
    }
    const QList<Resource*> resources = m_project->resourceList();
    for (Resource *r : resources) {
        if (r == index.internalPointer()) {
            return r;
        }
    }
    return nullptr;
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
