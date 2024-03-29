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
#include <ExtraProperties.h>

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
    setXMLFile(QStringLiteral("Portfolio_PerformanceViewUi.rc"));

    ui.setupUi(this);

    ProjectsFilterModel *model = new ProjectsFilterModel(ui.treeView);
    model->setAcceptedColumns(QList<int>() << KPlato::NodeModel::NodeName);
    model->setPortfolio(qobject_cast<MainDocument*>(doc));
    ui.treeView->setModel(model);

    setupCharts();

    connect(static_cast<MainDocument*>(doc), &MainDocument::documentChanged, this, &PerformanceView::slotDocumentChanged);
    connect(ui.treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &PerformanceView::currentChanged);
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

void PerformanceView::slotDocumentChanged(KoDocument *doc)
{
    const auto idx = ui.treeView->currentIndex();
    auto project = idx.data(PROJECT_ROLE).value<KPlato::Project*>();
    if (project && project == doc->project()) {
        currentChanged(idx);
    }

}

void PerformanceView::currentChanged(const QModelIndex &current)
{
    KPlato::Project *project = nullptr;
    KPlato::ScheduleManager *sm = nullptr;
    auto doc = current.data(DOCUMENT_ROLE).value<KoDocument*>();
    if (doc) {
        project = doc->project();
        sm = project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
    }
    updateCharts(project, sm);
}

void PerformanceView::updateCharts(KPlato::Project *project, KPlato::ScheduleManager *sm)
{
    m_chartModel->setProject(project);
    m_chartModel->setNodes(project ? QList<KPlato::Node*>() << project : QList<KPlato::Node*>());
    m_chartModel->setScheduleManager(sm);

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

void PerformanceView::saveSettings(QDomElement &settings) const
{
    settings.setAttribute(QStringLiteral("current-row"), ui.treeView->currentIndex().row());
    qreal w = 0.0;
    qreal w0 = ui.splitter_2->sizes().value(0);
    if (w0 > 0.) {
        w = w0 + ui.splitter_2->sizes().value(1) / w0;
    }
    const auto treesize = w / (ui.splitter_2->size().rwidth() - ui.splitter_2->handleWidth());
    settings.setAttribute(QStringLiteral("tree-size"), treesize);

    w = 0.0;
    w0 = ui.splitter->sizes().value(0);
    if (w0 > 0.) {
        w = w0 + ui.splitter->sizes().value(1) / w0;
    }
    const auto chartsize = w / (ui.splitter->size().rwidth() - ui.splitter->handleWidth());
    settings.setAttribute(QStringLiteral("chart-size"), chartsize);
}

void PerformanceView::loadSettings(KoXmlElement &settings)
{
    const auto idx = ui.treeView->model()->index(settings.attribute(QStringLiteral("current-row")).toInt(), 0);
    ui.treeView->setCurrentIndex(idx);
    const auto treesize = ui.splitter_2->size().rwidth();
    int w = treesize * settings.attribute(QStringLiteral("tree-size"), QStringLiteral("0.2")).toDouble();
    ui.splitter_2->setSizes(QList<int>() << w << treesize - w);
    const auto chartsize = ui.splitter->size().rwidth();
    w = chartsize * settings.attribute(QStringLiteral("chart-size"), QStringLiteral("0.5")).toDouble();
    ui.splitter->setSizes(QList<int>() << w << chartsize - w);
}
