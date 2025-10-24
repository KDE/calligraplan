/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptcalendarmodel.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptdatetime.h"
#include "kcalendar/kdatetable.h"
#include "kptdebug.h"

#include <QMimeData>
#include <QPainter>
#include <QLocale>
#include <QTimeZone>
#include <QApplication>
#include <QPalette>

#include <KFormat>
#ifdef HAVE_KHOLIDAYS
#include <KHolidays/HolidayRegion>
#endif

namespace KPlato
{


//-----------------------------------------
CalendarDayItemModelBase::CalendarDayItemModelBase(QObject *parent)
    : ItemModelBase(parent),
    m_calendar(nullptr)
{
}

CalendarDayItemModelBase::~CalendarDayItemModelBase()
{
}

void CalendarDayItemModelBase::slotCalendarToBeRemoved(const Calendar *calendar)
{
    if (calendar && calendar == m_calendar) {
        setCalendar(nullptr);
    }
}


void CalendarDayItemModelBase::setCalendar(Calendar *calendar)
{
    m_calendar = calendar;
}

void CalendarDayItemModelBase::setProject(Project *project)
{
    beginResetModel();
    setCalendar(nullptr);
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &CalendarDayItemModelBase::projectDeleted);
        disconnect(m_project, &Project::calendarToBeRemoved, this, &CalendarDayItemModelBase::slotCalendarToBeRemoved);
    }
    m_project = project;
    if (project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &CalendarDayItemModelBase::projectDeleted);
        connect(m_project, &Project::calendarToBeRemoved, this, &CalendarDayItemModelBase::slotCalendarToBeRemoved);
    }
    endResetModel();
}


//-------------------------------------
CalendarItemModel::CalendarItemModel(QObject *parent)
    : ItemModelBase(parent),
    m_calendar(nullptr)
{
}

CalendarItemModel::~CalendarItemModel()
{
}

const QMetaEnum CalendarItemModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

void CalendarItemModel::slotCalendarToBeInserted(const Calendar *parent, int row)
{
    //debugPlan<<(parent?parent->name():"Top level")<<","<<row;
    Q_ASSERT(m_calendar == nullptr);
    m_calendar = const_cast<Calendar *>(parent);
    beginInsertRows(index(parent), row, row);
}

void CalendarItemModel::slotCalendarInserted(const Calendar *calendar)
{
    //debugPlan<<calendar->name();
    Q_ASSERT(calendar->parentCal() == m_calendar);
#ifdef NDEBUG
    Q_UNUSED(calendar)
#endif

    endInsertRows();
    m_calendar = nullptr;
}

void CalendarItemModel::slotCalendarToBeRemoved(const Calendar *calendar)
{
    //debugPlan<<calendar->name();
    int row = index(calendar).row();
    beginRemoveRows(index(calendar->parentCal()), row, row);
}

void CalendarItemModel::slotCalendarRemoved(const Calendar *)
{
    //debugPlan<<calendar->name();
    endRemoveRows();
}

void CalendarItemModel::setProject(Project *project)
{
    beginResetModel();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &CalendarItemModel::projectDeleted);
        disconnect(m_project , &Project::calendarChanged, this, &CalendarItemModel::slotCalendarChanged);

        disconnect(m_project, &Project::calendarAdded, this, &CalendarItemModel::slotCalendarInserted);
        disconnect(m_project, &Project::calendarToBeAdded, this, &CalendarItemModel::slotCalendarToBeInserted);

        disconnect(m_project, &Project::calendarRemoved, this, &CalendarItemModel::slotCalendarRemoved);
        disconnect(m_project, &Project::calendarToBeRemoved, this, &CalendarItemModel::slotCalendarToBeRemoved);
    }
    m_project = project;
    if (project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &CalendarItemModel::projectDeleted);
        connect(m_project, &Project::calendarChanged, this, &CalendarItemModel::slotCalendarChanged);

        connect(m_project, &Project::calendarAdded, this, &CalendarItemModel::slotCalendarInserted);
        connect(m_project, &Project::calendarToBeAdded, this, &CalendarItemModel::slotCalendarToBeInserted);

        connect(m_project, &Project::calendarRemoved, this, &CalendarItemModel::slotCalendarRemoved);
        connect(m_project, &Project::calendarToBeRemoved, this, &CalendarItemModel::slotCalendarToBeRemoved);
    }
    endResetModel();
}

Qt::ItemFlags CalendarItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = ItemModelBase::flags(index);
    if (!m_readWrite) {
        return flags &= ~Qt::ItemIsEditable;
    }
    flags |= Qt::ItemIsDropEnabled;
    if (!index.isValid()) {
        return flags;
    }
    Calendar *c = calendar(index);
    if (!c || c->isShared()) {
        if (index.column() == Name) {
            flags |= Qt::ItemIsUserCheckable;
        }
        return flags;
    }
    flags |= Qt::ItemIsDragEnabled;
    if (calendar (index)) {
        switch (index.column()) {
            case Name:
                flags |= (Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
                break;
            case Origin:
                flags &= ~Qt::ItemIsEditable;
                break;
            case TimeZone:
                if (parent(index).isValid()) {
                    flags &= ~Qt::ItemIsEditable;
                } else {
                    flags |= Qt::ItemIsEditable;
                }
                break;
#ifdef HAVE_KHOLIDAYS
            case HolidayRegion:
                flags |= Qt::ItemIsEditable;
                break;
#endif
            default:
                flags |= Qt::ItemIsEditable;
                break;
        }
    }
    return flags;
}


