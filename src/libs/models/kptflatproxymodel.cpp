/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2010, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptflatproxymodel.h"

#include "kptglobal.h"

#include <KLocalizedString>

#include <QModelIndex>

#include "kptdebug.h"

using namespace KPlato;


FlatProxyModel::FlatProxyModel(QObject *parent)
    : KDescendantsProxyModel(parent)
{
}

int FlatProxyModel::columnCount(const QModelIndex &parent) const
{
    int count = KDescendantsProxyModel::columnCount(parent);
    return count ? count + 1 : 0;
}

QVariant FlatProxyModel::data(const QModelIndex &index, int role) const
{
    if (index.column() == columnCount() - 1) {
        // get parent name
        QModelIndex source_index = mapToSource(index.sibling(index.row(), 0)).parent();
        return source_index.data(role);
    }
    return KDescendantsProxyModel::data(index, role);
}

QVariant FlatProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == columnCount() - 1) {
        if (orientation == Qt::Vertical) {
            return QVariant();
        }
        return role == Role::ColumnTag ? QStringLiteral("Parent") : i18n("Parent");
    }
    return KDescendantsProxyModel::headerData(section, orientation, role);
}

QMimeData *FlatProxyModel::mimeData(const QModelIndexList &indexes) const
{
    if (sourceModel() == nullptr) {
        return nullptr;
    }
    QModelIndexList source_indexes;
    for (int i = 0; i < indexes.count(); ++i) {
        source_indexes << mapToSource(indexes.at(i));
    }
    return sourceModel()->mimeData(source_indexes);
}

QStringList FlatProxyModel::mimeTypes() const
{
    if (sourceModel() == nullptr) {
        return QStringList();
    }
    return sourceModel()->mimeTypes();
}

Qt::DropActions FlatProxyModel::supportedDropActions() const
{
    if (sourceModel() == nullptr) {
        return {};
    }
    return sourceModel()->supportedDropActions();
}

bool FlatProxyModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                         int row, int column, const QModelIndex &parent)
{
    if (sourceModel() == nullptr) {
        return false;
    }
    if ((row == -1) && (column == -1))
        return sourceModel()->dropMimeData(data, action, -1, -1, mapToSource(parent));
    int source_destination_row = -1;
    int source_destination_column = -1;
    QModelIndex source_parent;
    if (row == rowCount(parent)) {
        source_parent = mapToSource(parent);
        source_destination_row = sourceModel()->rowCount(source_parent);
    } else {
        QModelIndex proxy_index = index(row, column, parent);
        QModelIndex source_index = mapToSource(proxy_index);
        source_destination_row = source_index.row();
        source_destination_column = source_index.column();
        source_parent = source_index.parent();
    }
    return sourceModel()->dropMimeData(data, action, source_destination_row,
                                  source_destination_column, source_parent);
}

QModelIndex FlatProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (! proxyIndex.isValid() || proxyIndex.column() == columnCount() - 1) {
        return QModelIndex();
    }
    return KDescendantsProxyModel::mapToSource(proxyIndex);
}
