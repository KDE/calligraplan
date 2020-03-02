/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net>
 * Copyright (C) 2007 Dag Andersen <danders@get2net>
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

#ifndef RESOURCEMODEL_H
#define RESOURCEMODEL_H

#include "planmodels_export.h"

#include <QObject>
#include <QMetaEnum>

class QByteArray;

namespace KIO {
    class Job;
}
class KJob;

namespace KPlato
{

class Project;
class Resource;

class PLANMODELS_EXPORT ResourceModel : public QObject
{
    Q_OBJECT
public:
    explicit ResourceModel(QObject *parent = 0);
    ~ResourceModel() override;

    enum Properties {
        ResourceName = 0,
        ResourceOrigin,
        ResourceType,
        ResourceInitials,
        ResourceEmail,
        ResourceCalendar,
        ResourceLimit,
        ResourceAvailableFrom,
        ResourceAvailableUntil,
        ResourceNormalRate,
        ResourceOvertimeRate,
        ResourceAccount
    };
    Q_ENUM(Properties)
    
    const QMetaEnum columnMap() const;
    void setProject(Project *project);
    int propertyCount() const;
    QVariant data(const Resource *resource, int property, int role = Qt::DisplayRole) const;
    static QVariant headerData(int section, int role = Qt::DisplayRole);

    QVariant name(const Resource *res, int role) const;
    QVariant origin(const Resource *res, int role) const;
    QVariant type(const Resource *res, int role) const;
    QVariant initials(const Resource *res, int role) const;
    QVariant email(const Resource *res, int role) const;
    QVariant calendar(const Resource *res, int role) const;
    QVariant units(const Resource *res, int role) const;
    QVariant availableFrom(const Resource *res, int role) const;
    QVariant availableUntil(const Resource *res, int role) const;
    QVariant normalRate(const Resource *res, int role) const;
    QVariant overtimeRate(const Resource *res, int role) const;
    QVariant account(const Resource *res, int role) const;

private:
    Project *m_project;
};

}  //KPlato namespace

#endif
