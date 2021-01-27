/* This file is part of the KDE project
* Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
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

#ifndef PLANPORTFOLIO_PORTFOLIOMODEL_H
#define PLANPORTFOLIO_PORTFOLIOMODEL_H

#include <KExtraColumnsProxyModel>
#include <QString>

class MainDocument;
class KoDocument;

class ProjectsFilterModel;

class PortfolioModel : public KExtraColumnsProxyModel
{
    Q_OBJECT
public:
    explicit PortfolioModel(QObject *parent = nullptr);
    ~PortfolioModel() override;

    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role = Qt::EditRole) override;
    QVariant extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role) const override;

    void setPortfolio(MainDocument *doc);
    MainDocument *portfolio() const;

    KoDocument *documentFromIndex(const QModelIndex &idx) const;

private:
    ProjectsFilterModel *m_baseModel;
};

#endif