QModelIndex CalendarItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || m_project == nullptr) {
        return QModelIndex();
    }
    //debugPlan<<index.internalPointer()<<":"<<index.row()<<","<<index.column();
    Calendar *a = calendar(index);
    if (a == nullptr) {
        return QModelIndex();
    }
    Calendar *par = a->parentCal();
    if (par) {
        a = par->parentCal();
        int row = -1;
        if (a) {
            row = a->indexOf(par);
        } else {
            row = m_project->indexOf(par);
        }
        //debugPlan<<par->name()<<":"<<row;
        return createIndex(row, 0, par);
    }
    return QModelIndex();
}

QModelIndex CalendarItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    Calendar *par = calendar(parent);
    if (par == nullptr) {
        if (row < m_project->calendars().count()) {
            return createIndex(row, column, m_project->calendars().at(row));
        }
    } else if (row < par->calendars().count()) {
        return createIndex(row, column, par->calendars().at(row));
    }
    return QModelIndex();
}

QModelIndex CalendarItemModel::index(const Calendar *calendar, int column) const
{
    if (m_project == nullptr || calendar == nullptr) {
        return QModelIndex();
    }
    Calendar *a = const_cast<Calendar*>(calendar);
    int row = -1;
    Calendar *par = a->parentCal();
    if (par == nullptr) {
         row = m_project->calendars().indexOf(a);
    } else {
        row = par->indexOf(a);
    }
    if (row == -1) {
        return QModelIndex();
    }
    return createIndex(row, column, a);

}

int CalendarItemModel::columnCount(const QModelIndex &) const
{
    return columnMap().keyCount();
}

int CalendarItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == nullptr) {
        return 0;
    }
    Calendar *par = calendar(parent);
    if (par == nullptr) {
        return m_project->calendars().count();
    }
    return par->calendars().count();
}

QVariant CalendarItemModel::name(const Calendar *a, int role) const
{
    //debugPlan<<res->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return a->name();
        case Qt::ToolTipRole:
            if (a->isDefault()) {
                return xi18nc("1=calendar name", "%1 (Default calendar)", a->name());
            }
            return a->name();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::CheckStateRole:
            return a->isDefault() ? Qt::Checked : Qt::Unchecked;
    }
    return QVariant();
}

QVariant CalendarItemModel::origin(const Calendar *a, int role) const
{
    //debugPlan<<res->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
            return a->isShared() ? i18n("Shared") : i18n("Local");
        case Qt::EditRole:
            return a->isShared() ? QStringLiteral("Shared") : QStringLiteral("Local");
        case Qt::ToolTipRole:
            if (!a->isShared()) {
                return xi18nc("@info:tooltip 1=calendar name", "%1 is a <emphasis>Local</emphasis> calendar", a->name());
            }
            return xi18nc("@info:tooltip 1=calendar name", "%1 is a <emphasis>Shared</emphasis> calendar", a->name());
        case Role::EnumList:
            return QStringList() << i18n("Shared") << i18n("Local");
        case Role::EnumListValue:
            return a->isShared() ? 0 : 1;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool CalendarItemModel::setName(Calendar *a, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toString() != a->name()) {
                Q_EMIT executeCommand(new CalendarModifyNameCmd(a, value.toString(), kundo2_i18n("Modify calendar name")));
                return true;
            }
            break;
        case Qt::CheckStateRole: {
            switch (value.toInt()) {
                case Qt::Unchecked:
                    if (a->isDefault()) {
                        Q_EMIT executeCommand(new ProjectModifyDefaultCalendarCmd(m_project, nullptr, kundo2_i18n("De-select as default calendar")));
                        return true;
                    }
                    break;
                case Qt::Checked:
                    if (! a->isDefault()) {
                        Q_EMIT executeCommand(new ProjectModifyDefaultCalendarCmd(m_project, a, kundo2_i18n("Select as default calendar")));
                        return true;
                    }
                    break;
                default: break;
            }
        }
        default: break;
    }
    return false;
}

QVariant CalendarItemModel::timeZone(const Calendar *a, int role) const
{
    //debugPlan<<res->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return i18n(a->timeZone().id().constData());
        case Role::EnumList: {
            QStringList lst;
            const QList<QByteArray> zones = QTimeZone::availableTimeZoneIds();
            for (const QByteArray &id : zones) {
                lst << i18n(id.constData());
            }
            lst.sort();
            return lst;
        }
        case Role::EnumListValue: {
            QStringList lst = timeZone(a, Role::EnumList).toStringList();
            return lst.indexOf(i18n(a->timeZone().id().constData()));
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool CalendarItemModel::setTimeZone(Calendar *a, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            if (timeZone(a, Role::EnumListValue) == value.toInt()) {
                return false;
            }
            QStringList lst = timeZone(a, Role::EnumList).toStringList();
            QString name = lst.value(value.toInt());
            QTimeZone tz;
            const QList<QByteArray> zones = QTimeZone::availableTimeZoneIds();
            for (const QByteArray &id : zones) {
                if (name == i18n(id.constData())) {
                    tz = QTimeZone(id);
                    break;
                }
            }
            if (!tz.isValid()) {
                return false;
            }
            Q_EMIT executeCommand(new CalendarModifyTimeZoneCmd(a, tz, kundo2_i18n("Modify calendar timezone")));
            return true;
        }
    }
    return false;
}

