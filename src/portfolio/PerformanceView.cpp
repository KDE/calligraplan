/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "PerformanceView.h"
#include "MainDocument.h"
#include "ProjectsModel.h"

#include <kptproject.h>
#include <kptlocale.h>
#include <kptnodechartmodel.h>

#include <KoComponentData.h>
#include <KoPart.h>

#include <KRecentFilesAction>

#include <QTreeView>
#include <QVBoxLayout>
#include <QItemSelectionModel>
#include <QSplitter>

#include <KChartChart>
#include <KChartAbstractCoordinatePlane>
#include <KChartLineDiagram>
#include <KChartCartesianAxis>
#include <KChartLegend>

PerformanceView::PerformanceView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    if (doc && doc->isReadWrite()) {
        setXMLFile(QStringLiteral("Portfolio_PerformanceViewUi.rc"));
    } else {
        setXMLFile(QStringLiteral("Portfolio_PerformanceViewUi_readonly.rc"));
    }
    ui.setupUi(this);

    ProjectsFilterModel *model = new ProjectsFilterModel(ui.treeView);
    model->setAcceptedColumns(QList<int>() << KPlato::NodeModel::NodeName);
    model->setPortfolio(qobject_cast<MainDocument*>(doc));
    ui.treeView->setModel(model);

    setupCharts();

    connect(ui.treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PerformanceView::selectionChanged);
}

PerformanceView::~PerformanceView()
{
}

void PerformanceView::setupCharts()
{
    // setup models
    m_chartModel = new KPlato::ChartItemModel(ui.splitter);

    const QList<int> effortWork = QList<int>() << KPlato::ChartItemModel::BCWSEffort << KPlato::ChartItemModel::BCWPEffort << KPlato::ChartItemModel::ACWPEffort;
    const QList<int> effortPI = QList<int>() << KPlato::ChartItemModel::SPIEffort << KPlato::ChartItemModel::CPIEffort;
    const QList<int> costWork = QList<int>() << KPlato::ChartItemModel::BCWSCost << KPlato::ChartItemModel::BCWPCost << KPlato::ChartItemModel::ACWPCost;
    const QList<int> costPI = QList<int>() << KPlato::ChartItemModel::SPICost << KPlato::ChartItemModel::CPICost;

    QStringList axisTitles;
    QList<KPlato::ChartProxyModel*> models;
    KPlato::ChartProxyModel *m = new KPlato::ChartProxyModel(ui.splitter);
    m->setRejectColumns(costWork + effortWork + effortPI);
    m->setZeroColumns(costWork + effortWork + effortPI);
    m->setSourceModel(m_chartModel);
    models << m;

    m = new KPlato::ChartProxyModel(ui.splitter);
    m->setRejectColumns(costPI + effortWork + effortPI);
    m->setSourceModel(m_chartModel);
    models << m;

    m = new KPlato::ChartProxyModel(ui.splitter);
    m->setRejectColumns(effortWork + costWork + costPI);
    m->setSourceModel(m_chartModel);
    models << m;

    m = new KPlato::ChartProxyModel(ui.splitter);
    m->setRejectColumns(effortPI + costWork + costPI);
    m->setSourceModel(m_chartModel);
    models << m;

    // setup charts
    const QList<KChart::Chart*> charts = QList<KChart::Chart*>() << ui.costChartPi << ui.costChartWork << ui.effortChartPi << ui.effortChartWork;
    for (int i = 0; i < charts.count(); ++i) {
        KChart::LineDiagram *dia = new KChart::LineDiagram(charts.at(i));
        dia->setModel(models.at(i));
        KChart::CartesianAxis* xAxis = new KChart::CartesianAxis(dia);
        KChart::CartesianAxis* yAxis = new KChart::CartesianAxis(dia);
        xAxis->setPosition(KChart::CartesianAxis::Bottom);
        yAxis->setPosition(KChart::CartesianAxis::Left);
        yAxis->setTitleText(axisTitles.value(i));
        dia->addAxis(xAxis);
        dia->addAxis(yAxis);
        m_yAxes << yAxis;
        charts.at(i)->coordinatePlane()->replaceDiagram(dia);
        KChart::Legend *legend = new KChart::Legend(dia);
        legend->setPosition(KChart::Position::North);
        legend->setOrientation(Qt::Horizontal);
        legend->setTitleText(QString());
        charts.at(i)->addLegend(legend);
    }
}

