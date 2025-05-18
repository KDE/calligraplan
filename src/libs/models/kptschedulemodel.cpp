/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptschedulemodel.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptschedule.h"
#include "kptdatetime.h"
#include "kptschedulerplugin.h"
#include "kptdebug.h"

#include <KoIcon.h>

#include <QObject>
#include <QStringList>
#include <QLocale>

#include <KFormat>


namespace KPlato
{

//--------------------------------------

// internal, for displaying schedule name + state
#define SpecialScheduleDisplayRole 99999999

ScheduleModel::ScheduleModel(QObject *parent)
    : QObject(parent)
{
}

ScheduleModel::~ScheduleModel()
{
}

const QMetaEnum ScheduleModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

int ScheduleModel::propertyCount() const
{
    return columnMap().keyCount();
}

//--------------------------------------

ScheduleItemModel::ScheduleItemModel(QObject *parent)
    : ItemModelBase(parent),
    m_manager(nullptr),
    m_flat(false)
{
}

ScheduleItemModel::~ScheduleItemModel()
{
}

void ScheduleItemModel::slotScheduleManagerToBeInserted(const ScheduleManager *parent, int row)
{
    //debugPlan<<parent<<row;
    if (m_flat) {
        return; // handle in *Inserted();
    }
    Q_ASSERT(m_manager == nullptr);
    m_manager = const_cast<ScheduleManager*>(parent);
    beginInsertRows(index(parent), row, row);
}

void ScheduleItemModel::slotScheduleManagerInserted(const ScheduleManager *manager)
{
    //debugPlan<<manager->name();
    if (m_flat) {
        int row = m_project->allScheduleManagers().indexOf(const_cast<ScheduleManager*>(manager));
        Q_ASSERT(row >= 0);
        beginInsertRows(QModelIndex(), row, row);
        m_managerlist.insert(row, const_cast<ScheduleManager*>(manager));
        endInsertRows();
        Q_EMIT scheduleManagerAdded(const_cast<ScheduleManager*>(manager));
        return;
    }
    Q_ASSERT(manager->parentManager() == m_manager);
    endInsertRows();
    m_manager = nullptr;
    Q_EMIT scheduleManagerAdded(const_cast<ScheduleManager*>(manager));
}

void ScheduleItemModel::slotScheduleManagerToBeRemoved(const ScheduleManager *manager)
{
    //debugPlan<<manager->name();
    if (m_flat) {
        int row = m_managerlist.indexOf(const_cast<ScheduleManager*>(manager));
        beginRemoveRows(QModelIndex(), row, row);
        m_managerlist.removeAt(row);
        return;
    }

    Q_ASSERT(m_manager == nullptr);
    m_manager = const_cast<ScheduleManager*>(manager);
    QModelIndex i = index(manager);
    int row = i.row();
    beginRemoveRows(parent(i), row, row);
}

void ScheduleItemModel::slotScheduleManagerRemoved(const ScheduleManager *manager)
{
    //debugPlan<<manager->name();
    if (m_flat) {
        endRemoveRows();
        return;
    }
    Q_ASSERT(manager == m_manager); Q_UNUSED(manager);
    endRemoveRows();
    m_manager = nullptr;
}

void ScheduleItemModel::slotScheduleManagerToBeMoved(const ScheduleManager *manager)
{
    //debugPlan<<this<<manager->name()<<"from"<<(manager->parentManager()?manager->parentManager()->name():"project");
    slotScheduleManagerToBeRemoved(manager);
}

void ScheduleItemModel::slotScheduleManagerMoved(const ScheduleManager *manager, int index)
{
    //debugPlan<<this<<manager->name()<<"to"<<manager->parentManager()<<index;
    slotScheduleManagerRemoved(manager);
    slotScheduleManagerToBeInserted(manager->parentManager(), index);
    slotScheduleManagerInserted(manager);
}

void ScheduleItemModel::slotScheduleToBeInserted(const ScheduleManager *, int /*row*/)
{
}

void ScheduleItemModel::slotScheduleInserted(const MainSchedule *)
{
}

void ScheduleItemModel::slotScheduleToBeRemoved(const MainSchedule *)
{
}

void ScheduleItemModel::slotScheduleRemoved(const MainSchedule *)
{
}

void ScheduleItemModel::setProject(Project *project)
{
    beginResetModel();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ScheduleItemModel::projectDeleted);

        disconnect(m_project, &Project::scheduleManagerChanged, this, &ScheduleItemModel::slotManagerChanged);

        disconnect(m_project, &Project::scheduleManagerToBeAdded, this, &ScheduleItemModel::slotScheduleManagerToBeInserted);

        disconnect(m_project, &Project::scheduleManagerToBeRemoved, this, &ScheduleItemModel::slotScheduleManagerToBeRemoved);

        disconnect(m_project, &Project::scheduleManagerAdded, this, &ScheduleItemModel::slotScheduleManagerInserted);

        disconnect(m_project, &Project::scheduleManagerRemoved, this, &ScheduleItemModel::slotScheduleManagerRemoved);

        disconnect(m_project, &Project::scheduleManagerToBeMoved, this, &ScheduleItemModel::slotScheduleManagerToBeMoved);

        disconnect(m_project, &Project::scheduleManagerMoved, this, &ScheduleItemModel::slotScheduleManagerMoved);

        disconnect(m_project, &Project::scheduleChanged, this, &ScheduleItemModel::slotScheduleChanged);

        disconnect(m_project, &Project::scheduleToBeAdded, this, &ScheduleItemModel::slotScheduleToBeInserted);

        disconnect(m_project, &Project::scheduleToBeRemoved, this, &ScheduleItemModel::slotScheduleToBeRemoved);