#ifdef HAVE_KHOLIDAYS
QVariant CalendarItemModel::holidayRegion(const Calendar *a, int role) const
{
    //debugPlan<<res->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
            if (a->holidayRegionCode().isEmpty() || !a->holidayRegion()->isValid()) {
                return i18n("None");
            }
            if (a->holidayRegionCode() == QStringLiteral("Default")) {
                return i18n("Default");
            }
            return a->holidayRegion()->name();
        case Qt::EditRole:
            if (a->holidayRegionCode().isEmpty()) {
                return QStringLiteral("None");
            }
            return a->holidayRegionCode();
        case Qt::ToolTipRole:
            if (!a->holidayRegion()->isValid()) {
                return xi18nc("@info:tooltip", "No holidays");
            } else if (a->holidayRegionCode() == QStringLiteral("Default")) {
                return xi18nc("@info:tooltip", "Default region: <emphasis>%1</emphasis>", a->holidayRegion()->name());
            }
            return a->holidayRegion()->description();
        case Role::EnumList: {
            QStringList lst;
            lst << i18n("None") << i18n("Default");
            const QStringList codes = a->holidayRegionCodes();
            for (const QString &code : codes) {
                lst << KHolidays::HolidayRegion::name(code);
            }
            return lst;
        }
        case Role::EnumListValue: {
            if (!a->holidayRegion()->isValid()) {
                return 0; // None
            }
            if (a->holidayRegionCode() == QStringLiteral("Default")) {
                return 1;
            }
            return a->holidayRegionCodes().indexOf(a->holidayRegionCode()) + 2;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool CalendarItemModel::setHolidayRegion(Calendar *a, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            QString code = QStringLiteral("None");
            if (value.toInt() == 1) {
                code = QStringLiteral("Default");
            } else if (value.toInt() > 1) {
                code = a->holidayRegionCodes().value(value.toInt() - 2);
            }
            if (a->holidayRegionCode() == code || (code == QStringLiteral("None") && a->holidayRegionCode().isEmpty())) {
                return false;
            }
            Q_EMIT executeCommand(new CalendarModifyHolidayRegionCmd(a, code, kundo2_i18n("Modify calendar holiday region")));
            return true;
        }
    }
    return false;
}
#endif

QVariant CalendarItemModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    Calendar *a = calendar(index);
    if (a == nullptr) {
        return QVariant();
    }
    switch (index.column()) {
        case Name: result = name(a, role); break;
        case Origin: result = origin(a, role); break;
        case TimeZone: result = timeZone(a, role); break;
#ifdef HAVE_KHOLIDAYS
        case HolidayRegion: result = holidayRegion(a, role); break;
#endif
        default:
            debugPlan<<"data: invalid display value column"<<index.column();
            return QVariant();
    }
    return result;
}

bool CalendarItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (! index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    if ((flags(index) &(Qt::ItemIsEditable | Qt::CheckStateRole)) == 0) {
        Q_ASSERT(true);
        return false;
    }
    Calendar *a = calendar(index);
    switch (index.column()) {
        case Name: return setName(a, value, role);
        case Origin: return false;
        case TimeZone: return setTimeZone(a, value, role);
#ifdef HAVE_KHOLIDAYS
        case HolidayRegion: return setHolidayRegion(a, value, role);
#endif
        default:
            warnPlan<<"data: invalid display value column "<<index.column();
            return false;
    }
    return false;
}

QVariant CalendarItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section) {
                case Name: return xi18nc("@title:column", "Name");
                case Origin: return xi18nc("@title:column", "Origin");
                case TimeZone: return xi18nc("@title:column", "Timezone");
#ifdef HAVE_KHOLIDAYS
                case HolidayRegion: return xi18nc("@title:column", "Holiday Region");
#endif
                default: return QVariant();
            }
        } else if (role == Qt::TextAlignmentRole) {
            switch (section) {
                default: return QVariant();
            }
        }
    }
    if (role == Qt::ToolTipRole) {
        switch (section) {
            case Name: return ToolTip::calendarName();
            case Origin: return QVariant();
            case TimeZone: return ToolTip::calendarTimeZone();
#ifdef HAVE_KHOLIDAYS
            case HolidayRegion: return xi18nc("@info:tooltip", "The holiday region");
#endif
            default: return QVariant();
        }
    }
    return ItemModelBase::headerData(section, orientation, role);
}

Calendar *CalendarItemModel::calendar(const QModelIndex &index) const
{
    return static_cast<Calendar*>(index.internalPointer());
}

void CalendarItemModel::slotCalendarChanged(Calendar *calendar)
{
    Calendar *par = calendar->parentCal();
    if (par) {
        int row = par->indexOf(calendar);
        Q_EMIT dataChanged(createIndex(row, 0, calendar), createIndex(row, columnCount() - 1, calendar));
    } else {
        int row = m_project->indexOf(calendar);
        Q_EMIT dataChanged(createIndex(row, 0, calendar), createIndex(row, columnCount() - 1, calendar));
    }
}

Qt::DropActions CalendarItemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


QStringList CalendarItemModel::mimeTypes() const
{
    return QStringList() << QStringLiteral("application/x-vnd.kde.plan.calendarid.internal");
}

QMimeData *CalendarItemModel::mimeData(const QModelIndexList & indexes) const
{
    QMimeData *m = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODeviceBase::WriteOnly);
    QList<int> rows;
    for (const QModelIndex &index : indexes) {
        if (index.isValid() && !rows.contains(index.row())) {
            debugPlan<<index.row();
            Calendar *c = calendar(index);
            if (c) {
                stream << c->id();
            }
        }
    }
    m->setData(QStringLiteral("application/x-vnd.kde.plan.calendarid.internal"), encodedData);
    return m;
}

