/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "PortfolioModel.h"
#include "ProjectsModel.h"
#include "MainDocument.h"

#include <kptproject.h>

#include <KoDocument.h>

#include <QDebug>

PortfolioModel::PortfolioModel(QObject *parent)
    : KExtraColumnsProxyModel(parent)
{
    appendColumn(xi18nc("@title:column", "Portfolio"));
    appendColumn(xi18nc("@title:column", "File"));

    m_baseModel = new ProjectsFilterModel(this);
    // Note: changes might affect methods below
    const QList<int> columns = QList<int>() << KPlato::NodeModel::NodeName;
    m_baseModel->setAcceptedColumns(columns);
    setSourceModel(m_baseModel);
}

PortfolioModel::~PortfolioModel()
{
}


Qt::ItemFlags PortfolioModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags flg = QAbstractItemModel::flags(idx);
    KoDocument *doc = documentFromIndex(idx);
    if (doc) {
        int extraColumn = idx.column() - sourceModel()->columnCount();
        switch (extraColumn) {
            case 0: {
                flg |= Qt::ItemIsEditable;
                break;
            }
            default: {
                break;
            }
        }
    }
    return flg;
}

QVariant PortfolioModel::extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role) const
{
    Q_UNUSED(parent)
    KoDocument *doc = documentFromIndex(index(row, 0, parent));
    if (!doc) {
        return QVariant();
    }
    switch (extraColumn) {
        case 0: {
            switch (role) {
                case Qt::DisplayRole:
                    return doc->property(ISPORTFOLIO).toBool() ? i18n("Yes") : i18n("No");
                case Qt::EditRole:
                    return doc->property(ISPORTFOLIO).toBool();// ? i18n("Yes") : i18n("No");
                default: break;
            }
            break;
        }
        case 1: {
            switch (role) {
                case Qt::DisplayRole:
                case Qt::EditRole: {
                    return doc->url().toDisplayString();
                }
                default: break;
            }
            break;
        }
        default: break;
    }
    return QVariant();
}

bool PortfolioModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    KoDocument *doc = documentFromIndex(idx);
    if (!doc) {
        return false;
    }
    if (role == Qt::EditRole) {
        int extraColumn = idx.column() - sourceModel()->columnCount();
        switch (extraColumn) {
            case 0: {
                doc->setProperty(ISPORTFOLIO, value);
                Q_EMIT dataChanged(idx, idx);
                portfolio()->setModified(true);
                return true;
            }
            default: {
                break;
            }
        }
    }
    return false;
}

void PortfolioModel::setPortfolio(MainDocument *doc)
{
    m_baseModel->setPortfolio(doc);
}

MainDocument *PortfolioModel::portfolio() const
{
    return m_baseModel->portfolio();
}

KoDocument *PortfolioModel::documentFromIndex(const QModelIndex &idx) const
{
    return m_baseModel->documentFromIndex(mapToSource(idx));
}
