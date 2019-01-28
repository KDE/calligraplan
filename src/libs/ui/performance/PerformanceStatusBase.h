/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2007 - 2010 Dag Andersen <danders@get2net.dk>
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

#ifndef PERFORMANCESTATUSBASE_H
#define PERFORMANCESTATUSBASE_H

#include "planui_export.h"

#include "kptitemmodelbase.h"

#include "kptviewbase.h"
#include "kptitemviewsettup.h"
#include "ui_kptperformancestatus.h"
#include "ui_kptperformancestatusviewsettingspanel.h"
#include "kptnodechartmodel.h"

#include <QSplitter>

#include <KChartBarDiagram>


class QItemSelection;

class KoDocument;
class KoPageLayoutWidget;
class PrintingHeaderFooter;

namespace KChart
{
    class CartesianCoordinatePlane;
    class CartesianAxis;
    class Legend;
};

namespace KPlato
{

class Project;
class Node;
class ScheduleManager;
class TaskStatusItemModel;
class NodeItemModel;
class PerformanceStatusBase;

typedef QList<Node*> NodeList;


struct PerformanceChartInfo
{
    bool showBarChart;
    bool showLineChart;
    bool showTableView;

    bool showBaseValues;
    bool showIndices;

    bool showCost;
    bool showBCWSCost;
    bool showBCWPCost;
    bool showACWPCost;

    bool showEffort;
    bool showBCWSEffort;
    bool showBCWPEffort;
    bool showACWPEffort;

    bool showSpiCost;
    bool showCpiCost;
    bool showSpiEffort;
    bool showCpiEffort;

    bool effortShown() const {
        return (showBaseValues && showEffort) || (showIndices && (showSpiEffort || showCpiEffort));
    }
    bool costShown() const {
        return (showBaseValues && showCost) || (showIndices && (showSpiCost || showCpiCost));
    }
    bool bcwsCost() const { return showBaseValues && showCost && showBCWSCost; }
    bool bcwpCost() const { return showBaseValues && showCost && showBCWPCost; }
    bool acwpCost() const { return showBaseValues && showCost && showACWPCost; }
    bool bcwsEffort() const { return showBaseValues && showEffort && showBCWSEffort; }
    bool bcwpEffort() const { return showBaseValues && showEffort && showBCWPEffort; }
    bool acwpEffort() const { return showBaseValues && showEffort && showACWPEffort; }

    bool spiCost() const { return showIndices && showSpiCost; }
    bool cpiCost() const { return showIndices && showCpiCost; }
    bool spiEffort() const { return showIndices && showSpiEffort; }
    bool cpiEffort() const { return showIndices && showCpiEffort; }

    PerformanceChartInfo() {
        showBarChart = false; showLineChart = true; showTableView = false;
        showBaseValues = true; showIndices = false;
        showCost = showBCWSCost = showBCWPCost = showACWPCost = true;
        showEffort = showBCWSEffort = showBCWPEffort = showACWPEffort = true;
        showSpiCost = showCost = showSpiEffort = showCpiEffort = true;
    }
    bool operator!=(const PerformanceChartInfo &o) const { return ! operator==(o); }
    bool operator==(const PerformanceChartInfo &o) const {
        return showBarChart == o.showBarChart && showLineChart == o.showLineChart &&
                showBaseValues == o.showBaseValues && showIndices == o.showIndices &&
                showCost == o.showCost && 
                showBCWSCost == o.showBCWSCost &&
                showBCWPCost == o.showBCWPCost &&
                showACWPCost == o.showACWPCost &&
                showEffort == o.showEffort &&
                showBCWSEffort == o.showBCWSEffort &&
                showBCWPEffort == o.showBCWPEffort &&
                showACWPEffort == o.showACWPEffort &&
                showSpiCost == o.showSpiCost &&
                showCpiCost == o.showCpiCost &&
                showSpiEffort == o.showSpiEffort &&
                showCpiEffort == o.showCpiEffort;
    }
};


class PLANUI_EXPORT PerformanceStatusPrintingDialog : public PrintingDialog
{
    Q_OBJECT
public:
    PerformanceStatusPrintingDialog(ViewBase *view, PerformanceStatusBase *chart, Project *project = 0);
    ~PerformanceStatusPrintingDialog() {}

    virtual int documentLastPage() const;
    virtual QList<QWidget*> createOptionWidgets() const;

protected:
    virtual void printPage(int pageNumber, QPainter &painter);

private:
    PerformanceStatusBase *m_chart;
    Project *m_project;
};

class PerformanceStatusBase : public QWidget, public Ui::PerformanceStatus
{
    Q_OBJECT
public:
    explicit PerformanceStatusBase(QWidget *parent);
    
    void setProject(Project *project);
    void setScheduleManager(ScheduleManager *sm);

    ChartItemModel *model() const { return const_cast<ChartItemModel*>(&m_chartmodel); }
    
    void setupChart();
    void setChartInfo(const PerformanceChartInfo &info);
    PerformanceChartInfo chartInfo() const { return m_chartinfo; }
    
    /// Loads context info into this view. Reimplement.
    virtual bool loadContext(const KoXmlElement &context);
    /// Save context info from this view. Reimplement.
    virtual void saveContext(QDomElement &context) const;

    /// Create a print job dialog
    KoPrintJob *createPrintJob(ViewBase *parent);

    void setNodes(const QList<Node*> &nodes);

public Q_SLOTS:
    void refreshChart();

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    
    void createBarChart();
    void createLineChart();
    void setEffortValuesVisible(bool visible);
    void setCostValuesVisible(bool visible);

protected Q_SLOTS:
    void slotUpdate();
    void slotLocaleChanged();

private:
    struct ChartContents {
        ~ChartContents() {
            delete dateaxis;
            delete effortaxis;
            delete costaxis;
            delete effortplane;
            delete costplane;
            delete effortdiagram;
            delete costdiagram;
        }
        ChartProxyModel costproxy;
        ChartProxyModel effortproxy;
    
        KChart::CartesianCoordinatePlane *effortplane;
        KChart::CartesianCoordinatePlane *costplane;
        KChart::AbstractDiagram *effortdiagram;
        KChart::AbstractDiagram *costdiagram;
        KChart::CartesianAxis *effortaxis;
        KChart::CartesianAxis *costaxis;
        KChart::CartesianAxis *dateaxis;

        ChartProxyModel piproxy;
        KChart::CartesianCoordinatePlane *piplane;
        KChart::AbstractDiagram *pidiagram;
        KChart::CartesianAxis *piaxis;
    };
    void setupChart(ChartContents &cc);

private:
    Project *m_project;
    ScheduleManager *m_manager;
    PerformanceChartInfo m_chartinfo;

    ChartItemModel m_chartmodel;
    KChart::Legend *m_legend;
    KChart::BarDiagram m_legenddiagram;
    struct ChartContents m_barchart;
    struct ChartContents m_linechart;
};

//--------------------------------------
class PerformanceStatusViewSettingsPanel : public QWidget, public Ui::PerformanceStatusViewSettingsPanel
{
    Q_OBJECT
public:
    explicit PerformanceStatusViewSettingsPanel(PerformanceStatusBase *view, QWidget *parent = 0);
    
public Q_SLOTS:
    void slotOk();
    void setDefault();
    
Q_SIGNALS:
    void changed();
    
protected Q_SLOTS:
    void switchStackWidget();
    
private:
    PerformanceStatusBase *m_view;
};

} //namespace KPlato


#endif