bool CalendarItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int /*column*/, const QModelIndex &parent)
{
    debugPlan<<action<<row;
    if (action == Qt::IgnoreAction) {
        return true;
    }
    if (!data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.calendarid.internal"))) {
        return false;
    }
    if (action == Qt::MoveAction) {
        debugPlan<<"MoveAction";

        QByteArray encodedData = data->data(QStringLiteral("application/x-vnd.kde.plan.calendarid.internal"));
        QDataStream stream(&encodedData, QIODeviceBase::ReadOnly);
        Calendar *par = nullptr;
        if (parent.isValid()) {
            par = calendar(parent);
        }
        MacroCommand *cmd = nullptr;
        const QList<Calendar*> lst = calendarList(stream);
        for (Calendar *c : lst) {
            if (c->parentCal() != par) {
                if (cmd == nullptr) cmd = new MacroCommand(kundo2_i18n("Re-parent calendar"));
                cmd->addCommand(new CalendarModifyParentCmd(m_project, c, par));
            } else {
                if (cmd == nullptr) cmd = new MacroCommand(kundo2_i18n("Move calendar"));
                cmd->addCommand(new CalendarMoveCmd(m_project, c, row, par));
            }
        }
        if (cmd) {
            Q_EMIT executeCommand(cmd);
            return true;
        }
        //debugPlan<<row<<","<<column<<" parent="<<parent.row()<<","<<parent.column()<<":"<<par->name();
    }
    return false;
}

QList<Calendar*> CalendarItemModel::calendarList(QDataStream &stream) const
{
    QList<Calendar*> lst;
    while (!stream.atEnd()) {
        QString id;
        stream >> id;
        Calendar *c = m_project->findCalendar(id);
        if (c) {
            lst << c;
        }
    }
    return lst;
}

bool CalendarItemModel::dropAllowed(Calendar *on, const QMimeData *data)
{
    debugPlan<<on<<data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.calendarid.internal"));
    if (!data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.calendarid.internal"))) {
        return false;
    }
    if (on == nullptr && ! (flags(QModelIndex()) & (int)Qt::ItemIsDropEnabled)) {
        return false;
    }
    QByteArray encodedData = data->data(QStringLiteral("application/x-vnd.kde.plan.calendarid.internal"));
    QDataStream stream(&encodedData, QIODeviceBase::ReadOnly);
    const QList<Calendar*> lst = calendarList(stream);
    for (Calendar *c : lst) {
        if ((flags(index(c)) & (int)Qt::ItemIsDropEnabled) == 0) {
            return false;
        }
        if (on != nullptr && on == c->parentCal()) {
            return false;
        }
        if (on != nullptr && (on == c || on->isChildOf(c))) {
            return false;
        }
    }
    return true;
}

QModelIndex CalendarItemModel::insertCalendar (Calendar *calendar, int pos, Calendar *parent)
{
    //debugPlan<<calendar<<pos<<parent;
    Q_EMIT executeCommand(new CalendarAddCmd(m_project, calendar, pos, parent, kundo2_i18n("Add calendar")));
    int row = -1;
    if (parent) {
        row = parent->indexOf(calendar);
    } else {
        row = m_project->indexOf(calendar);
    }
    if (row != -1) {
        //debugPlan<<"Inserted:"<<calendar->name()<<"row="<<row;
        return createIndex(row, 0, calendar);
    }
    return QModelIndex();
}

void CalendarItemModel::removeCalendar(QList<Calendar *> /*lst*/)
{
}

void CalendarItemModel::removeCalendar(Calendar *calendar)
{
    if (calendar == nullptr) {
        return;
    }
    Q_EMIT executeCommand(new CalendarRemoveCmd(m_project, calendar, kundo2_i18n("Delete calendar")));
}


//------------------------------------------
CalendarDayItemModel::CalendarDayItemModel(QObject *parent)
    : CalendarDayItemModelBase(parent)
{
}

CalendarDayItemModel::~CalendarDayItemModel()
{
}

void CalendarDayItemModel::slotWorkIntervalAdded(CalendarDay *day, TimeInterval *ti)
{
    Q_UNUSED(ti);
    //debugPlan<<day<<","<<ti;
    int c = m_calendar->indexOfWeekday(day);
    if (c == -1) {
        return;
    }
    Q_EMIT dataChanged(createIndex(0, c, day), createIndex(0, c, day));
}

void CalendarDayItemModel::slotWorkIntervalRemoved(CalendarDay *day, TimeInterval *ti)
{
    Q_UNUSED(ti);
    int c = m_calendar->indexOfWeekday(day);
    if (c == -1) {
        return;
    }
    Q_EMIT dataChanged(createIndex(0, c, day), createIndex(0, c, day));
}

void CalendarDayItemModel::slotDayChanged(CalendarDay *day)
{
    int c = m_calendar->indexOfWeekday(day);
    if (c == -1) {
        return;
    }
    debugPlan<<day<<", "<<c;
    Q_EMIT dataChanged(createIndex(0, c, day), createIndex(0, c, day));
}

void CalendarDayItemModel::slotTimeIntervalChanged(TimeInterval *ti)
{
    Q_UNUSED(ti);
/*    CalendarDay *d = parentDay(ti);
    if (d == 0) {
        return;
    }
    int row = d->indexOf(ti);
    Q_EMIT dataChanged(createIndex(row, 0, ti), createIndex(row, columnCount() - 1, ti));*/
}

