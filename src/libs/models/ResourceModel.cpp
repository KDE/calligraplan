/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ResourceModel.h"

#include "kptlocale.h"
#include "kptcommonstrings.h"
#include <AddResourceCmd.h>
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdatetime.h"
#include "kptdebug.h"

#include <KoIcon.h>

#include <QMimeData>
#include <QMimeDatabase>
#include <QStringList>
#include <QLocale>

#include <KIO/TransferJob>
#include <KIO/StatJob>

#ifdef PLAN_KCONTACTS_FOUND
#include <KContacts/Addressee>
#include <KContacts/VCardConverter>
#endif


using namespace KPlato;


ResourceModel::ResourceModel(QObject *parent)
    : QObject(parent),
    m_project(nullptr)
{
}

ResourceModel::~ResourceModel()
{
}

const QMetaEnum ResourceModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

void ResourceModel::setProject(Project *project)
{
    m_project = project;
}

int ResourceModel::propertyCount() const
{
    return columnMap().keyCount();
}

QVariant ResourceModel::name(const Resource *res, int role) const
{
    //debugPlan<<res->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return res->name();
        case Qt::ToolTipRole: {
            return xi18nc("@info:tooltip", "%1:<nl/>Type: %2<nl/>Allocation: %3<nl/>Origin: %4",
                          res->name(),
                          res->typeToString(true),
                          res->autoAllocate() ? i18n("On task creation") : i18n("Manual"),
                          res->isShared() ? i18n("Shared") : i18n("Local")
                          );
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::DecorationRole:
            if (res->isBaselined()) {
                return koIcon("view-time-schedule-baselined");
             }
             break;
        case Qt::CheckStateRole:
            return res->autoAllocate() ? Qt::Checked : Qt::Unchecked;
        default:
            break;
    }
    return QVariant();
}

