/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
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
