/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ResourceGroupModel.h"

#include "ResourceGroup.h"
#include "kptlocale.h"
#include "kptcommonstrings.h"
#include <AddResourceCmd.h>
#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
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

//--------------------------------------

ResourceGroupModel::ResourceGroupModel(QObject *parent)
    : QObject(parent)
    , m_project(nullptr)
{
}

ResourceGroupModel::~ResourceGroupModel()
{
}

const QMetaEnum ResourceGroupModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

void ResourceGroupModel::setProject(Project *project)
{
    m_project = project;
}

int ResourceGroupModel::propertyCount() const
{
    return columnMap().keyCount();
}

QVariant ResourceGroupModel::name(const  ResourceGroup *group, int role) const
{
    //debugPlan<<group->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return group->name();
        case Qt::ToolTipRole:
            if (!group->isShared()) {
                return group->name();
            }
            return xi18nc("@info:tooltip", "%1 is a <emphasis>Shared</emphasis> resource group and can thus be shared with other projects", group->name());
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceGroupModel::origin(const ResourceGroup *group, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return group->isShared() ? i18n("Shared") : i18n("Local");
        case Qt::ToolTipRole:
            if (!group->isShared()) {
                return xi18nc("@info:tooltip", "%1 is a <emphasis>Local</emphasis> resource group and can only be used in this project", group->name());
            }
            return xi18nc("@info:tooltip", "%1 is a <emphasis>Shared</emphasis> resource group and can thus be shared with other projects", group->name());
        case Qt::EditRole:
            return group->isShared() ? QStringLiteral("Shared") : QStringLiteral("Local");
        case Role::EnumList:
            return QStringList() << i18n("Local") << i18n("Shared");
        case Role::EnumListValue:
            return group->isShared() ? 1 : 0;
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceGroupModel::type(const ResourceGroup *group, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::EditRole:
            return group->type();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceGroupModel::units(const ResourceGroup *group, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return group->units();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceGroupModel::coordinator(const ResourceGroup *group, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return group->coordinator();
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return QVariant();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceGroupModel::data(const ResourceGroup *group, int property, int role) const
{
    if (role == Role::ObjectType) {
        return OT_ResourceGroup;
    }
    QVariant result;
    if (group == nullptr) {
        return result;
    }
    switch (property) {
        case Name: result = name(group, role); break;
        case Origin: result = origin(group, role); break;
        case Type: result = type(group, role); break;
        case Units: result = units(group, role); break;
        case Coordinator: result = coordinator(group, role); break;
        default:
            if (role == Qt::DisplayRole) {
                result = QString();
            }
            break;
    }
    return result;
}

QVariant ResourceGroupModel::headerData(int section, int role)
{
    if (role == Qt::DisplayRole) {
        switch (section) {
            case Name: return i18n("Name");
            case Origin: return i18n("Origin");
            case Type: return i18n("Type");
            case Units: return i18n("Units");
            case Coordinator: return i18n("Coordinator");
            default: return QVariant();
        }
    } else if (role == Qt::TextAlignmentRole) {
        switch (section) {
            case Units:
                return (int)(Qt::AlignRight|Qt::AlignVCenter);
            default:
                return QVariant();
        }
    } else if (role == Qt::ToolTipRole) {
        switch (section) {
            case Name: return ToolTip::resourceGroupName();
            case Origin: return ToolTip::resourceGroupOrigin();
            case Type: return ToolTip::resourceGroupType();
            case Units: return ToolTip::resourceGroupUnits();
            case Coordinator: return ToolTip::resourceGroupCoordinator();
            default: return QVariant();
        }
    }
    return QVariant();
}