        disconnect(m_project, &Project::scheduleAdded, this, &ScheduleItemModel::slotScheduleInserted);

        disconnect(m_project, &Project::scheduleRemoved, this, &ScheduleItemModel::slotScheduleRemoved);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &ScheduleItemModel::projectDeleted);

        connect(m_project, &Project::scheduleManagerChanged, this, &ScheduleItemModel::slotManagerChanged);

        connect(m_project, &Project::scheduleManagerToBeAdded, this, &ScheduleItemModel::slotScheduleManagerToBeInserted);

        connect(m_project, &Project::scheduleManagerToBeRemoved, this, &ScheduleItemModel::slotScheduleManagerToBeRemoved);

        connect(m_project, &Project::scheduleManagerAdded, this, &ScheduleItemModel::slotScheduleManagerInserted);

        connect(m_project, &Project::scheduleManagerRemoved, this, &ScheduleItemModel::slotScheduleManagerRemoved);

        connect(m_project, &Project::scheduleManagerToBeMoved, this, &ScheduleItemModel::slotScheduleManagerToBeMoved);

        connect(m_project, &Project::scheduleManagerMoved, this, &ScheduleItemModel::slotScheduleManagerMoved);

        connect(m_project, &Project::scheduleChanged, this, &ScheduleItemModel::slotScheduleChanged);

        connect(m_project, &Project::scheduleToBeAdded, this, &ScheduleItemModel::slotScheduleToBeInserted);

        connect(m_project, &Project::scheduleToBeRemoved, this, &ScheduleItemModel::slotScheduleToBeRemoved);

        connect(m_project, &Project::scheduleAdded, this, &ScheduleItemModel::slotScheduleInserted);

        connect(m_project, &Project::scheduleRemoved, this, &ScheduleItemModel::slotScheduleRemoved);
    }
    setFlat(m_flat); // update m_managerlist
    endResetModel();
}

void ScheduleItemModel::slotManagerChanged(ScheduleManager *sch)
{
    if (m_flat) {
        int row = m_managerlist.indexOf(sch);
        Q_EMIT dataChanged(createIndex(row, 0, sch), createIndex(row, columnCount() - 1, sch));
        return;
    }

    int r = sch->parentManager() ? sch->parentManager()->indexOf(sch) : m_project->indexOf(sch);
    //debugPlan<<sch<<":"<<r;
    Q_EMIT dataChanged(createIndex(r, 0, sch), createIndex(r, columnCount() - 1, sch));
}


void ScheduleItemModel::slotScheduleChanged(MainSchedule *)
{
}


Qt::ItemFlags ScheduleItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (!index.isValid())
        return flags;
    if (!m_readWrite) {
        return flags &= ~Qt::ItemIsEditable;
    }
    ScheduleManager *sm = manager(index);
    if (sm == nullptr) {
        return flags;
    }
    SchedulerPlugin *pl = sm->schedulerPlugin();
    if (pl == nullptr) {
        return flags;
    }
    int capabilities = pl->capabilities();
    flags &= ~Qt::ItemIsEditable;
    if (sm && ! sm->isBaselined()) {
        switch (index.column()) {
            case ScheduleModel::ScheduleName:
                if (!hasChildren(index) && !index.parent().isValid()) {
                    flags |= Qt::ItemIsUserCheckable;
                }
                flags |= Qt::ItemIsEditable;
                break;
            case ScheduleModel::ScheduleState:
                break;
            case ScheduleModel::ScheduleMode:
                if (!sm->isBaselined() && !hasChildren(index) && !index.parent().isValid()) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case ScheduleModel::ScheduleOverbooking:
                if (capabilities & SchedulerPlugin::AllowOverbooking &&
                     capabilities & SchedulerPlugin::AvoidOverbooking)
                {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case ScheduleModel::ScheduleDirection:
                if (sm->parentManager() == nullptr &&
                    capabilities & SchedulerPlugin::ScheduleForward &&
                    capabilities & SchedulerPlugin::ScheduleBackward)
                {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case ScheduleModel::SchedulePlannedStart: break;
            case ScheduleModel::SchedulePlannedFinish: break;
            case ScheduleModel::ScheduleGranularity:
                if (! sm->supportedGranularities().isEmpty()) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            default: flags |= Qt::ItemIsEditable; break;
        }
    }
    return flags;
}


QModelIndex ScheduleItemModel::parent(const QModelIndex &inx) const
{
    if (!inx.isValid() || m_project == nullptr || m_flat) {
        return QModelIndex();
    }
    //debugPlan<<inx.internalPointer()<<":"<<inx.row()<<","<<inx.column();
    ScheduleManager *sm = manager(inx);
    if (sm == nullptr) {
        return QModelIndex();
    }
    return index(sm->parentManager());
}

QModelIndex ScheduleItemModel::index(int row, int column, const QModelIndex &parent) const
{
    //debugPlan<<m_project<<":"<<row<<","<<column;
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0 || row >= rowCount(parent)) {
        //debugPlan<<row<<","<<column<<" out of bounce";
        return QModelIndex();
    }
    if (m_flat) {
        return createIndex(row, column, m_managerlist[ row ]);
    }

    if (parent.isValid()) {
        return createIndex(row, column, manager(parent)->children().value(row));
    }
    return createIndex(row, column, m_project->scheduleManagers().value(row));
}

QModelIndex ScheduleItemModel::index(const ScheduleManager *manager) const
{
    if (m_project == nullptr || manager == nullptr) {
        return QModelIndex();
    }
    if (m_flat) {
        return createIndex(m_managerlist.indexOf(const_cast<ScheduleManager*>(manager)), 0, const_cast<ScheduleManager*>(manager));
    }

    if (manager->parentManager() == nullptr) {
        return createIndex(m_project->indexOf(manager), 0, const_cast<ScheduleManager*>(manager));
    }
    return createIndex(manager->parentManager()->indexOf(manager), 0, const_cast<ScheduleManager*>(manager));
}

int ScheduleItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return m_model.propertyCount();
}

int ScheduleItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == nullptr) {
        return 0;
    }
    if (m_flat) {
        return m_managerlist.count();
    }
    if (!parent.isValid()) {
        return m_project->numScheduleManagers();
    }
    ScheduleManager *sm = manager(parent);
    if (sm) {
        //debugPlan<<sm->name()<<","<<sm->children().count();
        return sm->children().count();
    }
    return 0;
}

QVariant ScheduleItemModel::name(const QModelIndex &index, int role) const
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return sm->name();
        case Qt::StatusTipRole:
        case Qt::ToolTipRole:
        case Qt::WhatsThisRole:
            break;
        case Qt::DecorationRole:
            if (sm->isBaselined()) {
                return koIcon("view-time-schedule-baselined");
            }
            return QVariant();
        default:
            break;
    }
    return QVariant();
}

bool ScheduleItemModel::setName(const QModelIndex &index, const QVariant &value, int role)
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return false;
    }
    switch (role) {
        case Qt::EditRole:
            Q_EMIT executeCommand(new ModifyScheduleManagerNameCmd(*sm, value.toString(), kundo2_i18n("Modify schedule name")));
            return true;
        default:
            break;
    }
    return false;
}

QVariant ScheduleItemModel::schedulingMode(const QModelIndex &index, int role) const
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            return schedulingMode(index, Role::EnumList).toList().value(sm->schedulingMode());
        case Qt::EditRole:
            return sm->schedulingMode();
        case Qt::StatusTipRole:
            break;
        case Qt::ToolTipRole:
            if (sm->schedulingMode() == ScheduleManager::ManualMode) {
                return xi18nc("@info:tooltip", "The schedule is in Manual Mode, calculation must be initiated manually");
            }
            return xi18nc("@info:tooltip", "The schedule is in Auto Mode, it will be calculated automatically");
        case Qt::WhatsThisRole:
            break;
        case Role::EnumList:
            return QStringList() << xi18nc("@label:listbox", "Manual") << xi18nc("@label:listbox", "Auto");
        case Role::EnumListValue:
            return sm->schedulingMode();
        default:
            break;
    }
    return QVariant();
}

bool ScheduleItemModel::setSchedulingMode(const QModelIndex &index, const QVariant &value, int role)
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return false;
    }
    switch (role) {
        case Qt::EditRole:
            Q_EMIT executeCommand(new ModifyScheduleManagerSchedulingModeCmd(*sm, value.toInt(), kundo2_i18n("Modify scheduling mode")));
            return true;
        default:
            break;
    }
    return false;
}

QVariant ScheduleItemModel::state(const QModelIndex &index, int role) const
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
        {
            if (sm->progress() > 0) {
                return sm->progress();
            }
            QStringList l = sm->state();
            if (l.isEmpty()) {
                return QLatin1String("");
            }
            return l.first();
        }
        case Qt::EditRole:
        {
            QStringList l = sm->state();
            if (l.isEmpty()) {
                return QLatin1String("");
            }
            return l.first();
        }
        case Qt::ToolTipRole: {
            if (sm->owner() == ScheduleManager::OwnerPortfolio) {
                return i18nc("@info:tooltip", "This schedule is created by the Portfolio Manager");
            }
            const auto states = sm->state();
            if (states.count() > 1) {
                return states.join(QStringLiteral(", "));
            }
            break;
        }
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Maximum:
            return sm->maxProgress();
        case Role::Minimum:
            return 0;
        case SpecialScheduleDisplayRole: {
            QString st;
            if (!sm->isScheduled()) {
                st = sm->state().value(0);
                if (sm->progress() > 0) {
                    st = QStringLiteral("%1 %2%").arg(st).arg(sm->progress());
                }
            }
            return st;
        }
    }
    return QVariant();
}

bool ScheduleItemModel::setState(const QModelIndex &, const QVariant &, int role)
{
    switch (role) {
        case Qt::EditRole:
            return false;
    }
    return false;
}

QVariant ScheduleItemModel::allowOverbooking(const QModelIndex &index, int role) const
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return QVariant();
    }
    SchedulerPlugin *pl = sm->schedulerPlugin();
    if (pl == nullptr) {
        return QVariant();
    }
    int capabilities = pl->capabilities();
    switch (role) {
        case Qt::EditRole:
            return sm->allowOverbooking();
        case Qt::DisplayRole:
            if (capabilities & SchedulerPlugin::AllowOverbooking &&
                 capabilities & SchedulerPlugin::AvoidOverbooking)
            {
                return sm->allowOverbooking() ? i18n("Allow") : i18n("Avoid");
            }
            if (capabilities & SchedulerPlugin::AllowOverbooking) {
                return sm->allowOverbooking() ? i18n("Allow") : i18n("(Avoid)");
            }
            if (capabilities & SchedulerPlugin::AvoidOverbooking) {
                return sm->allowOverbooking() ? i18n("(Allow)") : i18n("Avoid");
            }
            break;
        case Qt::ToolTipRole:
            if (capabilities & SchedulerPlugin::AllowOverbooking &&
                 capabilities & SchedulerPlugin::AvoidOverbooking)
            {
                return sm->allowOverbooking()
                            ? xi18nc("@info:tooltip", "Allow overbooking resources")
                            : xi18nc("@info:tooltip", "Avoid overbooking resources");
            }
            if (capabilities & SchedulerPlugin::AllowOverbooking) {
                return sm->allowOverbooking()
                            ? xi18nc("@info:tooltip", "Allow overbooking of resources")
                            : xi18nc("@info:tooltip 1=scheduler name", "%1 always allows overbooking of resources", pl->name());
            }
            if (capabilities & SchedulerPlugin::AvoidOverbooking) {
                return sm->allowOverbooking()
                            ? xi18nc("@info:tooltip 1=scheduler name", "%1 always avoids overbooking of resources", pl->name())
                            : xi18nc("@info:tooltip", "Avoid overbooking resources");
            }
            break;
        case Role::EnumList:
            return QStringList() << xi18nc("@label:listbox", "Avoid") << xi18nc("@label:listbox", "Allow");
        case Role::EnumListValue:
            return sm->allowOverbooking() ? 1 : 0;
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ScheduleItemModel::setAllowOverbooking(const QModelIndex &index, const QVariant &value, int role)
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return false;
    }
    switch (role) {
        case Qt::EditRole:
            Q_EMIT executeCommand(new ModifyScheduleManagerAllowOverbookingCmd(*sm, value.toBool(), kundo2_i18n("Modify allow overbooking")));
            return true;
    }
    return false;
}