void PerformanceView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // HACK: Workaround for misbehaving KChart legend resize
    const QList<KChart::Chart*> charts = QList<KChart::Chart*>() << ui.costChartPi << ui.costChartWork << ui.effortChartPi << ui.effortChartWork;
    for (auto chart : charts) {
        auto legend = chart->legend();
        if (legend && legend->orientation() == Qt::Horizontal) {
            legend->forceRebuild();
        }
    }
}

void PerformanceView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * PerformanceView::popupMenu(const QString& name)
{
    Q_UNUSED(name)
    return nullptr;
}

KoPrintJob *PerformanceView::createPrintJob()
{
    return nullptr;
}

void PerformanceView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)
    QModelIndexList indexes = ui.treeView->selectionModel()->selectedRows();
    KPlato::Project *project = indexes.value(0).data(PROJECT_ROLE).value<KPlato::Project*>();
    Q_ASSERT(project);
    m_chartModel->setProject(project);
    KoDocument *doc = indexes.value(0).data(DOCUMENT_ROLE).value<KoDocument*>();
    Q_ASSERT(doc);
    m_chartModel->setScheduleManager(project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString()));
    m_chartModel->setNodes(QList<KPlato::Node*>() << project);

    int row = m_chartModel->rowForDate(QDate::currentDate());
    QString text = i18n("No data");
    if (row < 0) {
        ui.costSPI->setText(text);
        ui.costCPI->clear();
        ui.costBCWS->setText(text);
        ui.costBCWP->clear();
        ui.costACWP->clear();
        ui.effortSPI->setText(text);
        ui.effortCPI->clear();
        ui.effortBCWS->setText(text);
        ui.effortBCWP->clear();
        ui.effortACWP->clear();
        return;
    }
    QString costTitle = i18n("Cost (%1)", project->config().locale()->currencySymbol());
    m_yAxes.at(1)->setTitleText(costTitle);
    QString effortTitle = i18n("Effort (Hour)");
    m_yAxes.at(3)->setTitleText(effortTitle);

    double value =  m_chartModel->index(row, KPlato::ChartItemModel::SPICost).data(Qt::EditRole).toDouble();
    text = i18n("SPI: %1", QStringLiteral("%1").arg(value, 0, 'f', 2));
    ui.costSPI->setText(text);

    value =  m_chartModel->index(row, KPlato::ChartItemModel::CPICost).data(Qt::EditRole).toDouble();
    text = i18n("CPI: %1", QStringLiteral("%1").arg(value, 0, 'f', 2));
    ui.costCPI->setText(text);

    value =  m_chartModel->index(row, KPlato::ChartItemModel::BCWSCost).data(Qt::EditRole).toDouble();
    text = i18n("BCWS: %1", QStringLiteral("%1").arg(value, 0, 'f', 0));
    ui.costBCWS->setText(text);

    value =  m_chartModel->index(row, KPlato::ChartItemModel::BCWPCost).data(Qt::EditRole).toDouble();
    text = i18n("BCWP: %1", QStringLiteral("%1").arg(value, 0, 'f', 0));
    ui.costBCWP->setText(text);

    value =  m_chartModel->index(row, KPlato::ChartItemModel::ACWPCost).data(Qt::EditRole).toDouble();
    text = i18n("ACWP: %1", QStringLiteral("%1").arg(value, 0, 'f', 0));
    ui.costACWP->setText(text);

    value =  m_chartModel->index(row, KPlato::ChartItemModel::SPIEffort).data(Qt::EditRole).toDouble();
    text = i18n("SPI: %1", QStringLiteral("%1").arg(value, 0, 'f', 2));
    ui.effortSPI->setText(text);

    value =  m_chartModel->index(row, KPlato::ChartItemModel::CPIEffort).data(Qt::EditRole).toDouble();
    text = i18n("CPI: %1", QStringLiteral("%1").arg(value, 0, 'f', 2));
    ui.effortCPI->setText(text);

    value =  m_chartModel->index(row, KPlato::ChartItemModel::BCWSEffort).data(Qt::EditRole).toDouble();
    text = i18n("BCWS: %1", QStringLiteral("%1").arg(value, 0, 'f', 1));
    ui.effortBCWS->setText(text);

    value =  m_chartModel->index(row, KPlato::ChartItemModel::BCWPEffort).data(Qt::EditRole).toDouble();
    text = i18n("BCWP: %1", QStringLiteral("%1").arg(value, 0, 'f', 1));
    ui.effortBCWP->setText(text);

    value =  m_chartModel->index(row, KPlato::ChartItemModel::ACWPEffort).data(Qt::EditRole).toDouble();
    text = i18n("ACWP: %1", QStringLiteral("%1").arg(value, 0, 'f', 1));
    ui.effortACWP->setText(text);

}
