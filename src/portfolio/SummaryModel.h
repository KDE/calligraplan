/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLANPORTFOLIO_SUMMARYMODEL_H
#define PLANPORTFOLIO_SUMMARYMODEL_H

#include "kptnodeitemmodel.h"

#include <KExtraColumnsProxyModel>

#include <QSortFilterProxyModel>
#include <QAbstractItemModel>
#include <QString>

class MainDocument;
class SummaryFilterModel;
class SummaryBaseModel;
class ProjectsModel;

class KoDocument;

namespace KPlato {
    class Project;
    class ChartItemModel;
}

class SummaryModel : public KExtraColumnsProxyModel
{
    Q_OBJECT
    Q_PROPERTY(MainDocument* portfolio READ portfolio WRITE setPortfolio NOTIFY portfolioChanged)

public:
    explicit SummaryModel(QObject *parent = nullptr);
    ~SummaryModel();

    MainDocument *portfolio() const;

public Q_SLOTS:
    void setPortfolio(MainDocument *portfolio);

Q_SIGNALS:
    void portfolioChanged();

protected Q_SLOTS:
    void slotModelReset();
    void slotUpdateChartModel();

protected:
    QVariant extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role = Qt::DisplayRole) const override;

private:
    SummaryFilterModel *m_baseModel;
    QHash<KoDocument*, KPlato::ChartItemModel*> m_performanceModels;
};

class SummaryFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(MainDocument* portfolio READ portfolio WRITE setPortfolio NOTIFY portfolioChanged)

public:
    explicit SummaryFilterModel(QObject *parent = nullptr);
    ~SummaryFilterModel();

    MainDocument *portfolio() const;

public Q_SLOTS:
    void setPortfolio(MainDocument *portfolio);

Q_SIGNALS:
    void portfolioChanged();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;

private:
    ProjectsModel *m_baseModel;
};

#endif
