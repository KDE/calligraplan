/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007-2010, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "PerformanceStatusBase.h"

#include "kptglobal.h"
#include "kptlocale.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kptschedule.h"
#include "kpteffortcostmap.h"
#include "kptdebug.h"

#include <KoXmlReader.h>
#include "KoDocument.h"
#include "KoPageLayoutWidget.h"

#include <KChartChart>
#include <KChartAbstractCoordinatePlane>
#include <KChartBarDiagram>
#include <KChartLineDiagram>
#include <KChartCartesianAxis>
#include <KChartCartesianCoordinatePlane>
#include <KChartLegend>
#include <KChartBackgroundAttributes>
#include <KChartGridAttributes>

#include <QClipboard>

using namespace KChart;

using namespace KPlato;

PerformanceStatusPrintingDialog::PerformanceStatusPrintingDialog(ViewBase *view, PerformanceStatusBase *chart, Project *project)
    : PrintingDialog(view),
    m_chart(chart),
    m_project(project)
{
}

int PerformanceStatusPrintingDialog::documentLastPage() const
{
    return documentFirstPage();
}

QList<QWidget*> PerformanceStatusPrintingDialog::createOptionWidgets() const
{
    QList<QWidget*> lst;
    lst << createPageLayoutWidget();
    lst += PrintingDialog::createOptionWidgets();
    return  lst;
}

void PerformanceStatusPrintingDialog::printPage(int page, QPainter &painter)
{
    //debugPlan<<page<<printer().pageLayout().paintrectpixels(resolution)<<printer().paperRect()<<printer().margins()<<printer().fullPage();
    painter.save();
    QRect rect = printer().pageLayout().paintRectPixels(printer().resolution());
    rect.moveTo(0, 0); // the printer already has margins set
    QRect header = headerRect();
    QRect footer = footerRect();
    paintHeaderFooter(painter, printingOptions(), page, *m_project);
    int gap = 8;
    if (header.isValid()) {
        rect.setTop(header.height() + gap);
    }
    if (footer.isValid()) {
        rect.setBottom(rect.bottom() - footer.height() - gap);
    }
    QSize s = m_chart->ui_chart->geometry().size();
    qreal r = (qreal)s.width() / (qreal)s.height();
    if (rect.height() > rect.width() && r > 0.0) {
        rect.setHeight(rect.width() / r);
    }
    debugPlan<<s<<rect;
    m_chart->ui_chart->paint(&painter, rect);
    painter.restore();
}

//-----------------------------------
PerformanceStatusBase::PerformanceStatusBase(QWidget *parent)
    : QWidget(parent),
    m_project(nullptr),
    m_manager(nullptr)
{
    setupUi(this);

    ui_performancetable->setModel(new PerformanceDataCurrentDateModel(this));

    BackgroundAttributes backgroundAttrs(ui_chart->backgroundAttributes());
    backgroundAttrs.setVisible(true);
    backgroundAttrs.setBrush(Qt::white);
    ui_chart->setBackgroundAttributes(backgroundAttrs);

    m_legend = new Legend(ui_chart);
    ui_chart->replaceLegend(m_legend);
    m_legend->setObjectName(QStringLiteral("Chart legend"));

    backgroundAttrs = m_legend->backgroundAttributes();
    m_legend->setBackgroundAttributes(backgroundAttrs);
    backgroundAttrs.setVisible(true);
    backgroundAttrs.setBrush(Qt::white);

    m_legend->setPosition(Position::East);
    //m_legend->setAlignment((Qt::Alignment)(Qt::AlignTop | Qt::AlignCenter));
    m_legenddiagram.setModel(&m_chartmodel);
    m_legenddiagram.setObjectName(QStringLiteral("Legend diagram"));
    m_legend->setDiagram(&m_legenddiagram);

    // get rid of the default coordinate plane
    AbstractCoordinatePlane *p = ui_chart->coordinatePlane();
    ui_chart->takeCoordinatePlane(p);
    delete p;

    createBarChart();
    createLineChart();
    setupChart();
#ifdef PLAN_CHART_DEBUG
    ui_tableView->setModel(&m_chartmodel);
#endif
    setContextMenuPolicy(Qt::PreventContextMenu);
    connect(&m_chartmodel, &QAbstractItemModel::modelReset, this, &PerformanceStatusBase::slotUpdate);
    connect(ui_chart, &KPlato::Chart::customContextMenuRequested, this, &PerformanceStatusBase::customContextMenuRequested);
    ui_performancetable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_performancetable, &PerformanceTableView::contextMenuRequested, this, &PerformanceStatusBase::slotContextMenuRequested);
    connect(ui_stack, &QStackedWidget::customContextMenuRequested, this, [this](const QPoint &pos){
        Q_EMIT customContextMenuRequested(ui_stack->mapToGlobal(pos));
    });
}