QVariant ScheduleItemModel::usePert(const QModelIndex &index, int role) const
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::EditRole:
            return sm->usePert();
        case Qt::DisplayRole:
            return sm->usePert() ? i18n("PERT") : i18n("None");
        case Qt::ToolTipRole:
            return sm->usePert()
                        ? xi18nc("@info:tooltip", "Use PERT distribution to calculate expected estimate for the tasks")
                        : xi18nc("@info:tooltip", "Use the tasks expected estimate directly");
        case Role::EnumList:
            return QStringList() << xi18nc("@label:listbox", "None") << xi18nc("@label:listbox", "PERT");
        case Role::EnumListValue:
            return sm->usePert() ? 1 : 0;
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ScheduleItemModel::setUsePert(const QModelIndex &index, const QVariant &value, int role)
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return false;
    }
    switch (role) {
        case Qt::EditRole:
            Q_EMIT executeCommand(new ModifyScheduleManagerDistributionCmd(*sm, value.toBool(), kundo2_i18n("Modify scheduling distribution")));
            slotManagerChanged(static_cast<ScheduleManager*>(sm));
            return true;
    }
    return false;
}

QVariant ScheduleItemModel::projectStart(const QModelIndex &index, int role) const
{
    if (m_project == nullptr) {
        return QVariant();
    }
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            if (sm->isScheduled()) {
                return QLocale().toString(sm->expected()->start(), QLocale::ShortFormat);
            }
            break;
        case Qt::EditRole:
            if (sm->isScheduled()) {
                return sm->expected()->start();
            }
            break;
        case Qt::ToolTipRole:
            if (sm->isScheduled()) {
                return xi18nc("@info:tooltip", "Planned start: %1<nl/>Target start: %2", QLocale().toString(sm->expected()->start(), QLocale::ShortFormat), QLocale().toString(m_project->constraintStartTime(), QLocale::ShortFormat));
            } else {
                return xi18nc("@info:tooltip", "Target start: %1", QLocale().toString(m_project->constraintStartTime(), QLocale::ShortFormat));
            }
            break;
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ScheduleItemModel::projectEnd(const QModelIndex &index, int role) const
{
    if (m_project == nullptr) {
        return QVariant();
    }
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            if (sm->isScheduled()) {
                return QLocale().toString(sm->expected()->end(), QLocale::ShortFormat);
            }
            break;
        case Qt::EditRole:
            if (sm->isScheduled()) {
                return sm->expected()->end();
            }
            break;
        case Qt::ToolTipRole:
            if (sm->isScheduled()) {
                return xi18nc("@info:tooltip", "Planned finish: %1<nl/>Target finish: %2", QLocale().toString(sm->expected()->end(), QLocale::ShortFormat), QLocale().toString(m_project->constraintEndTime(), QLocale::ShortFormat));
            } else {
                return xi18nc("@info:tooltip", "Target finish: %1", QLocale().toString(m_project->constraintEndTime(), QLocale::ShortFormat));
            }
            break;
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ScheduleItemModel::schedulingDirection(const QModelIndex &index, int role) const
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return QVariant();
    }
    SchedulerPlugin *pl = sm->schedulerPlugin();
    if (pl == nullptr) {
        return QVariant();
    }
    int capabilities = pl->capabilities();
    switch (role) {
        case Qt::EditRole:
            return sm->schedulingDirection();
        case Qt::DisplayRole:
            if (capabilities & SchedulerPlugin::ScheduleForward &&
                 capabilities & SchedulerPlugin::ScheduleBackward)
            {
                return sm->schedulingDirection() ? i18n("Backwards") : i18n("Forward");
            }
            if (capabilities & SchedulerPlugin::ScheduleForward) {
                return sm->schedulingDirection() ? i18n("(Backwards)") : i18n("Forward");
            }
            if (capabilities & SchedulerPlugin::ScheduleBackward) {
                return sm->schedulingDirection() ? i18n("Backwards") : i18n("(Forward)");
            }
            break;
        case Qt::ToolTipRole:
            if (capabilities & SchedulerPlugin::ScheduleForward &&
                 capabilities & SchedulerPlugin::ScheduleBackward)
            {
                return sm->schedulingDirection()
                            ? xi18nc("@info:tooltip", "Schedule project from target end time")
                            : xi18nc("@info:tooltip", "Schedule project from target start time");
            }
            if (capabilities & SchedulerPlugin::ScheduleForward) {
                return sm->schedulingDirection()
                            ? xi18nc("@info:tooltip 1=scheduler name", "%1 always schedules from target start time", pl->name())
                            : xi18nc("@info:tooltip", "Schedule project from target start time");
            }
            if (capabilities & SchedulerPlugin::ScheduleBackward) {
                return sm->schedulingDirection()
                            ? xi18nc("@info:tooltip", "Schedule project from target end time")
                            : xi18nc("@info:tooltip 1=scheduler name", "%1 always schedules from target end time", pl->name());
            }
            break;
        case Role::EnumList:
            return QStringList() << xi18nc("@label:listbox", "Forward") << xi18nc("@label:listbox", "Backwards");
        case Role::EnumListValue:
            return sm->schedulingDirection() ? 1 : 0;
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ScheduleItemModel::setSchedulingDirection(const QModelIndex &index, const QVariant &value, int role)
{
    ScheduleManager *sm = manager (index);
    if (sm == nullptr) {
        return false;
    }
    switch (role) {
        case Qt::EditRole:
            Q_EMIT executeCommand(new ModifyScheduleManagerSchedulingDirectionCmd(*sm, value.toBool(), kundo2_i18n("Modify scheduling direction")));
            slotManagerChanged(static_cast<ScheduleManager*>(sm));
            return true;
    }
    return false;
}

QVariant ScheduleItemModel::scheduler(const QModelIndex &index, int role) const
{
    ScheduleManager *sm = manager(index);
    if (sm == nullptr) {
        return QVariant();
    }
    SchedulerPlugin *pl = sm->schedulerPlugin();
    if (pl) {
        switch (role) {
            case Qt::EditRole:
                return sm->schedulerPluginId();
            case Qt::DisplayRole:
                return pl ? pl->name() : i18n("Unknown");
            case Qt::ToolTipRole:
                return pl ? pl->comment() : QString();
            case Role::EnumList:
                return sm->schedulerPluginNames();
            case Role::EnumListValue:
                return sm->schedulerPluginIndex();
            case Qt::TextAlignmentRole:
                return Qt::AlignCenter;
            case Qt::StatusTipRole:
                return QVariant();
            case Qt::WhatsThisRole: {
                QString s = pl->description();
                return s.isEmpty() ? QVariant() : QVariant(s);
            }
        }
    }
    return QVariant();
}

bool ScheduleItemModel::setScheduler(const QModelIndex &index, const QVariant &value, int role)
{
    ScheduleManager *sm = manager(index);
    if (sm != nullptr) {
        switch (role) {
            case Qt::EditRole: {
                Q_EMIT executeCommand(new ModifyScheduleManagerSchedulerCmd(*sm, value.toInt(), kundo2_i18n("Modify scheduler")));
                return true;
            }
        }
    }
    return false;
}

QVariant ScheduleItemModel::isScheduled(const QModelIndex &index, int role) const
{
    ScheduleManager *sm = manager(index);
    if (sm == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::EditRole:
            return sm->isScheduled();
        case Qt::DisplayRole:
            return sm->isScheduled() ? i18n("Scheduled") : i18n("Not scheduled");
        case Qt::ToolTipRole:
            break;
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ScheduleItemModel::granularity(const QModelIndex &index, int role) const
{
    ScheduleManager *sm = manager(index);
    if (sm == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::EditRole:
        case Role::EnumListValue:
            return qMin(sm->granularityIndex(), sm->supportedGranularities().count() - 1);
        case Qt::DisplayRole: {
            QList<long unsigned int> lst = sm->supportedGranularities();
            if (lst.isEmpty()) {
                return i18nc("Scheduling granularity not supported", "None");
            }
            int idx = sm->granularityIndex();
            qulonglong g = idx < lst.count() ? lst[ idx ] : lst.last();
            return KFormat().formatDuration(g);
        }
        case Qt::ToolTipRole: {
            QList<long unsigned int> lst = sm->supportedGranularities();
            if (lst.isEmpty()) {
                return xi18nc("@info:tooltip", "Scheduling granularityIndex not supported");
            }
            int idx = sm->granularityIndex();
            qulonglong g = idx < lst.count() ? lst[ idx ] : lst.last();
            return xi18nc("@info:tooltip", "Selected scheduling granularity: %1", KFormat().formatDuration(g));
        }
        case Qt::TextAlignmentRole:
            return Qt::AlignRight;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::EnumList: {
            QStringList sl;
            KFormat format;
            const QList<long unsigned int> values = sm->supportedGranularities();
            for (long unsigned int v : values) {
                sl << format.formatDuration(v);
            }
            return sl;
        }
    }
    return QVariant();
}

bool ScheduleItemModel::setGranularity(const QModelIndex &index, const QVariant &value, int role)
{
    ScheduleManager *sm = manager(index);
    if (sm != nullptr) {
        switch (role) {
            case Qt::EditRole: {
                Q_EMIT executeCommand(new ModifyScheduleManagerSchedulingGranularityIndexCmd(*sm, value.toInt(), kundo2_i18n("Modify scheduling granularity")));
                return true;
            }
        }
    }
    return false;
}

QVariant ScheduleItemModel::data(const QModelIndex &index, int role) const
{
    //debugPlan<<index.row()<<","<<index.column();
    QVariant result;
    if (role == Qt::TextAlignmentRole) {
        return headerData(index.column(), Qt::Horizontal, role);
    }
    switch (index.column()) {
        case ScheduleModel::ScheduleName: result = name(index, role); break;
        case ScheduleModel::ScheduleState: result = state(index, role); break;
        case ScheduleModel::ScheduleDirection: result = schedulingDirection(index, role); break;
        case ScheduleModel::ScheduleOverbooking: result = allowOverbooking(index, role); break;
        case ScheduleModel::ScheduleDistribution: result = usePert(index, role); break;
        case ScheduleModel::SchedulePlannedStart: result = projectStart( index, role); break;
        case ScheduleModel::SchedulePlannedFinish: result = projectEnd(index, role); break;
        case ScheduleModel::ScheduleScheduler: result = scheduler(index, role); break;
        case ScheduleModel::ScheduleGranularity: result = granularity(index, role); break;
        case ScheduleModel::ScheduleScheduled: result = isScheduled(index, role); break;
        case ScheduleModel::ScheduleMode: result = schedulingMode(index, role); break;
        default:
            debugPlan<<"data: invalid display value column"<<index.column();
            return QVariant();
    }
    if (result.isValid()) {
        if (role == Qt::DisplayRole && result.type() == QVariant::String && result.toString().isEmpty()) {
            // HACK to show focus in empty cells
            result = ' ';
        }
        return result;
    }
    return QVariant();
}

bool ScheduleItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (! index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    if ((flags(index) & (Qt::ItemIsEditable | Qt::ItemIsUserCheckable)) == 0 || (role != Qt::EditRole && role != Qt::CheckStateRole)) {
        return false;
    }
    switch (index.column()) {
        case ScheduleModel::ScheduleName: return setName(index, value, role);
        case ScheduleModel::ScheduleState: return setState(index, value, role);
        case ScheduleModel::ScheduleDirection: return setSchedulingDirection(index, value, role);
        case ScheduleModel::ScheduleOverbooking: return setAllowOverbooking(index, value, role);
        case ScheduleModel::ScheduleDistribution: return setUsePert(index, value, role);
//        case ScheduleModel::ScheduleCalculate: return setCalculateAll(index, value, role);
        case ScheduleModel::SchedulePlannedStart: return false;
        case ScheduleModel::SchedulePlannedFinish: return false;
        case ScheduleModel::ScheduleScheduler: return setScheduler(index, value, role); break;
        case ScheduleModel::ScheduleGranularity: return setGranularity(index, value, role);
        case ScheduleModel::ScheduleScheduled: break;
        case ScheduleModel::ScheduleMode: return setSchedulingMode(index, value, role);
        default:
            qWarning("data: invalid display value column %d", index.column());
            break;
    }
    return false;
}

QVariant ScheduleItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section) {
                case ScheduleModel::ScheduleName: return i18n("Name");
                case ScheduleModel::ScheduleState: return i18n("State");
                case ScheduleModel::ScheduleMode: return i18n("Mode");
                case ScheduleModel::ScheduleDirection: return i18n("Direction");
                case ScheduleModel::ScheduleOverbooking: return i18n("Overbooking");
                case ScheduleModel::ScheduleDistribution: return i18n("Distribution");
//                case ScheduleModel::ScheduleCalculate: return i18n("Calculate");
                case ScheduleModel::SchedulePlannedStart: return i18n("Planned Start");
                case ScheduleModel::SchedulePlannedFinish: return i18n("Planned Finish");
                case ScheduleModel::ScheduleScheduler: return i18n("Scheduler");
                case ScheduleModel::ScheduleGranularity: return xi18nc("title:column", "Granularity");
                case ScheduleModel::ScheduleScheduled: return i18n("Scheduled");
                default: return QVariant();
            }
        } else if (role == Qt::EditRole) {
            switch (section) {
                case ScheduleModel::ScheduleName: return QStringLiteral("Name");
                case ScheduleModel::ScheduleState: return QStringLiteral("State");
                case ScheduleModel::ScheduleMode: return QStringLiteral("Mode");
                case ScheduleModel::ScheduleDirection: return QStringLiteral("Direction");
                case ScheduleModel::ScheduleOverbooking: return QStringLiteral("Overbooking");
                case ScheduleModel::ScheduleDistribution: return QStringLiteral("Distribution");
                //                case ScheduleModel::ScheduleCalculate: return i18n("Calculate");
                case ScheduleModel::SchedulePlannedStart: return QStringLiteral("Planned Start");
                case ScheduleModel::SchedulePlannedFinish: return QStringLiteral("Planned Finish");
                case ScheduleModel::ScheduleScheduler: return QStringLiteral("Scheduler");
                case ScheduleModel::ScheduleGranularity: return QStringLiteral("Granularity");
                case ScheduleModel::ScheduleScheduled: return QStringLiteral("Scheduled");
                default: return QVariant();
            }
        } else if (role == Qt::TextAlignmentRole) {
            return QVariant();
        }
    }
    if (role == Qt::ToolTipRole) {
        switch (section) {
            case ScheduleModel::ScheduleName: return ToolTip::scheduleName();
            case ScheduleModel::ScheduleState: return ToolTip::scheduleState();
            case ScheduleModel::ScheduleDirection: return ToolTip::schedulingDirection();
            case ScheduleModel::ScheduleOverbooking: return ToolTip::scheduleOverbooking();
            case ScheduleModel::ScheduleDistribution: return ToolTip::scheduleDistribution();
//            case ScheduleModel::ScheduleCalculate: return ToolTip::scheduleCalculate();
            case ScheduleModel::SchedulePlannedStart: return ToolTip::scheduleStart();
            case ScheduleModel::SchedulePlannedFinish: return ToolTip::scheduleFinish();
            case ScheduleModel::ScheduleScheduler: return ToolTip::scheduleScheduler();
            case ScheduleModel::ScheduleGranularity: return ToolTip::scheduleGranularity();
            case ScheduleModel::ScheduleScheduled: return QVariant();
            case ScheduleModel::ScheduleMode: return ToolTip::scheduleMode();
            default: return QVariant();
        }
    } else if (role == Qt::WhatsThisRole) {
        switch (section) {
            case ScheduleModel::ScheduleDirection: return WhatsThis::schedulingDirection();
            case ScheduleModel::ScheduleOverbooking: return WhatsThis::scheduleOverbooking();
            case ScheduleModel::ScheduleDistribution: return WhatsThis::scheduleDistribution();
            case ScheduleModel::ScheduleScheduler: return WhatsThis::scheduleScheduler();
            default: return QVariant();
        }
    }

    return ItemModelBase::headerData(section, orientation, role);
}