void CalendarDayItemModel::setCalendar(Calendar *calendar)
{
    beginResetModel();
    //debugPlan<<m_calendar<<" -->"<<calendar;
    if (m_calendar) {
        disconnect(m_calendar, static_cast<void (Calendar::*)(CalendarDay*)>(&Calendar::calendarDayChanged), this, &CalendarDayItemModel::slotDayChanged);
        disconnect(m_calendar, static_cast<void (Calendar::*)(TimeInterval*)>(&Calendar::timeIntervalChanged), this, &CalendarDayItemModel::slotTimeIntervalChanged);

        disconnect(m_calendar, &Calendar::workIntervalAdded, this, &CalendarDayItemModel::slotWorkIntervalAdded);
        disconnect(m_calendar, &Calendar::workIntervalRemoved, this, &CalendarDayItemModel::slotWorkIntervalRemoved);
    }
    m_calendar = calendar;
    if (calendar) {
        connect(m_calendar, static_cast<void (Calendar::*)(CalendarDay*)>(&Calendar::calendarDayChanged), this, &CalendarDayItemModel::slotDayChanged);
        connect(m_calendar, static_cast<void (Calendar::*)(TimeInterval*)>(&Calendar::timeIntervalChanged), this, &CalendarDayItemModel::slotTimeIntervalChanged);

        connect(m_calendar, &Calendar::workIntervalAdded, this, &CalendarDayItemModel::slotWorkIntervalAdded);
        connect(m_calendar, &Calendar::workIntervalRemoved, this, &CalendarDayItemModel::slotWorkIntervalRemoved);
    }
    endResetModel();
}

Qt::ItemFlags CalendarDayItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = ItemModelBase::flags(index);
    if (!m_readWrite) {
        return flags &= ~Qt::ItemIsEditable;
    }
    return flags |= Qt::ItemIsEditable;
}

QModelIndex CalendarDayItemModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

bool CalendarDayItemModel::hasChildren(const QModelIndex &parent) const
{
    //debugPlan<<parent.internalPointer()<<":"<<parent.row()<<","<<parent.column();
    if (m_project == nullptr || m_calendar == nullptr) {
        return false;
    }
    return ! parent.isValid();
}

QModelIndex CalendarDayItemModel::index(int row, int column, const QModelIndex &par) const
{
    if (m_project == nullptr || m_calendar == nullptr) {
        return QModelIndex();
    }
    if (par.isValid()) {
        return QModelIndex();
    }
    CalendarDay *d = m_calendar->weekday(column + 1); // weekdays are 1..7
    if (d == nullptr) {
        return QModelIndex();
    }
    return createIndex(row, column, d);
}

QModelIndex CalendarDayItemModel::index(const CalendarDay *d) const
{
    if (m_project == nullptr || m_calendar == nullptr) {
        return QModelIndex();
    }
    int col = m_calendar->indexOfWeekday(d);
    if (col == -1) {
        return QModelIndex();
    }
    return createIndex(0, col, const_cast<CalendarDay*>(d));
}

int CalendarDayItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 7;
}

int CalendarDayItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == nullptr || m_calendar == nullptr || parent.isValid()) {
        return 0;
    }
    return 1;
}

QVariant CalendarDayItemModel::name(int weekday, int role) const
{
    //debugPlan<<res->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
            if (weekday >= 1 && weekday <= 7) {
                return QLocale().dayName(weekday, QLocale::ShortFormat);
            }
            break;
        case Qt::ToolTipRole:
            if (weekday >= 1 && weekday <= 7) {
                return QLocale().dayName(weekday, QLocale::LongFormat);
            }
            break;
        case Qt::EditRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant CalendarDayItemModel::dayState(const CalendarDay *d, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            switch (d->state()) {
                case CalendarDay::Undefined: return i18nc("Undefined", "U");
                case CalendarDay::NonWorking: return i18nc("NonWorking", "NW");
                case CalendarDay::Working: return i18nc("Working", "W");
            }
            break;
        case Qt::ToolTipRole:
            return CalendarDay::stateToString(d->state(), true);
        case Role::EnumList: {
            QStringList lst = CalendarDay::stateList(true);
            return lst;
        }
        case Qt::EditRole:
        case Role::EnumListValue: {
            return d->state();
        }
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::EditorType:
            return Delegate::EnumEditor;
    }
    return QVariant();
}
bool CalendarDayItemModel::setDayState(CalendarDay *d, const QVariant &value, int role)
{
    //debugPlan;
    switch (role) {
        case Qt::EditRole:
            int v = value.toInt();
            Q_EMIT executeCommand(new CalendarModifyStateCmd(m_calendar, d, static_cast<CalendarDay::State>(v), kundo2_i18n("Modify calendar state")));
            return true;
    }
    return false;
}

QVariant CalendarDayItemModel::workDuration(const CalendarDay *day, int role) const
{
    //debugPlan<<day->date()<<","<<role;
    switch (role) {
        case Qt::DisplayRole: {
            if (day->state() == CalendarDay::Working) {
                return QLocale().toString(day->workDuration().toDouble(Duration::Unit_h), 'f', 1);
            }
            return QVariant();
        }
        case Qt::ToolTipRole: {
            if (day->state() == CalendarDay::Working) {
                QLocale locale;
                QStringList tip;
                const QList<TimeInterval*> intervals = day->timeIntervals();
                for (TimeInterval *i : intervals) {
                    tip <<  i18nc("1=time 2=The number of hours of work duration (non integer)", "%1, %2 hours", locale.toString(i->startTime(), QLocale::ShortFormat), locale.toString(i->hours(), 'f', 2));
                }
                return tip.join(QStringLiteral("\n"));
            }
            return QVariant();
        }
        case Qt::EditRole:
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
    }
    return QVariant();
}