void PerformanceStatusBase::slotContextMenuRequested(const QPoint &pos)
{
    Q_EMIT customContextMenuRequested(ui_performancetable->mapToGlobal(pos));
}

void PerformanceStatusBase::editCopy()
{
    QMimeData *mimeData = new QMimeData;
    QPixmap pixmap(size());
    render(&pixmap);
    mimeData->setImageData(pixmap);
    QGuiApplication::clipboard()->setMimeData(mimeData);
}

void PerformanceStatusBase::setChartInfo(const PerformanceChartInfo &info)
{
    if (info != m_chartinfo) {
        m_chartinfo = info;
        setupChart();
    }
}

void PerformanceStatusBase::refreshChart()
{
    ui_performancetable->resize(QSize());

    // NOTE: Force grid/axis recalculation, couldn't find a better way :(
    QResizeEvent event(ui_chart->size(), QSize());
    QApplication::sendEvent(ui_chart, &event);
    m_legend->forceRebuild();
}

void PerformanceStatusBase::createBarChart()
{
    m_barchart.effortplane = new CartesianCoordinatePlane(ui_chart);
    m_barchart.effortplane->setRubberBandZoomingEnabled(true);
    m_barchart.effortplane->setObjectName(QStringLiteral("Bar chart, Effort"));
    m_barchart.costplane = new CartesianCoordinatePlane(ui_chart);
    m_barchart.costplane->setRubberBandZoomingEnabled(true);
    m_barchart.costplane->setObjectName(QStringLiteral("Bar chart, Cost"));

    BarDiagram *effortdiagram = new BarDiagram(ui_chart, m_barchart.effortplane);
    effortdiagram->setObjectName(QStringLiteral("Effort diagram"));

    m_barchart.dateaxis = new CartesianAxis();
    m_barchart.dateaxis->setPosition(CartesianAxis::Bottom);

    m_barchart.effortaxis = new CartesianAxis(effortdiagram);
    m_barchart.effortaxis->setPosition(CartesianAxis::Right);
    effortdiagram->addAxis(m_barchart.effortaxis);
    m_barchart.effortplane->addDiagram(effortdiagram);

    // Hide cost in effort diagram
    effortdiagram->setHidden(0, true);
    effortdiagram->setHidden(1, true);
    effortdiagram->setHidden(2, true);
    m_barchart.effortproxy.setZeroColumns(QList<int>() << 0 << 1 << 2);

    m_barchart.effortproxy.setSourceModel(&m_chartmodel);
    effortdiagram->setModel(&(m_barchart.effortproxy));

    BarDiagram *costdiagram = new BarDiagram(ui_chart, m_barchart.costplane);
    costdiagram->setObjectName(QStringLiteral("Cost diagram"));

    m_barchart.costaxis = new CartesianAxis(costdiagram);
    m_barchart.costaxis->setPosition(CartesianAxis::Left);
    costdiagram->addAxis(m_barchart.costaxis);
    m_barchart.costplane->addDiagram(costdiagram);

    // Hide effort in cost diagram
    costdiagram->setHidden(3, true);
    costdiagram->setHidden(4, true);
    costdiagram->setHidden(5, true);
    m_barchart.costproxy.setZeroColumns(QList<int>() << 3 << 4 << 5);

    m_barchart.costproxy.setObjectName(QStringLiteral("Bar: Cost"));
    m_barchart.costproxy.setSourceModel(&m_chartmodel);
    costdiagram->setModel(&(m_barchart.costproxy));

    m_barchart.effortdiagram = effortdiagram;
    m_barchart.costdiagram = costdiagram;

    m_barchart.piplane = new CartesianCoordinatePlane(ui_chart);
    m_barchart.piplane->setObjectName(QStringLiteral("Performance Indices"));
    BarDiagram *pidiagram = new BarDiagram(ui_chart, m_barchart.piplane);
    pidiagram->setObjectName(QStringLiteral("PI diagram"));
    m_barchart.piaxis = new CartesianAxis(pidiagram);
    pidiagram->addAxis(m_barchart.piaxis);
    m_barchart.piplane->addDiagram(pidiagram);
    m_barchart.piproxy.setSourceModel(&m_chartmodel);
    pidiagram->setModel(&(m_barchart.piproxy));
}

