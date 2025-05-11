/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ProjectsDashboard.h"
#include "MainDocument.h"
#include "GanttView.h"
#include "GanttModel.h"
#include "SummaryView.h"

#include <kptnodechartmodel.h>
#include <kptnodeitemmodel.h>
#include <kptproject.h>

#include <KoApplication.h>
#include <KoComponentData.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>
#include <KoFileDialog.h>

#include <KRecentFilesAction>
#include <KActionCollection>

#include <QTreeView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QSplitter>

#include <KChartChart>
#include <KChartLineDiagram>
#include <KChartLegend>

ProjectsDashboard::ProjectsDashboard(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    if (doc && doc->isReadWrite()) {
        setXMLFile("Portfolio_ProjectsDashboardUi.rc");
    }
    setupGui();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto sp1 = new QSplitter(Qt::Vertical, this);
    layout->addWidget(sp1);
    auto sp2 = new QSplitter(Qt::Horizontal, this);
    sp1->addWidget(sp2);
    auto sp3 = new QSplitter(Qt::Horizontal, this);
    sp1->addWidget(sp3);

    auto *gv = new GanttView(part, doc, this);
    const auto sections = QList<int>() << GanttModel::Type << GanttModel::StartTime << GanttModel::EndTime;
    gv->hideSections(sections);
    sp2->addWidget(gv);
    sp3->addWidget(new SummaryView(part, doc, this));

    auto chart = new KChart::Chart(this);
    KChart::LineDiagram *dia = new KChart::LineDiagram(chart);
    auto chartModel = new ProjectsPerformanceModel(this);
    chartModel->setPortfolio(doc);
    dia->setModel(chartModel);
    KChart::CartesianAxis* xAxis = new KChart::CartesianAxis(dia);
    KChart::CartesianAxis* yAxis = new KChart::CartesianAxis(dia);
    xAxis->setPosition(KChart::CartesianAxis::Bottom);
    yAxis->setPosition(KChart::CartesianAxis::Left);
    dia->addAxis(xAxis);
    dia->addAxis(yAxis);

    chart->coordinatePlane()->replaceDiagram(dia);
    KChart::Legend *legend = new KChart::Legend(dia);
    legend->setPosition(KChart::Position::East);
    legend->setOrientation(Qt::Vertical);
    legend->setTitleText(i18n("Projects"));
//     auto attr = legend->textAttributes();
//     auto size = attr.minimalFontSize();
//     size.setValue(10.0);
//     attr.setMinimalFontSize(size);
//     legend->setTextAttributes(attr);
    chart->addLegend(legend);

    sp3->addWidget(chart);

    sp1->setSizes(QList<int>()<<1<<1);
    sp2->setSizes(QList<int>()<<1<<1);
    sp3->setSizes(QList<int>()<<1<<1);
}

ProjectsDashboard::~ProjectsDashboard()
{
}

void ProjectsDashboard::setupGui()
{
}

void ProjectsDashboard::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * ProjectsDashboard::popupMenu(const QString& name)
{
    return nullptr;
}

KoPrintJob *ProjectsDashboard::createPrintJob()
{
    return nullptr;
}

//---------------------------

ProjectsPerformanceModel::ProjectsPerformanceModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_portfolio(nullptr)
    , m_valueToRetreive(KPlato::ChartItemModel::SPIEffort)
{
}

ProjectsPerformanceModel::~ProjectsPerformanceModel()
{
    qDeleteAll(m_chartModels);
}

void ProjectsPerformanceModel::setPortfolio(KoDocument* doc)
{
    beginResetModel();
    if (m_portfolio) {
        disconnect(m_portfolio, &MainDocument::changed, this, &ProjectsPerformanceModel::slotChanged);
    }
    qDeleteAll(m_chartModels);
    m_chartModels.clear();
    m_portfolio = qobject_cast<MainDocument*>(doc);
    if (!m_portfolio) {
        qInfo()<<Q_FUNC_INFO<<doc<<"not portfolio";
        return;
    }
    const auto docs = m_portfolio->documents();
    for (auto doc : docs) {
        auto m = new KPlato::ChartItemModel();
        m->setProject(doc->project());
        m->setScheduleManager(doc->project()->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString()));
        m->addNode(doc->project());
        m_chartModels.insert(doc, m);
        const auto startdate = m->startDate();
        if (startdate.isValid()) {
            if (!m_startDate.isValid() || startdate < m_startDate) {
                m_startDate = startdate;
            }
        }
        const auto enddate = m->endDate();
        if (m_endDate < enddate) {
            m_endDate = enddate;
        }
        qInfo()<<Q_FUNC_INFO<<doc->project()<<startdate<<enddate<<':'<<m_startDate<<m_endDate;
    }
    if (m_portfolio) {
        connect(m_portfolio, &MainDocument::changed, this, &ProjectsPerformanceModel::slotChanged);
    }
    endResetModel();
}

void ProjectsPerformanceModel::setValueToRetreive(int value)
{
    m_valueToRetreive = value;
}

void ProjectsPerformanceModel::slotChanged()
{
    setPortfolio(m_portfolio);
}


int ProjectsPerformanceModel::rowCount(const QModelIndex &idx) const
{
    Q_ASSERT(!idx.isValid());
    if (!m_startDate.isValid() || !m_endDate.isValid()) {
        return 0;
    }
    return m_startDate.daysTo(m_endDate) + 1;
}

int ProjectsPerformanceModel::columnCount(const QModelIndex &idx) const
{
    if (idx.isValid() || !m_portfolio) {
        qInfo()<<Q_FUNC_INFO<<idx<<m_portfolio;
        return 0;
    }
    return m_chartModels.count();
}

QVariant ProjectsPerformanceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        switch (role) {
            case Qt::DisplayRole:
                return m_startDate.addDays(section);
        default:
            return QVariant();
        }
    } else {
        const auto doc = m_portfolio->documents().value(section);
        if (!doc) {
            return QVariant();
        }
        const auto project = doc->project();
        switch (role) {
            case Qt::DisplayRole:
                return project->name();
            default:
                break;
        }
    }
    return QVariant();
}

QVariant ProjectsPerformanceModel::data(const QModelIndex &idx, int role) const
{
    const auto chartModel = m_chartModels.value(m_portfolio->documents().value(idx.column()));
    if (!chartModel) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
            QDate date = m_startDate.addDays(idx.row());
            const auto startdate = chartModel->startDate();
            const auto enddate = chartModel->endDate();
            if (startdate > date || enddate < date) {
                return QVariant();
            }
            int row = startdate.daysTo(date);
            return chartModel->index(row, m_valueToRetreive).data(role);
        }
        default: break;
    }
    return QVariant();
}