QAbstractItemDelegate *ScheduleItemModel::createDelegate(int column, QWidget *parent) const
{
    switch (column) {
        case ScheduleModel::ScheduleState: return new ProgressBarDelegate(parent);
        case ScheduleModel::ScheduleMode: return new EnumDelegate(parent);
        case ScheduleModel::ScheduleDirection: return new EnumDelegate(parent);
        case ScheduleModel::ScheduleOverbooking: return new EnumDelegate(parent);
        case ScheduleModel::ScheduleDistribution: return new EnumDelegate(parent);
        case ScheduleModel::ScheduleGranularity: return new EnumDelegate(parent);
        case ScheduleModel::ScheduleScheduler: return new EnumDelegate(parent);
    }
    return nullptr;
}

void ScheduleItemModel::sort(int column, Qt::SortOrder order)
{
    Q_UNUSED(column);
    Q_UNUSED(order);
}

QMimeData * ScheduleItemModel::mimeData(const QModelIndexList &) const
{
    return nullptr;
}

QStringList ScheduleItemModel::mimeTypes () const
{
    return QStringList()
            << QStringLiteral("text/html")
            << QStringLiteral("text/plain");
}

ScheduleManager *ScheduleItemModel::manager(const QModelIndex &index) const
{
    ScheduleManager *o = nullptr;
    if (index.isValid() && m_project != nullptr && index.internalPointer() != nullptr && m_project->isScheduleManager(index.internalPointer())) {
        o = static_cast<ScheduleManager*>(index.internalPointer());
        Q_ASSERT(o);
    }
    return o;
}