void PerformanceStatusBase::createLineChart()
{
    m_linechart.effortplane = new CartesianCoordinatePlane(ui_chart);
    m_linechart.effortplane->setObjectName(QStringLiteral("Line chart, Effort"));
    m_linechart.effortplane->setRubberBandZoomingEnabled(true);
    m_linechart.costplane = new CartesianCoordinatePlane(ui_chart);
    m_linechart.costplane->setObjectName(QStringLiteral("Line chart, Cost"));
    m_linechart.costplane->setRubberBandZoomingEnabled(true);

    LineDiagram *effortdiagram = new LineDiagram(ui_chart, m_linechart.effortplane);
    effortdiagram->setObjectName(QStringLiteral("Effort diagram"));

    m_linechart.dateaxis = new CartesianAxis();
    m_linechart.dateaxis->setPosition(CartesianAxis::Bottom);

    m_linechart.effortaxis = new CartesianAxis(effortdiagram);
    m_linechart.effortaxis->setPosition(CartesianAxis::Right);
    effortdiagram->addAxis(m_linechart.effortaxis);
    m_linechart.effortplane->addDiagram(effortdiagram);

    // Hide cost in effort diagram
    effortdiagram->setHidden(0, true);
    effortdiagram->setHidden(1, true);
    effortdiagram->setHidden(2, true);
    m_linechart.effortproxy.setZeroColumns(QList<int>() << 0 << 1 << 2);

    m_linechart.effortproxy.setObjectName(QStringLiteral("Line: Effort"));
    m_linechart.effortproxy.setSourceModel(&m_chartmodel);
    effortdiagram->setModel(&(m_linechart.effortproxy));

    LineDiagram *costdiagram = new LineDiagram(ui_chart, m_linechart.costplane);
    costdiagram->setObjectName(QStringLiteral("Cost diagram"));

    m_linechart.costaxis = new CartesianAxis(costdiagram);
    m_linechart.costaxis->setPosition(CartesianAxis::Left);
    costdiagram->addAxis(m_linechart.costaxis);
    m_linechart.costplane->addDiagram(costdiagram);
    // Hide effort in cost diagram
    costdiagram->setHidden(3, true);
    costdiagram->setHidden(4, true);
    costdiagram->setHidden(5, true);

    m_linechart.costproxy.setObjectName(QStringLiteral("Line: Cost"));
    m_linechart.costproxy.setZeroColumns(QList<int>() << 3 << 4 << 5);
    m_linechart.costproxy.setSourceModel(&m_chartmodel);
    costdiagram->setModel(&(m_linechart.costproxy));

    m_linechart.effortdiagram = effortdiagram;
    m_linechart.costdiagram = costdiagram;

    m_linechart.piplane = new CartesianCoordinatePlane(ui_chart);
    m_linechart.piplane->setObjectName(QStringLiteral("Performance Indices"));
    m_linechart.piplane->setRubberBandZoomingEnabled(true);
    LineDiagram *pidiagram = new LineDiagram(ui_chart, m_linechart.piplane);
    pidiagram->setObjectName(QStringLiteral("PI diagram"));
    m_linechart.piaxis = new CartesianAxis(pidiagram);
    pidiagram->addAxis(m_linechart.piaxis);
    m_linechart.piplane->addDiagram(pidiagram);
    m_linechart.piproxy.setSourceModel(&m_chartmodel);
    pidiagram->setModel(&(m_linechart.piproxy));
}

