/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ResourceModel.h"
#include "MainDocument.h"

#include <kptproject.h>
#include <libs/models/ResourceModel.h>

#include <KoIcon.h>

ResourceModel::ResourceModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_portfolio(nullptr)
{
}

ResourceModel::~ResourceModel()
{
}

int ResourceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_resourceIds.count();
}

int ResourceModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}


QVariant ResourceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    if (orientation == Qt::Horizontal) {
        return KPlato::ResourceModel::headerData(section, role);
    }
    return QVariant();
}

QVariant ResourceModel::data(const QModelIndex &idx, int role) const
{
    if (role == RESOURCEID_ROLE) {
        const auto ids = m_resourceIds.keys();
        return ids.value(idx.row());
    }
    switch (role) {
        case Qt::DisplayRole: {
            const auto names = m_resourceIds.values();
            return names.value(idx.row());
        }
        default: break;
    }
    return QVariant();
}

MainDocument *ResourceModel::portfolio() const
{
    return m_portfolio;
}

void ResourceModel::setPortfolio(MainDocument *portfolio)
{
    beginResetModel();
    if (m_portfolio) {
        disconnect(m_portfolio, &MainDocument::documentAboutToBeInserted, this, &ResourceModel::documentAboutToBeInserted);
        disconnect(m_portfolio, &MainDocument::documentInserted, this, &ResourceModel::documentInserted);
        disconnect(m_portfolio, &MainDocument::documentAboutToBeRemoved, this, &ResourceModel::documentAboutToBeRemoved);
        disconnect(m_portfolio, &MainDocument::documentRemoved, this, &ResourceModel::documentRemoved);

        disconnect(m_portfolio, &MainDocument::documentChanged, this, &ResourceModel::documentChanged);
        disconnect(m_portfolio, &MainDocument::projectChanged, this, &ResourceModel::projectChanged);
    }
    m_portfolio = portfolio;
    updateData();
    if (m_portfolio) {
        connect(m_portfolio, &MainDocument::documentAboutToBeInserted, this, &ResourceModel::documentAboutToBeInserted);
        connect(m_portfolio, &MainDocument::documentInserted, this, &ResourceModel::documentInserted);
        connect(m_portfolio, &MainDocument::documentAboutToBeRemoved, this, &ResourceModel::documentAboutToBeRemoved);
        connect(m_portfolio, &MainDocument::documentRemoved, this, &ResourceModel::documentRemoved);

        connect(m_portfolio, &MainDocument::documentChanged, this, &ResourceModel::documentChanged);
        connect(m_portfolio, &MainDocument::projectChanged, this, &ResourceModel::projectChanged);
    }
    endResetModel();
    Q_EMIT portfolioChanged();
}

void ResourceModel::reset()
{
    beginResetModel();
    updateData();
    endResetModel();
    Q_EMIT portfolioChanged();
}

void ResourceModel::documentAboutToBeInserted(int row)
{
    beginInsertRows(QModelIndex(), row, row);
}

void ResourceModel::documentInserted()
{
    endInsertRows();
}

void ResourceModel::documentAboutToBeRemoved(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
}

void ResourceModel::documentRemoved()
{
    endRemoveRows();
}

void ResourceModel::documentChanged(KoDocument *doc, int row)
{
    Q_UNUSED(doc);
    const QModelIndex idx = this->index(row, 0);
    Q_EMIT dataChanged(idx, idx.siblingAtColumn(columnCount()-1));
}

void ResourceModel::projectChanged(KoDocument *doc)
{
    documentChanged(doc, m_portfolio->documents().indexOf(doc));
}

void ResourceModel::updateData()
{
    m_resourceIds.clear();
    if (!m_portfolio) {
        return;
    }
    const auto docs = m_portfolio->documents();
    for (const auto doc : docs) {
        const auto project = doc->project();
//         auto sm = project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
//         if (!sm) {
//             continue;
//         }
//         const auto sid = sm->expected()->id();
        const auto rlist = project->resourceList();
        for (const auto r : rlist) {
            if (!m_resourceIds.contains(r->id())) {
                m_resourceIds.insert(r->id(), r->name());
            }
        }
    }
}
