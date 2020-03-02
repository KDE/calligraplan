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

#ifndef RESOURCEGROUPMODEL_H
#define RESOURCEGROUPMODEL_H

#include "planmodels_export.h"

#include <QSortFilterProxyModel>
#include <QMetaEnum>

class QByteArray;

namespace KIO {
    class Job;
}
class KJob;

namespace KPlato
{

class Project;
class ResourceGroup;

class PLANMODELS_EXPORT ResourceGroupModel : public QObject
{
    Q_OBJECT
public:
    explicit ResourceGroupModel(QObject *parent = nullptr);
    ~ResourceGroupModel() override;

    enum Properties {
        Name = 0,
        Origin,
        Type,
        Units,
        Coordinator
    };
    Q_ENUM(Properties)

    const QMetaEnum columnMap() const;
    void setProject(Project *project);
    int propertyCount() const;
    QVariant data(const ResourceGroup *group, int property, int role = Qt::DisplayRole) const;
    static QVariant headerData(int section, int role = Qt::DisplayRole);

    QVariant name(const ResourceGroup *group, int role) const;
    QVariant origin(const ResourceGroup *group, int role) const;
    QVariant type(const ResourceGroup *group, int role) const;
    QVariant units(const ResourceGroup *group, int role) const;
    QVariant coordinator(const ResourceGroup *group, int role) const;

private:
    Project *m_project;
};

}  //KPlato namespace

#endif