QVariant CalendarDayItemModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    if (! index.isValid()) {
        return result;
    }
    CalendarDay *d = day(index);
    if (d == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
            switch (d->state()) {
                case CalendarDay::Working:
                    result = workDuration(d, role);
                    break;
                case CalendarDay::NonWorking:
                    result = dayState(d, role);
                    break;
                default: {
                    // Return parent value (if any)
                    for (Calendar *c = m_calendar->parentCal(); c != nullptr; c = c->parentCal()) {
                        d = c->weekday(index.column() + 1);
                        Q_ASSERT(d);
                        if (d->state() == CalendarDay::Working) {
                            return workDuration(d, role);
                        }
                        if (d->state() == CalendarDay::NonWorking) {
                           return  dayState(d, role);
                        }
                    }
                    break;
                }
            }
            break;
        }
        case Qt::ToolTipRole: {
            if (d->state() == CalendarDay::Undefined) {
                return xi18nc("@info:tooltip", "Undefined");
            }
            if (d->state() == CalendarDay::NonWorking) {
                return xi18nc("@info:tooltip", "Non-working");
            }
            QLocale locale;
            KFormat format(locale);
            QStringList tip;
                const QList<TimeInterval*> intervals = d->timeIntervals();
                for (TimeInterval *i : intervals) {
                tip <<  xi18nc("@info:tooltip 1=time 2=The work duration (non integer)", "%1, %2", locale.toString(i->startTime(), QLocale::ShortFormat), format.formatDuration(i->second));
            }
            return tip.join(QStringLiteral("<nl/>"));
        }
        case Qt::FontRole: {
            if (d->state() != CalendarDay::Undefined) {
                return QVariant();
            }
            // If defined in parent, return italic
            for (Calendar *c = m_calendar->parentCal(); c != nullptr; c = c->parentCal()) {
                d = c->weekday(index.column() + 1);
                Q_ASSERT(d);
                if (d->state() != CalendarDay::Undefined) {
                    QFont f;
                    f.setItalic(true);
                    return f;
                }
            }
            break;
        }
    }
    return result;
}

bool CalendarDayItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return ItemModelBase::setData(index, value, role);
}

QVariant CalendarDayItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                    return name(section + 1, role);
                default:
                    return QVariant();
             }
         } else if (role == Qt::TextAlignmentRole) {
             switch (section) {
                default: return Qt::AlignCenter;
             }
         }
    }
    if (role == Qt::ToolTipRole) {
        switch (section) {
       /*     case 0: return ToolTip::Calendar Name;*/
            default: return QVariant();
        }
    }
    return ItemModelBase::headerData(section, orientation, role);
}

CalendarDay *CalendarDayItemModel::day(const QModelIndex &index) const
{
    return static_cast<CalendarDay*>(index.internalPointer());
}

QAbstractItemDelegate *CalendarDayItemModel::createDelegate(int column, QWidget *parent) const
{
    Q_UNUSED(parent);
    switch (column) {
        default: return nullptr;
    }
    return nullptr;
}

//-----------------------
DateTableDataModel::DateTableDataModel(QObject *parent)
    : KDateTableDataModel(parent),
    m_calendar(nullptr)
{
}

void DateTableDataModel::setCalendar(Calendar *calendar)
{
    if (m_calendar) {
        disconnect(m_calendar, &Calendar::dayAdded, this, &KDateTableDataModel::reset);
        disconnect(m_calendar, &Calendar::dayRemoved, this, &KDateTableDataModel::reset);
        disconnect(m_calendar, static_cast<void (Calendar::*)(KPlato::CalendarDay*)>(&Calendar::calendarDayChanged), this, &DateTableDataModel::reset);
    }
    m_calendar = calendar;
    if (m_calendar) {
        connect(m_calendar, &Calendar::dayAdded, this, &KDateTableDataModel::reset);
        connect(m_calendar, &Calendar::dayRemoved, this, &KDateTableDataModel::reset);
        connect(m_calendar, static_cast<void (Calendar::*)(KPlato::CalendarDay*)>(&Calendar::calendarDayChanged), this, &DateTableDataModel::reset);
    }
    Q_EMIT reset();
}

QVariant DateTableDataModel::data(const Calendar &cal, const QDate &date, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            CalendarDay *day = cal.findDay(date);
            if (day == nullptr || day->state() == CalendarDay::Undefined) {
#ifdef HAVE_KHOLIDAYS
                if (cal.isHoliday(date)) {
                    return i18nc("NonWorking", "NW");
                }
#endif
                if (cal.parentCal()) {
                    return data(*(cal.parentCal()), date, role);
                }
                return QLatin1String("");
            }
            if (day->state() == CalendarDay::NonWorking) {
                return i18nc("NonWorking", "NW");
            }
            double v;
            v = day->workDuration().toDouble(Duration::Unit_h);
            return QLocale().toString(v, 'f', 1);
        }
        case Qt::TextAlignmentRole:
            return (uint)(Qt::AlignHCenter | Qt::AlignBottom);
        case Qt::FontRole: {
            CalendarDay *day = cal.findDay(date);
            if (day && day->state() != CalendarDay::Undefined) {
                if (&cal != m_calendar) {
                    QFont f;
                    f.setItalic(true);
                    return f;
                }
                return QVariant();
            }
            if (cal.parentCal()) {
                return data(*(cal.parentCal()), date, role);
            }
            break;
        }
        default:
            break;
    }
    return QVariant();
}

