/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ViewModel.h"

ViewModel::ViewModel(QObject *parent)
    : KPageModel(parent)
{
}

ViewModel::~ViewModel()
{
}

int ViewModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_views.count();
}

int ViewModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant ViewModel::headerData(int section, Qt::Orientation o, int role) const
{
    if (o == Qt::Horizontal) {
        switch (section) {
            case 0:
                switch (role) {
                    case Qt::DisplayRole:
                    case Qt::EditRole:
                        return "Name";
                }
            case 1:
                switch (role) {
                    case Qt::DisplayRole:
                    case Qt::EditRole:
                        return "File";
                }
            default: break;
        }
        return QVariant();
    }
    return section;
}

QVariant ViewModel::data(const QModelIndex &idx, int role) const
{
    Q_ASSERT(idx.row() >= 0 && idx.row() < m_views.count())
    switch (role) {
        case KPageModel::HeaderRole: {
            return m_views.at(idx.row())->getProperty("title");
        }
        case KPageModel::HeaderVisibleRole: {
            return true;
        }
        case KPageModel::WidgetRole: {
            return m_views.at(idx.row());
        }
        default: break;
    }
    return QVariant();
}

QModelIndex ViewModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

QModelIndex ViewModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

void ViewModel::setWidgets(const QList<QWidget*> &views)
{
    beginResetModel();
    m_views = views;
    endResetModel();
}