void ScheduleItemModel::setFlat(bool flat)
{
    m_flat = flat;
    m_managerlist.clear();
    if (! flat || m_project == nullptr) {
        return;
    }
    m_managerlist = m_project->allScheduleManagers();
}

//--------------------------------------
ScheduleSortFilterModel::ScheduleSortFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

ScheduleSortFilterModel::~ScheduleSortFilterModel()
{
}

ScheduleManager *ScheduleSortFilterModel::manager(const QModelIndex &index) const
{
    QModelIndex i = mapToSource(index);
    const ScheduleItemModel *m = qobject_cast<const ScheduleItemModel*>(i.model());
    return m == nullptr ? nullptr : m->manager(i);
}

QVariant ScheduleSortFilterModel::data(const QModelIndex &index, int role) const
{
    QVariant v = QSortFilterProxyModel::data(index, role);
    if (role == Qt::DisplayRole && index.column() == ScheduleModel::ScheduleName) {
        QModelIndex state = index.sibling(index.row(), ScheduleModel::ScheduleState);
        if (state.data(Qt::EditRole).toString() != QStringLiteral("Scheduled")) {
            QString s = state.data(SpecialScheduleDisplayRole).toString();
            if (!s.isEmpty()) {
                return QStringLiteral("%1 (%2)").arg(v.toString(), s); // TODO i18n
            }
        }
    }
    return v;
}