QVariant DateTableDataModel::data(const QDate &date, int role, int dataType) const
{
    //debugPlan<<date<<role<<dataType;
    if (role ==  Qt::ToolTipRole) {
        if (m_calendar == nullptr) {
            return QVariant();
        }
        CalendarDay *day = m_calendar->findDay(date);
        if (day == nullptr || day->state() == CalendarDay::Undefined) {
#ifdef HAVE_KHOLIDAYS
            if (m_calendar->isHoliday(date)) {
                return xi18nc("@info:tooltip", "Holiday");
            }
#endif
            return xi18nc("@info:tooltip", "Undefined");
        }
        if (day->state() == CalendarDay::NonWorking) {
            return xi18nc("@info:tooltip", "Non-working");
        }
        QLocale locale;
        KFormat format(locale);
        QStringList tip;
        const QList<TimeInterval*> intervals = day->timeIntervals();
        for (TimeInterval *i : intervals) {
            tip <<  xi18nc("@info:tooltip 1=time 2=The work duration (non integer)", "%1, %2", locale.toString(i->startTime(), QLocale::ShortFormat), format.formatDuration(i->second));
        }
        return tip.join(QStringLiteral("\n"));
    }

    switch (dataType) {
        case -1: { //default (date)
            switch (role) {
                case Qt::DisplayRole: {
                    return QVariant();
                }
                case Qt::TextAlignmentRole:
                    return (uint)Qt::AlignLeft | Qt::AlignTop;
                case Qt::FontRole:
                    break;//return QFont("Helvetica", 6);
                case Qt::BackgroundRole:
#ifdef HAVE_KHOLIDAYS
                    if (m_calendar && m_calendar->isHoliday(date)) {
                        const auto palette = QApplication::palette("QTreeView");
                        return palette.brush(QPalette::AlternateBase);
                    }
#endif
                    break;
                default:
                    break;
            }
            break;
        }
        case 0: {
            if (m_calendar == nullptr) {
                return QLatin1String("");
            }
            if (m_calendar) {
                return data(*m_calendar, date, role);
            }
            break;
        }
        default:
            break;
    }
    return QVariant();
}

QVariant DateTableDataModel::weekDayData(int day, int role) const
{
    Q_UNUSED(day);
    Q_UNUSED(role);
    return QVariant();
}

QVariant DateTableDataModel::weekNumberData(int week, int role) const
{
    Q_UNUSED(week);
    Q_UNUSED(role);
    return QVariant();
}

//-------------
DateTableDateDelegate::DateTableDateDelegate(QObject *parent)
    : KDateTableDateDelegate(parent)
{
}

QRectF DateTableDateDelegate::paint(QPainter *painter, const StyleOptionViewItem &option, const QDate &date, KDateTableDataModel *model)
{
    //debugPlan<<date;
    QRectF r;
    StyleOptionViewItem style = option;
    style.font.setPointSize(style.font.pointSize() - 2);
    //debugPlan<<" fonts: "<<option.font.pointSize()<<style.font.pointSize();
    r = KDateTableDateDelegate::paint(painter, style, date, model);
    if (model == nullptr) {
        return r;
    }
    painter->save();

    painter->translate(r.width(), 0.0);
    QRectF rect(1, 1, option.rectF.right() - r.width(), option.rectF.bottom());
    //debugPlan<<" rects: "<<r<<rect;

    QString text = model->data(date, Qt::DisplayRole, 0).toString();
    int align = model->data(date, Qt::TextAlignmentRole, 0).toInt();
    QFont f = option.font;
    QVariant v = model->data(date, Qt::FontRole, 0);
    if (v.isValid()) {
        f = v.value<QFont>();
    }
    painter->setFont(f);

    v = model->data(date, Qt::BackgroundRole, 0);
    if (v.isValid()) {
        painter->setBackground(style.backgroundBrush);
    } else {
        painter->setBackground(v.value<QBrush>());
    }

    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.highlightedText().color());
    } else {
        painter->setPen(option.palette.color(QPalette::Text));
    }
    painter->drawText(rect, align, text, &r);

    painter->restore();
    return r;
}

//-------------------------------------
CalendarExtendedItemModel::CalendarExtendedItemModel(QObject *parent)
    : CalendarItemModel(parent)
{
}

Qt::ItemFlags CalendarExtendedItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = CalendarItemModel::flags(index);
    if (! m_readWrite || ! index.isValid() || calendar(index) == nullptr) {
        return flags;
    }
    return flags |=  Qt::ItemIsEditable;
}


QModelIndex CalendarExtendedItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    Calendar *par = calendar(parent);
    if (par == nullptr) {
        if (row < m_project->calendars().count()) {
            return createIndex(row, column, m_project->calendars().at(row));
        }
    } else if (row < par->calendars().count()) {
        return createIndex(row, column, par->calendars().at(row));
    }
    return QModelIndex();
}

int CalendarExtendedItemModel::columnCount(const QModelIndex &) const
{
    return CalendarItemModel::columnCount() + 2; // weekdays + date
}

QVariant CalendarExtendedItemModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    Calendar *a = calendar(index);
    if (a == nullptr) {
        return QVariant();
    }
    int col = index.column() - CalendarItemModel::columnCount(index);
    if (col < 0) {
        return CalendarItemModel::data(index, role);
    }
    switch (col) {
        default:
            debugPlan<<"Fetching data from weekdays and date is not supported";
            break;
    }
    return result;
}

