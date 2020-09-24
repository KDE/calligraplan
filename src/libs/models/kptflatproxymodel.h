/* This file is part of the KDE project
  Copyright (C) 2010 Dag Andersen <dag.andersen@kdemail.net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef KPTFLATPROXYMODEL_H
#define KPTFLATPROXYMODEL_H

#include "planmodels_export.h"

#include <KDescendantsProxyModel>

/// The main namespace
namespace KPlato
{

/**
    FlatProxyModel is a proxy model that makes a tree source model flat.
    
    This might be useful to present data from a tree model in e.g. a table view or a report.
    
    Note that the source model should have the same number of columns for all parent indices,
    since a flat model obviously have the same number of columns for all indices.
    If this is not the case, the behavior is undefined.
    
    The row sequence of the flat model is the same as if the source model was fully expanded.

    The flat model adds a Parent column at the end of the source model columns,
    to make it possible to access the parent index's data at column 0.
*/
class PLANMODELS_EXPORT FlatProxyModel : public KDescendantsProxyModel
{
    Q_OBJECT
public:
    explicit FlatProxyModel(QObject *parent = nullptr);

    QModelIndex mapToSource (const QModelIndex & proxyIndex) const override;

    int columnCount(const QModelIndex &parent  = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    QStringList mimeTypes() const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent = QModelIndex()) override;
};

} //namespace KPlato


#endif
