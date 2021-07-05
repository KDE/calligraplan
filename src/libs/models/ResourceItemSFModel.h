/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <calligra-devel@kde.org>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <calligra-devel@kde.org>
 * SPDX-FileCopyrightText: 2006-2009 Dag Andersen <calligra-devel@kde.org>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    explicit ResourceItemSFModel(QObject *parent = nullptr);
    
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