void PerformanceStatusBase::setupChart()
{
    while (! ui_chart->coordinatePlanes().isEmpty()) {
        const CoordinatePlaneList &planes = ui_chart->coordinatePlanes();
        ui_chart->takeCoordinatePlane(planes.last());
    }
    if (m_chartinfo.showBarChart) {
        setupChart(m_barchart);
    } else if (m_chartinfo.showLineChart) {
        setupChart(m_linechart);
    } else {
#ifdef PLAN_CHART_DEBUG
        ui_stack->setCurrentIndex(1);
        refreshChart();
        return;
#else
        setupChart(m_linechart);
#endif
    }
    ui_stack->setCurrentIndex(0);
    debugPlan<<"Planes:"<<ui_chart->coordinatePlanes();
    const QList<AbstractCoordinatePlane*> planes = ui_chart->coordinatePlanes();
    for (AbstractCoordinatePlane *pl : planes) {
        CartesianCoordinatePlane *p = dynamic_cast<CartesianCoordinatePlane*>(pl);
        if (p == nullptr) continue;
        GridAttributes ga = p->globalGridAttributes();
        ga.setGridVisible(p->referenceCoordinatePlane() == nullptr);
        p->setGlobalGridAttributes(ga);
    }
    m_legend->setDatasetHidden(0, ! (m_chartinfo.showBaseValues && m_chartinfo.showCost && m_chartinfo.showBCWSCost));
    m_legend->setDatasetHidden(1, ! (m_chartinfo.showBaseValues && m_chartinfo.showCost && m_chartinfo.showBCWPCost));
    m_legend->setDatasetHidden(2, ! (m_chartinfo.showBaseValues && m_chartinfo.showCost && m_chartinfo.showACWPCost));
    m_legend->setDatasetHidden(3, ! (m_chartinfo.showBaseValues && m_chartinfo.showEffort && m_chartinfo.showBCWSEffort));
    m_legend->setDatasetHidden(4, ! (m_chartinfo.showBaseValues && m_chartinfo.showEffort && m_chartinfo.showBCWPEffort));
    m_legend->setDatasetHidden(5, ! (m_chartinfo.showBaseValues && m_chartinfo.showEffort && m_chartinfo.showACWPEffort));
    // spi/cpi
    m_legend->setDatasetHidden(6, ! (m_chartinfo.showIndices && m_chartinfo.showSpiCost));
    m_legend->setDatasetHidden(7, ! (m_chartinfo.showIndices && m_chartinfo.showCpiCost));
    m_legend->setDatasetHidden(8, ! (m_chartinfo.showIndices && m_chartinfo.showSpiEffort));
    m_legend->setDatasetHidden(9, ! (m_chartinfo.showIndices && m_chartinfo.showCpiEffort));

    setEffortValuesVisible(m_chartinfo.effortShown());
    setCostValuesVisible(m_chartinfo.costShown());
    refreshChart();
}

void PerformanceStatusBase::setEffortValuesVisible(bool visible)
{
    ui_performancetable->verticalHeader()->setSectionHidden(1, ! visible);
    ui_performancetable->setMaximumHeight(ui_performancetable->sizeHint().height());
}

void PerformanceStatusBase::setCostValuesVisible(bool visible)
{
    ui_performancetable->verticalHeader()->setSectionHidden(0, ! visible);
    ui_performancetable->setMaximumHeight(ui_performancetable->sizeHint().height());
}