QVariant ResourceModel::origin(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return res->isShared() ? i18n("Shared") : i18n("Local");
        case Qt::ToolTipRole:
            if (!res->isShared()) {
                return xi18nc("@info:tooltip", "%1 is a <emphasis>Local</emphasis> resource and can only be used in this project", res->name());
            }
            return xi18nc("@info:tooltip", "%1 is a <emphasis>Shared</emphasis> resource and can thus be shared with other projects", res->name());
        case Qt::EditRole:
            return res->isShared() ? QStringLiteral("Shared") : QStringLiteral("Local");
        case Role::EnumList:
            return QStringList() << i18n("Local") << i18n("Shared");
        case Role::EnumListValue:
            return res->isShared() ? 1 : 0;
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::type(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return res->typeToString(true);
        case Qt::ToolTipRole: {
            if (res->type() == Resource::Type_Team) {
                QStringList resourceNames;
                const auto team = res->teamMembers();
                for (const auto r : team) {
                    resourceNames << r->name();
                }
                if (resourceNames.isEmpty()) {
                    return i18nc("@info:tooltip", "Team: No team members");
                }
                return i18nc("@info:tooltip", "Team: %1", resourceNames.join(QStringLiteral(", ")));
            }
            const auto required = res->requiredResources();
            if (!required.isEmpty()) {
                QStringList resourceNames;
                for (const auto r : required) {
                    resourceNames << r->name();
                }
                return i18nc("@info:tooltip", "Required: %1", resourceNames.join(QStringLiteral(", ")));
            }
            return res->typeToString(true);
        }
        case Qt::EditRole:
            return res->typeToString(false);
        case Role::EnumList:
            return res->typeToStringList(true);
        case Role::EnumListValue:
            return (int)res->type();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::initials(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->initials();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::email(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->email();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::calendar(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            if (res->type() == Resource::Type_Team) {
                return QStringLiteral(" ");
            }
            QString s = i18n("None");
            Calendar *cal = res->calendar(true); // don't check for default calendar
            if (cal) {
                s = cal->name();
            } else if (res->type() == Resource::Type_Work) {
                // Do we get a default calendar
                cal = res->calendar();
                if (cal) {
                    s = i18nc("Default (calendar name)", "Default (%1)", cal->name());
                }
            }
            return s;
        }
        case Qt::ToolTipRole: {
            if (res->type() == Resource::Type_Team) {
                return xi18nc("@info:tooltip", "A team resource does not have a calendar");
            }
            QString s = xi18nc("@info:tooltip", "No calendar");
            Calendar *cal = res->calendar(true); // don't check for default calendar
            if (cal) {
                s = cal->name();
            } else if (res->type() == Resource::Type_Work) {
                // Do we get a default calendar
                cal = res->calendar();
                if (cal) {
                    s = xi18nc("@info:tooltip 1=calendar name", "Using default calendar: %1", cal->name());
                }
            }
            return s;
        }
        case Role::EnumList: {
            Calendar *cal = m_project->defaultCalendar();
            QString s = i18n("None");
            if (cal &&  res->type() == Resource::Type_Work) {
                s = i18nc("Default (calendar name)", "Default (%1)", cal->name());
            }
            return QStringList() << s << m_project->calendarNames();
        }
        case Qt::EditRole:
        case Role::EnumListValue: {
            Calendar *cal = res->calendar(true); // don't check for default calendar
            return cal == nullptr ? 0 : m_project->calendarNames().indexOf(cal->name()) + 1;
        }
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::units(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return res->units();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::availableFrom(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(res->availableFrom(), QLocale::ShortFormat);
        case Qt::EditRole:
            return res->availableFrom();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole: {
            if (res->availableFrom().isValid()) {
                return xi18nc("infor:tooltip", "Available from: %1", QLocale().toString(res->availableFrom(), QLocale::LongFormat));
            }
            return xi18nc("infor:tooltip", "Available from project target start time: %1", QLocale().toString(m_project->constraintStartTime(), QLocale::LongFormat));
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::availableUntil(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(res->availableUntil(), QLocale::ShortFormat);
        case Qt::EditRole:
            return res->availableUntil();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole: {
            if (res->availableUntil().isValid()) {
                return xi18nc("infor:tooltip", "Available until: %1", QLocale().toString(res->availableUntil(), QLocale::LongFormat));
            }
            return xi18nc("infor:tooltip", "Available until project target finish time: %1", QLocale().toString(m_project->constraintEndTime(), QLocale::LongFormat));
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::normalRate(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return m_project->locale()->formatMoney(res->normalRate());
        case Qt::EditRole:
            return res->normalRate();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole:
            return i18n("Cost per hour, normal time: %1", m_project->locale()->formatMoney(res->normalRate()));
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::overtimeRate(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return m_project->locale()->formatMoney(res->overtimeRate());
        case Qt::EditRole:
            return res->overtimeRate();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole:
            return i18n("Cost per hour, overtime: %1", m_project->locale()->formatMoney(res->overtimeRate()));
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::account(const Resource *resource, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            Account *a = resource->account();
            return a == nullptr ? i18n("None") : a->name();
        }
        case Qt::ToolTipRole: {
            Account *a = resource->account();
            return i18n("Account: %1", (a == nullptr ? i18n("None") : a->name()));
        }
        case Role::EnumListValue:
        case Qt::EditRole: {
            Account *a = resource->account();
            return a == nullptr ? 0 : (m_project->accounts().costElements().indexOf(a->name()) + 1);
        }
        case Role::EnumList: {
            QStringList lst;
            lst << i18n("None");
            lst += m_project->accounts().costElements();
            return lst;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceModel::data(const Resource *resource, int property, int role) const
{
    if (role == Role::ObjectType) {
        return OT_Resource;
    }
    QVariant result;
    if (resource == nullptr) {
        return result;
    }
    switch (property) {
        case ResourceName: result = name(resource, role); break;
        case ResourceOrigin: result = origin(resource, role); break;
        case ResourceType: result = type(resource, role); break;
        case ResourceInitials: result = initials(resource, role); break;
        case ResourceEmail: result = email(resource, role); break;
        case ResourceCalendar: result = calendar(resource, role); break;
        case ResourceLimit: result = units(resource, role); break;
        case ResourceAvailableFrom: result = availableFrom(resource, role); break;
        case ResourceAvailableUntil: result = availableUntil(resource, role); break;
        case ResourceNormalRate: result = normalRate(resource, role); break;
        case ResourceOvertimeRate: result = overtimeRate(resource, role); break;
        case ResourceAccount: result = account(resource, role); break;
        default:
            debugPlan<<"data: invalid display value: property="<<property;
            break;
    }
    return result;
}

QVariant ResourceModel::headerData(int section, int role)
{
    if (role == Qt::DisplayRole) {
        switch (section) {
            case ResourceName: return i18n("Name");
            case ResourceOrigin: return i18n("Origin");
            case ResourceType: return i18n("Type");
            case ResourceInitials: return i18n("Initials");
            case ResourceEmail: return i18n("Email");
            case ResourceCalendar: return i18n("Calendar");
            case ResourceLimit: return i18n("Limit (%)");
            case ResourceAvailableFrom: return i18n("Available From");
            case ResourceAvailableUntil: return i18n("Available Until");
            case ResourceNormalRate: return i18n("Normal Rate");
            case ResourceOvertimeRate: return i18n("Overtime Rate");
            case ResourceAccount: return i18n("Account");
            default: return QVariant();
        }
    } else if (role == Qt::TextAlignmentRole) {
        switch (section) {
            case ResourceName:
            case ResourceOrigin:
            case ResourceType:
            case ResourceInitials:
            case ResourceEmail:
            case ResourceCalendar:
                return QVariant();
            case ResourceLimit:
                return (int)(Qt::AlignRight|Qt::AlignVCenter);
            case ResourceAvailableFrom:
            case ResourceAvailableUntil:
                return QVariant();
            case ResourceNormalRate:
            case ResourceOvertimeRate:
                return (int)(Qt::AlignRight|Qt::AlignVCenter);
            case ResourceAccount:
                return QVariant();
            default:
                return QVariant();
        }
    } else if (role == Qt::ToolTipRole) {
        switch (section) {
            case ResourceName: return ToolTip::resourceName();
            case ResourceOrigin: return ToolTip::resourceOrigin();
            case ResourceType: return ToolTip::resourceType();
            case ResourceInitials: return ToolTip::resourceInitials();
            case ResourceEmail: return ToolTip::resourceEMail();
            case ResourceCalendar: return ToolTip::resourceCalendar();
            case ResourceLimit: return ToolTip::resourceUnits();
            case ResourceAvailableFrom: return ToolTip::resourceAvailableFrom();
            case ResourceAvailableUntil: return ToolTip::resourceAvailableUntil();
            case ResourceNormalRate: return ToolTip::resourceNormalRate();
            case ResourceOvertimeRate: return ToolTip::resourceOvertimeRate();
            case ResourceAccount: return ToolTip::resourceAccount();
            default: return QVariant();
        }
    }
    return QVariant();
}