bool CalendarExtendedItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int col = index.column() - CalendarItemModel::columnCount(index);
    if (col < 0) {
        return CalendarItemModel::setData(index, value, role);
    }
    if ((flags(index) &(Qt::ItemIsEditable)) == 0) {
        return false;
    }
    Calendar *cal = calendar(index);
    if (cal == nullptr || col > 2) {
        return false;
    }
    switch (col) {
        case 0: { // weekday
            if (value.type() != QVariant::List) {
                return false;
            }
            QVariantList lst = value.toList();
            if (lst.count() < 2) {
                return false;
            }
            int wd = CalendarWeekdays::dayOfWeek(lst.at(0).toString());
            if (wd < 1 || wd > 7) {
                return false;
            }
            CalendarDay *day = new CalendarDay();
            if (lst.count() == 2) {
                QString state = lst.at(1).toString();
                if (state == QStringLiteral("NonWorking")) {
                    day->setState(CalendarDay::NonWorking);
                } else if (state == QStringLiteral("Undefined")) {
                    day->setState(CalendarDay::Undefined);
                } else {
                    delete day;
                    return false;
                }
                CalendarModifyWeekdayCmd *cmd = new CalendarModifyWeekdayCmd(cal, wd, day, kundo2_i18n("Modify calendar weekday"));
                Q_EMIT executeCommand(cmd);
                return true;
            }
            if (lst.count() % 2 == 0) {
                delete day;
                return false;
            }
            day->setState(CalendarDay::Working);
            for (int i = 1; i < lst.count(); i = i + 2) {
                QTime t1 = lst.at(i).toTime();
                QTime t2 = lst.at(i + 1).toTime();
                int length = t1.msecsTo(t2);
                if (t1 == QTime(0, 0, 0) && t2 == t1) {
                    length = 24 * 60 * 60 *1000;
                } else if (length < 0 && t2 == QTime(0, 0, 0)) {
                    length += 24 * 60 * 60 *1000;
                } else if (length == 0 || (length < 0 && t2 != QTime(0, 0, 0))) {
                    delete day;
                    return false;
                }
                length = qAbs(length);
                day->addInterval(t1, length);
            }
            CalendarModifyWeekdayCmd *cmd = new CalendarModifyWeekdayCmd(cal, wd, day, kundo2_i18n("Modify calendar weekday"));
            Q_EMIT executeCommand(cmd);
            return true;
        }
        case 1: { // day
            if (value.type() != QVariant::List) {
                return false;
            }
            CalendarDay *day = new CalendarDay();
            QVariantList lst = value.toList();
            if (lst.count() < 2) {
                delete day;
                return false;
            }
            day->setDate(lst.at(0).toDate());
            if (! day->date().isValid()) {
                delete day;
                return false;
            }
            if (lst.count() == 2) {
                QString state = lst.at(1).toString();
                if (state == QStringLiteral("NonWorking")) {
                    day->setState(CalendarDay::NonWorking);
                } else if (state == QStringLiteral("Undefined")) {
                    day->setState(CalendarDay::Undefined);
                } else {
                    delete day;
                    return false;
                }
                CalendarModifyDayCmd *cmd = new CalendarModifyDayCmd(cal, day, kundo2_i18n("Modify calendar date"));
                Q_EMIT executeCommand(cmd);
                return true;
            }
            if (lst.count() % 2 == 0) {
                delete day;
                return false;
            }
            day->setState(CalendarDay::Working);
            for (int i = 1; i < lst.count(); i = i + 2) {
                QTime t1 = lst.at(i).toTime();
                QTime t2 = lst.at(i + 1).toTime();
                int length = t1.msecsTo(t2);
                if (t1 == QTime(0, 0, 0) && t2 == t1) {
                    length = 24 * 60 * 60 *1000;
                } else if (length < 0 && t2 == QTime(0, 0, 0)) {
                    length += 24 * 60 * 60 *1000;
                } else if (length == 0 || (length < 0 && t2 != QTime(0, 0, 0))) {
                    delete day;
                    return false;
                }
                length = qAbs(length);
                day->addInterval(t1, length);
            }
            CalendarModifyDayCmd *cmd = new CalendarModifyDayCmd(cal, day, kundo2_i18n("Modify calendar date"));
            Q_EMIT executeCommand(cmd);
            return true;
        }
    }
    return false;
}

QVariant CalendarExtendedItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    int col = section - CalendarItemModel::columnCount();
    if (col < 0) {
        return CalendarItemModel::headerData(section, orientation, role);
    }
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (col) {
                case 0: return xi18nc("@title:column", "Weekday");
                case 1: return xi18nc("@title:column", "Date");
                default: return QVariant();
            }
        } else if (role == Qt::TextAlignmentRole) {
            switch (col) {
                default: return QVariant();
            }
        }
    }
    if (role == Qt::ToolTipRole) {
        switch (section) {
            default: return QVariant();
        }
    }
    return QVariant();
}

int CalendarExtendedItemModel::columnNumber(const QString& name) const
{
    QStringList lst;
    lst << QStringLiteral("Weekday")
        << QStringLiteral("Date");
    if (lst.contains(name)) {
        return lst.indexOf(name) + CalendarItemModel::columnCount();
    }
    return CalendarItemModel::columnMap().keyToValue(name.toUtf8().constData());
}

} // namespace KPlato