void PerformanceStatusBase::setupChart(ChartContents &cc)
{
    QList<int> erc, ezc, crc, czc; // sourcemodel column numbers
    int effort_start_column = 3; // proxy column number

    const PerformanceChartInfo &info = m_chartinfo;
    debugPlan<<"cost="<<info.showCost<<"effort="<<info.showEffort;
    static_cast<AbstractCartesianDiagram*>(cc.effortplane->diagram())->takeAxis(cc.dateaxis);
    static_cast<AbstractCartesianDiagram*>(cc.costplane->diagram())->takeAxis(cc.dateaxis);
    static_cast<AbstractCartesianDiagram*>(cc.piplane->diagram())->takeAxis(cc.dateaxis);
    cc.costplane->setReferenceCoordinatePlane(nullptr);
    if (info.showBaseValues) {
        if (info.showEffort) {
            // filter cost columns if cost is *not* shown, else hide them and zero out
            if (! info.showCost) {
                erc << 0 << 1 << 2;
                effort_start_column = 0; // no cost, so effort start at 0
            } else {
                ezc << 0 << 1 << 2;
                cc.effortplane->diagram()->setHidden(0, true);
                cc.effortplane->diagram()->setHidden(1, true);
                cc.effortplane->diagram()->setHidden(2, true);
            }
            // always disable spi/cpi
            erc << 6 << 7 << 8 << 9;
            ezc << 6 << 7 << 8 << 9;
            // if cost is shown don't return a cost value or else it goes into the effort axis scale calculation
            //cc.effortproxy.setZeroColumns(info.showCost ? QList<int>() << 0 << 1 << 2 : QList<int>() << 3 << 4 << 5);
            cc.effortaxis->setPosition(info.showCost ? CartesianAxis::Right : CartesianAxis::Left);
            ui_chart->addCoordinatePlane(cc.effortplane);

            static_cast<AbstractCartesianDiagram*>(cc.effortplane->diagram())->addAxis(cc.dateaxis);
            cc.effortplane->setGridNeedsRecalculate();
        }
        if (info.showCost) {
            // Should never get any effort values in cost diagram
            czc << 3 << 4 << 5;
            // remove effort columns from cost if no effort is shown, else hide them
            if (! info.showEffort) {
                crc << 3 << 4 << 5;
            } else {
                cc.costplane->diagram()->setHidden(3, true);
                cc.costplane->diagram()->setHidden(4, true);
                cc.costplane->diagram()->setHidden(5, true);
            }
            // always disable spi/cpi
            erc << 6 << 7 << 8 << 9;
            ezc << 6 << 7 << 8 << 9;

            cc.costplane->setReferenceCoordinatePlane(info.showEffort ? cc.effortplane : nullptr);
            ui_chart->addCoordinatePlane(cc.costplane);

            static_cast<AbstractCartesianDiagram*>(cc.costplane->diagram())->addAxis(cc.dateaxis);
            cc.costplane->setGridNeedsRecalculate();

            cc.costplane->diagram()->setHidden(0, ! info.showBCWSCost);
            cc.costplane->diagram()->setHidden(1, ! info.showBCWPCost);
            cc.costplane->diagram()->setHidden(2, ! info.showACWPCost);
        }

        if (info.showEffort) {
            cc.effortplane->diagram()->setHidden(effort_start_column, ! info.showBCWSEffort);
            cc.effortplane->diagram()->setHidden(effort_start_column+1, ! info.showBCWPEffort);
            cc.effortplane->diagram()->setHidden(effort_start_column+2, ! info.showACWPEffort);
            cc.effortaxis->setCachedSizeDirty();
            cc.effortproxy.reset();
            cc.effortproxy.setZeroColumns(ezc);
            cc.effortproxy.setRejectColumns(erc);
        }
        if (info.showCost) {
            cc.costaxis->setCachedSizeDirty();
            cc.costproxy.reset();
            cc.costproxy.setZeroColumns(czc);
            cc.costproxy.setRejectColumns(crc);
        }
    } else if (info.showIndices) {
        cc.piaxis->setPosition(CartesianAxis::Left);
        ui_chart->addCoordinatePlane(cc.piplane);
        static_cast<AbstractCartesianDiagram*>(cc.piplane->diagram())->addAxis(cc.dateaxis);
        cc.piplane->setGridNeedsRecalculate();
        cc.piaxis->setCachedSizeDirty();

        cc.piproxy.reset();
        QList<int> reject; reject << 0 << 1 << 2 << 3 << 4 << 5;
        if (! info.showSpiCost) {
            reject << ChartItemModel::SPICost;
        }
        if (! info.showCpiCost) {
            reject << ChartItemModel::CPICost;
        }
        if (! info.showSpiEffort) {
            reject <<  ChartItemModel::SPIEffort;
        }
        if (! info.showCpiEffort) {
            reject <<  ChartItemModel::CPIEffort;
        }
        cc.piproxy.setRejectColumns(reject);
    }
#if 0
    debugPlan<<"Effort:"<<info.showEffort;
    if (info.showEffort && cc.effortproxy.rowCount() > 0) {
        debugPlan<<"Effort:"<<info.showEffort<<"columns ="<<cc.effortproxy.columnCount()
            <<"reject="<<cc.effortproxy.rejectColumns()
            <<"zero="<<cc.effortproxy.zeroColumns();
        int row = cc.effortproxy.rowCount()-1;
        for (int i = 0; i < cc.effortproxy.columnCount(); ++i) {
            debugPlan<<"data ("<<row<<","<<i<<":"<<cc.effortproxy.index(row,i).data().toString()<<(cc.effortplane->diagram()->isHidden(i)?"hide":"show");
        }
    }
    debugPlan<<"Cost:"<<info.showCost;
    if (info.showCost && cc.costproxy.rowCount() > 0) {
        debugPlan<<"Cost:"<<info.showCost<<"columns ="<<cc.costproxy.columnCount()
            <<"reject="<<cc.costproxy.rejectColumns()
            <<"zero="<<cc.costproxy.zeroColumns();
        int row = cc.costproxy.rowCount()-1;
        for (int i = 0; i < cc.costproxy.columnCount(); ++i) {
            debugPlan<<"data ("<<row<<","<<i<<":"<<cc.costproxy.index(row,i).data().toString()<<(cc.costplane->diagram()->isHidden(i)?"hide":"show");
        }
    }

    for (AbstractCoordinatePlane *p : std::as_const(ui_chart->coordinatePlanes())) {
        debugPlan<<p<<"references:"<<p->referenceCoordinatePlane();
        for (AbstractDiagram *d : std::as_const(p->diagrams())) {
            debugPlan<<p<<"diagram:"<<d;
        }
    }
#endif
}

