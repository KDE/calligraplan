/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "SummaryModel.h"
#include "ProjectsModel.h"
#include "MainDocument.h"
#include "kptproject.h"
#include "kptnodechartmodel.h"

#include <QTimer>
#include <QDebug>


SummaryModel::SummaryModel(QObject *parent)
    : KExtraColumnsProxyModel(parent)
{
    KPlato::ChartItemModel m;
    for (int c = 0; c < m.columnCount(); ++c) {
        appendColumn(m.headerData(c, Qt::Horizontal).toString());
    }

    m_baseModel = new SummaryFilterModel(this);
    setSourceModel(m_baseModel);
    connect(m_baseModel, &SummaryFilterModel::modelReset, this, &SummaryModel::slotModelReset);
    connect(m_baseModel, &SummaryFilterModel::dataChanged, this, &SummaryModel::slotModelReset);
}

SummaryModel::~SummaryModel()
{
}

MainDocument *SummaryModel::portfolio() const
{
    return m_baseModel->portfolio();
}

void SummaryModel::setPortfolio(MainDocument *portfolio)
{
    m_baseModel->setPortfolio(portfolio);
}

void SummaryModel::slotModelReset()
{
    QTimer::singleShot(0, this, &SummaryModel::slotUpdateChartModel);
}

void SummaryModel::slotUpdateChartModel()
{
    Q_ASSERT(portfolio());
    beginResetModel();
    const auto documents = portfolio()->documents();
    for (KoDocument *doc : documents) {
        KPlato::Project *project = doc->project();
        KPlato::ChartItemModel *m = m_performanceModels.value(doc);
        if (!m) {
            m = new KPlato::ChartItemModel(this);
            m->setLocalizeValues(true);
            m_performanceModels.insert(doc, m);
        }
        m->setProject(project);
        if (project) {
            KPlato::ScheduleManager *sm = project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
            m->setScheduleManager(sm);
            m->setNodes(QList<KPlato::Node*>() << project);
        }
    }
    endResetModel();
}

QVariant SummaryModel::extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role) const
{
    if (parent.isValid()) {
        return QVariant();
    }
    KoDocument *doc = portfolio()->documents().at(row);
    if (!m_performanceModels.contains(doc)) {
        return QVariant();
    }
    const KPlato::ChartItemModel *m = m_performanceModels[doc];
    int r = std::min((int)(m->startDate().daysTo(QDate::currentDate()))+1, m->rowCount()-1);
    const QModelIndex idx = m->index(r, extraColumn, parent);
    QVariant v = m->data(idx, role);
    return v;
}

//-------------------
SummaryFilterModel::SummaryFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_baseModel = new ProjectsModel(this);
    setSourceModel(m_baseModel);
}

SummaryFilterModel::~SummaryFilterModel()
{
}

MainDocument *SummaryFilterModel::portfolio() const
{
    return m_baseModel->portfolio();
}

void SummaryFilterModel::setPortfolio(MainDocument *portfolio)
{
    m_baseModel->setPortfolio(portfolio);
}

bool SummaryFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    MainDocument *doc = portfolio();
    return !source_parent.isValid() && doc && doc->documents().at(source_row)->property(ISPORTFOLIO).toBool();
}

bool SummaryFilterModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)
    static QList<int> cols = QList<int>()
    << KPlato::NodeModel::NodeName
    << KPlato::NodeModel::NodeDescription
    << KPlato::NodeModel().propertyCount();
    return cols.contains(source_column);
}
