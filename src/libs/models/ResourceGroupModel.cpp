/* This file is part of the KDE project
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

QVariant ResourceGroupModel::scope(const ResourceGroup *group, int role) const
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
            return group->isShared() ? "Shared" : "Local";
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
        case Qt::EditRole:
        case Qt::ToolTipRole:
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
    if (group == 0) {
        return result;
    }
    switch (property) {
        case Name: result = name(group, role); break;
        case Scope: result = scope(group, role); break;
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
            case Scope: return i18n("Origin");
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
            case Name: return ToolTip::resourceName();
            case Scope: return ToolTip::resourceScope();
            case Type: return ToolTip::resourceType();
            case Units: return ToolTip::resourceUnits();
            case Coordinator: return i18n("TODO");
            default: return QVariant();
        }
    }
    return QVariant();
}