void PerformanceStatusBase::contextMenuEvent(QContextMenuEvent *event)
{
    debugPlan<<event->globalPos();
    if (event->modifiers() == Qt::NoModifier) {
        Q_EMIT customContextMenuRequested(event->globalPos());
    }
}

void PerformanceStatusBase::slotUpdate()
{
    //debugPlan;
    refreshChart();
}

void PerformanceStatusBase::setScheduleManager(ScheduleManager *sm)
{
    //debugPlan;
    if (sm == m_manager) {
        return;
    }
    m_manager = sm;
    m_chartmodel.setScheduleManager(sm);
    qobject_cast<PerformanceDataCurrentDateModel*>(ui_performancetable->model())->setScheduleManager(sm);
}

void PerformanceStatusBase::setProject(Project *project)
{
    if (m_project) {
        disconnect(m_project, &Project::localeChanged, this, &PerformanceStatusBase::slotLocaleChanged);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::localeChanged, this, &PerformanceStatusBase::slotLocaleChanged);
    }
    m_chartmodel.setProject(project);
    qobject_cast<PerformanceDataCurrentDateModel*>(ui_performancetable->model())->setProject(project);

    slotLocaleChanged();
}

void PerformanceStatusBase::slotLocaleChanged()
{
    debugPlan;

    const QString currencySymbol = m_project->locale()->currencySymbol();

    m_linechart.costaxis->setTitleText(i18nc("Chart axis title 1=currency symbol", "Cost (%1)", currencySymbol));
    m_linechart.effortaxis->setTitleText(i18nc("Chart axis title", "Effort (hours)"));

    m_barchart.costaxis->setTitleText(i18nc("Chart axis title 1=currency symbol", "Cost (%1)", currencySymbol));
    m_barchart.effortaxis->setTitleText(i18nc("Chart axis title", "Effort (hours)"));
}


bool PerformanceStatusBase::loadContext(const KoXmlElement &context)
{
    debugPlan;
    m_chartinfo.showBarChart = context.attribute("show-bar-chart", QString::number(0)).toInt();
    m_chartinfo.showLineChart = context.attribute("show-line-chart", QString::number(1)).toInt();
    m_chartinfo.showTableView = context.attribute("show-table-view", QString::number(0)).toInt();

    m_chartinfo.showBaseValues = context.attribute("show-base-values", QString::number(1)).toInt();
    m_chartinfo.showIndices = context.attribute("show-indeces", QString::number(0)).toInt();

    m_chartinfo.showCost = context.attribute("show-cost", QString::number(1)).toInt();
    m_chartinfo.showBCWSCost = context.attribute("show-bcws-cost", QString::number(1)).toInt();
    m_chartinfo.showBCWPCost = context.attribute("show-bcwp-cost", QString::number(1)).toInt();
    m_chartinfo.showACWPCost = context.attribute("show-acwp-cost", QString::number(1)).toInt();

    m_chartinfo.showEffort = context.attribute("show-effort", QString::number(1)).toInt();
    m_chartinfo.showBCWSEffort = context.attribute("show-bcws-effort", QString::number(1)).toInt();
    m_chartinfo.showBCWPEffort = context.attribute("show-bcwp-effort", QString::number(1)).toInt();
    m_chartinfo.showACWPEffort = context.attribute("show-acwp-effort", QString::number(1)).toInt();

    m_chartinfo.showSpiCost = context.attribute("show-spi-cost", QString::number(1)).toInt();
    m_chartinfo.showCpiCost = context.attribute("show-cpi-cost", QString::number(1)).toInt();
    m_chartinfo.showSpiEffort = context.attribute("show-spi-effort", QString::number(1)).toInt();
    m_chartinfo.showCpiEffort = context.attribute("show-cpi-effort", QString::number(1)).toInt();

    debugPlan<<"Cost:"<<m_chartinfo.showCost<<"bcws="<<m_chartinfo.showBCWSCost<<"bcwp="<<m_chartinfo.showBCWPCost<<"acwp="<<m_chartinfo.showACWPCost;
    debugPlan<<"Effort:"<<m_chartinfo.showCost<<"bcws="<<m_chartinfo.showBCWSCost<<"bcwp="<<m_chartinfo.showBCWPCost<<"acwp="<<m_chartinfo.showACWPCost;
    setupChart();
    return true;
}