//--------------------------------------
ScheduleLogItemModel::ScheduleLogItemModel(QObject *parent)
    : QStandardItemModel(parent),
    m_project(nullptr),
    m_manager(nullptr),
    m_schedule(nullptr)
{
}

ScheduleLogItemModel::~ScheduleLogItemModel()
{
}

void ScheduleLogItemModel::slotScheduleManagerToBeRemoved(const ScheduleManager *manager)
{
    if (m_manager == manager) {
        setManager(nullptr);
    }
}

void ScheduleLogItemModel::slotScheduleManagerRemoved(const ScheduleManager *manager)
{
    debugPlan<<manager->name();
}

void ScheduleLogItemModel::slotScheduleToBeInserted(const ScheduleManager *manager, int row)
{
    Q_UNUSED(manager);
    Q_UNUSED(row);
    if (m_manager && m_manager->expected() /*== ??*/) {
        //TODO
    }
}
//FIXME remove const on MainSchedule
void ScheduleLogItemModel::slotScheduleInserted(const MainSchedule *sch)
{
    debugPlan<<m_schedule<<sch;
    if (m_manager && m_manager == sch->manager() && sch == m_manager->expected()) {
        m_schedule = const_cast<MainSchedule*>(sch);
        refresh();
    }
}

void ScheduleLogItemModel::slotScheduleToBeRemoved(const MainSchedule *sch)
{
    debugPlan<<m_schedule<<sch;
    if (m_schedule == sch) {
        m_schedule = nullptr;
        clear();
    }
}

void ScheduleLogItemModel::slotScheduleRemoved(const MainSchedule *sch)
{
    debugPlan<<m_schedule<<sch;
}

void ScheduleLogItemModel::projectDeleted()
{
    setProject(nullptr);
}

