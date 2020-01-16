/* This file is part of the KDE project
 * Copyright (C) 2020 Dag Andersen <calligra-devel@kde.org>
 * Copyright (C) 2019 Dag Andersen <calligra-devel@kde.org>
 * Copyright (C) 2006 - 2009 Dag Andersen <calligra-devel@kde.org>
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

#ifndef RESOURCEITEMSFMODEL_H
#define RESOURCEITEMSFMODEL_H

#include "planmodels_export.h"

#include <QSortFilterProxyModel>

class KUndo2Command;


/// The main namespace
namespace KPlato
{

class Project;
class Resource;

class PLANMODELS_EXPORT ResourceItemSFModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ResourceItemSFModel(QObject *parent = 0);
    
    void setProject(Project *project);
    Resource *resource(const QModelIndex &index) const;
    using QAbstractProxyModel::index;
    QModelIndex index(Resource *r) const;

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    void setFilteredResources(const QList<Resource*> &lst = QList<Resource*>());
    void addFilteredResource(Resource *r);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
    
    QList<Resource*> m_filteredResources;
    Project *m_project;
};

} // namespace KPlato

#endif