void PerformanceStatusBase::saveContext(QDomElement &context) const
{
    context.setAttribute(QStringLiteral("show-bar-chart"), QString::number(m_chartinfo.showBarChart));
    context.setAttribute(QStringLiteral("show-line-chart"), QString::number(m_chartinfo.showLineChart));
    context.setAttribute(QStringLiteral("show-table-view"), QString::number(m_chartinfo.showTableView));

    context.setAttribute(QStringLiteral("show-base-values"), QString::number(m_chartinfo.showBaseValues));
    context.setAttribute(QStringLiteral("show-indeces"), QString::number(m_chartinfo.showIndices));

    context.setAttribute(QStringLiteral("show-cost"), QString::number(m_chartinfo.showCost));
    context.setAttribute(QStringLiteral("show-bcws-cost"), QString::number(m_chartinfo.showBCWSCost));
    context.setAttribute(QStringLiteral("show-bcwp-cost"), QString::number(m_chartinfo.showBCWPCost));
    context.setAttribute(QStringLiteral("show-acwp-cost"), QString::number(m_chartinfo.showACWPCost));

    context.setAttribute(QStringLiteral("show-effort"),  QString::number(m_chartinfo.showEffort));
    context.setAttribute(QStringLiteral("show-bcws-effort"), QString::number(m_chartinfo.showBCWSEffort));
    context.setAttribute(QStringLiteral("show-bcwp-effort"), QString::number(m_chartinfo.showBCWPEffort));
    context.setAttribute(QStringLiteral("show-acwp-effort"), QString::number(m_chartinfo.showACWPEffort));

    context.setAttribute(QStringLiteral("show-spi-cost"), QString::number(m_chartinfo.showSpiCost));
    context.setAttribute(QStringLiteral("show-cpi-cost"), QString::number(m_chartinfo.showCpiCost));
    context.setAttribute(QStringLiteral("show-spi-effort"), QString::number(m_chartinfo.showSpiEffort));
    context.setAttribute(QStringLiteral("show-cpi-effort"), QString::number(m_chartinfo.showCpiEffort));
}

KoPrintJob *PerformanceStatusBase::createPrintJob(ViewBase *parent)
{
    PerformanceStatusPrintingDialog *dia = new PerformanceStatusPrintingDialog(parent, this, parent->project());
    dia->printer().setCreator(QStringLiteral("Plan"));
    return dia;
}

void PerformanceStatusBase::setNodes(const QList<Node *> &nodes)
{
    m_chartmodel.setNodes(nodes);
    qobject_cast<PerformanceDataCurrentDateModel*>(ui_performancetable->model())->setNodes(nodes);
}