void ScheduleLogItemModel::setProject(Project *project)
{
    debugPlan<<m_project<<"->"<<project;
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ScheduleLogItemModel::projectDeleted);

        disconnect(m_project, &Project::scheduleManagerChanged, this, &ScheduleLogItemModel::slotManagerChanged);

        disconnect(m_project, &Project::scheduleManagerToBeRemoved, this, &ScheduleLogItemModel::slotScheduleManagerToBeRemoved);

        disconnect(m_project, &Project::scheduleManagerRemoved, this, &ScheduleLogItemModel::slotScheduleManagerRemoved);

        disconnect(m_project, &Project::scheduleChanged, this, &ScheduleLogItemModel::slotScheduleChanged);

        disconnect(m_project, &Project::scheduleToBeAdded, this, &ScheduleLogItemModel::slotScheduleToBeInserted);

        disconnect(m_project, &Project::scheduleToBeRemoved, this, &ScheduleLogItemModel::slotScheduleToBeRemoved);

        disconnect(m_project, &Project::scheduleAdded, this, &ScheduleLogItemModel::slotScheduleInserted);

        disconnect(m_project, &Project::scheduleRemoved, this, &ScheduleLogItemModel::slotScheduleRemoved);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &ScheduleLogItemModel::projectDeleted);

        connect(m_project, &Project::scheduleManagerChanged, this, &ScheduleLogItemModel::slotManagerChanged);

        connect(m_project, &Project::scheduleManagerToBeRemoved, this, &ScheduleLogItemModel::slotScheduleManagerToBeRemoved);

        connect(m_project, &Project::scheduleManagerRemoved, this, &ScheduleLogItemModel::slotScheduleManagerRemoved);

        connect(m_project, &Project::scheduleChanged, this, &ScheduleLogItemModel::slotScheduleChanged);

        connect(m_project, &Project::scheduleToBeAdded, this, &ScheduleLogItemModel::slotScheduleToBeInserted);

        connect(m_project, &Project::scheduleToBeRemoved, this, &ScheduleLogItemModel::slotScheduleToBeRemoved);

        connect(m_project, &Project::scheduleAdded, this, &ScheduleLogItemModel::slotScheduleInserted);

        connect(m_project, &Project::scheduleRemoved, this, &ScheduleLogItemModel::slotScheduleRemoved);
    }
}

void ScheduleLogItemModel::setManager(ScheduleManager *manager)
{
    debugPlan<<m_manager<<"->"<<manager;
    if (manager != m_manager) {
        if (m_manager) {
            disconnect(m_manager, &ScheduleManager::logInserted, this, &ScheduleLogItemModel::slotLogInserted);
        }
        m_manager = manager;
        m_schedule = nullptr;
        clear();
        if (m_manager) {
            m_schedule = m_manager->expected();
            refresh();
            connect(m_manager, &ScheduleManager::logInserted, this, &ScheduleLogItemModel::slotLogInserted);
        }
    }
}

void ScheduleLogItemModel::slotLogInserted(MainSchedule *s, int firstrow, int lastrow)
{
    for (int i = firstrow; i <= lastrow; ++i) {
        addLogEntry(s->logs().value(i), i + 1);
    }
}

//FIXME: This only add logs (insert is not used atm)
void ScheduleLogItemModel::addLogEntry(const Schedule::Log &log, int /*row*/)
{
//     debugPlan<<log;
    QList<QStandardItem*> lst;
    if (log.resource) {
        lst.append(new QStandardItem(log.resource->name()));
    } else if (log.node) {
        lst.append(new QStandardItem(log.node->name()));
    } else {
        lst.append(new QStandardItem());
    }
    lst.append(new QStandardItem(m_schedule->logPhase(log.phase)));
    QStandardItem *item = new QStandardItem(m_schedule->logSeverity(log.severity));
    item->setData(log.severity, SeverityRole);
    lst.append(item);
    lst.append(new QStandardItem(log.message));
    for (QStandardItem *itm : std::as_const(lst)) {
            if (log.resource) {
                itm->setData(log.resource->id(), IdentityRole);
            } else if (log.node) {
                itm->setData(log.node->id(), IdentityRole);
            }
            switch (log.severity) {
            case Schedule::Log::Type_Debug:
                itm->setData(QColor(Qt::darkYellow), Qt::ForegroundRole);
                break;
            case Schedule::Log::Type_Info:
                break;
            case Schedule::Log::Type_Warning:
                itm->setData(QColor(Qt::blue), Qt::ForegroundRole);
                break;
            case Schedule::Log::Type_Error:
                itm->setData(QColor(Qt::red), Qt::ForegroundRole);
                break;
            default:
                break;
        }
    }
    appendRow(lst);
//     debugPlan<<"added:"<<row<<rowCount()<<columnCount();
}

void ScheduleLogItemModel::refresh()
{
    clear();
    QStringList lst;
    lst << i18n("Name") << i18n("Phase") << i18n("Severity") << i18n("Message");
    setHorizontalHeaderLabels(lst);

    if (m_schedule == nullptr) {
        debugPlan<<"No main schedule";
        return;
    }
//     debugPlan<<m_schedule<<m_schedule->logs().count();
    int i = 1;
    const QVector<Schedule::Log> logs = m_schedule->logs();
    for (const Schedule::Log &l : logs) {
        addLogEntry(l, i++);
    }
}

QString ScheduleLogItemModel::identity(const QModelIndex &idx) const
{
    QStandardItem *itm = itemFromIndex(idx);
    return itm ? itm->data(IdentityRole).toString() : QString();
}

void ScheduleLogItemModel::slotManagerChanged(ScheduleManager *manager)
{
    debugPlan<<m_manager<<manager;
    if (m_manager == manager) {
        //TODO
//        refresh();
    }
}


void ScheduleLogItemModel::slotScheduleChanged(MainSchedule *sch)
{
    debugPlan<<m_schedule<<sch;
    if (m_schedule == sch) {
        refresh();
    }
}


Qt::ItemFlags ScheduleLogItemModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

} // namespace KPlato