//------------------------------------------------
PerformanceStatusViewSettingsPanel::PerformanceStatusViewSettingsPanel(PerformanceStatusBase *view, QWidget *parent)
    : QWidget(parent),
    m_view(view)
{
    setupUi(this);
#ifndef PLAN_CHART_DEBUG
    ui_table->hide();
#endif
    PerformanceChartInfo info = m_view->chartInfo();

    ui_linechart->setChecked(info.showLineChart);
    ui_barchart->setChecked(info.showBarChart);
#ifdef PLAN_CHART_DEBUG
    ui_table->setChecked(info.showTableView);
#endif
    ui_bcwsCost->setCheckState(info.showBCWSCost ? Qt::Checked : Qt::Unchecked);
    ui_bcwpCost->setCheckState(info.showBCWPCost ? Qt::Checked : Qt::Unchecked);
    ui_acwpCost->setCheckState(info.showACWPCost ? Qt::Checked : Qt::Unchecked);
    ui_cost->setChecked(info.showCost);

    ui_bcwsEffort->setCheckState(info.showBCWSEffort ? Qt::Checked : Qt::Unchecked);
    ui_bcwpEffort->setCheckState(info.showBCWPEffort ? Qt::Checked : Qt::Unchecked);
    ui_acwpEffort->setCheckState(info.showACWPEffort ? Qt::Checked : Qt::Unchecked);
    ui_effort->setChecked(info.showEffort);

    ui_showbasevalues->setChecked(info.showBaseValues);
    ui_showindices->setChecked(info.showIndices);

    ui_spicost->setCheckState(info.showSpiCost ? Qt::Checked : Qt::Unchecked);
    ui_cpicost->setCheckState(info.showCpiCost ? Qt::Checked : Qt::Unchecked);
    ui_spieffort->setCheckState(info.showSpiEffort ? Qt::Checked : Qt::Unchecked);
    ui_cpieffort->setCheckState(info.showCpiEffort ? Qt::Checked : Qt::Unchecked);

    connect(ui_showbasevalues, &QAbstractButton::toggled, this, &PerformanceStatusViewSettingsPanel::switchStackWidget);
    connect(ui_showindices, &QAbstractButton::toggled, this, &PerformanceStatusViewSettingsPanel::switchStackWidget);

    switchStackWidget();
}

void PerformanceStatusViewSettingsPanel::slotOk()
{
    PerformanceChartInfo info;
    info.showLineChart = ui_linechart->isChecked();
    info.showBarChart = ui_barchart->isChecked();
    info.showTableView = ui_table->isChecked();

    info.showBaseValues = ui_showbasevalues->isChecked();
    info.showIndices = ui_showindices->isChecked();

    info.showBCWSCost = ui_bcwsCost->checkState() == Qt::Unchecked ? false : true;
    info.showBCWPCost = ui_bcwpCost->checkState() == Qt::Unchecked ? false : true;
    info.showACWPCost = ui_acwpCost->checkState() == Qt::Unchecked ? false : true;
    info.showCost = ui_cost->isChecked();

    info.showBCWSEffort = ui_bcwsEffort->checkState() == Qt::Unchecked ? false : true;
    info.showBCWPEffort = ui_bcwpEffort->checkState() == Qt::Unchecked ? false : true;
    info.showACWPEffort = ui_acwpEffort->checkState() == Qt::Unchecked ? false : true;
    info.showEffort = ui_effort->isChecked();

    info.showSpiCost = ui_spicost->isChecked();
    info.showCpiCost = ui_cpicost->isChecked();
    info.showSpiEffort = ui_spieffort->isChecked();
    info.showCpiEffort = ui_cpieffort->isChecked();

    m_view->setChartInfo(info);
}

void PerformanceStatusViewSettingsPanel::setDefault()
{
    ui_linechart->setChecked(true);

    ui_bcwsCost->setCheckState(Qt::Checked);
    ui_bcwpCost->setCheckState(Qt::Checked);
    ui_acwpCost->setCheckState(Qt::Checked);
    ui_cost->setChecked(true);

    ui_bcwsEffort->setCheckState(Qt::Checked);
    ui_bcwpEffort->setCheckState(Qt::Checked);
    ui_acwpEffort->setCheckState(Qt::Checked);
    ui_effort->setChecked(Qt::Unchecked);

    ui_showbasevalues->setChecked(true);
    ui_showindices->setChecked(false);

    ui_spicost->setCheckState(Qt::Checked);
    ui_cpicost->setCheckState(Qt::Checked);
    ui_spieffort->setCheckState(Qt::Checked);
    ui_cpieffort->setCheckState(Qt::Checked);
}

void PerformanceStatusViewSettingsPanel::switchStackWidget()
{
    if (ui_showbasevalues->isChecked()) {
        ui_stackWidget->setCurrentIndex(0);
    } else if (ui_showindices->isChecked()) {
        ui_stackWidget->setCurrentIndex(1);
    }
    //debugPlan<<sender()<<ui_stackWidget->currentIndex();
}
