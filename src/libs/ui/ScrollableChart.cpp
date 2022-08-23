/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ScrollableChart.h"

#include <KChartAbstractCoordinatePlane>
#include <KChartBarDiagram>

#include <kptdebug.h>

using namespace KPlato;

ScrollableChart::ScrollableChart(QWidget *parent)
    : QWidget(parent)
    , m_scrollBarPolicy(Qt::ScrollBarAsNeeded)
    , m_diagramFlavor(0)
{
    ui.setupUi(this);

    auto bar = new KChart::BarDiagram();
    auto axis = new KChart::CartesianAxis();
    axis->setPosition(KChart::CartesianAxis::Bottom);
    bar->addAxis(axis);
    axis = new KChart::CartesianAxis();
    axis->setPosition(KChart::CartesianAxis::Left);
    bar->addAxis(axis);
    bar->useSubduedColors();
    bar->setModel(&m_model);
    ui.chart->coordinatePlane()->addDiagram(bar);
    ui.legend->setDiagram(bar);

    connect(ui.scrollBar, &QScrollBar::valueChanged, &m_model, &ScrollableChartFilterModel::setStart);
    connect(&m_model, &QAbstractItemModel::modelReset, this, &ScrollableChart::updateAxisRange);
}

ScrollableChart::~ScrollableChart()
{
}

void ScrollableChart::addDiagram(KChart::AbstractCartesianDiagram *diagram)
{
    ui.chart->coordinatePlane()->addDiagram(diagram);
}


KChart::Chart *ScrollableChart::chart() const
{
    return ui.chart;
}

KChart::Legend *ScrollableChart::legend() const
{
    return ui.legend;
}

ScrollableChartFilterModel &ScrollableChart::filterModel()
{
    return m_model;
}

void ScrollableChart::setDataModel(QAbstractItemModel *model)
{
    m_model.setSourceModel(model);
    updateAxisRange();
}

void ScrollableChart::setScrollBarPolicy(Qt::ScrollBarPolicy policy)
{
    m_scrollBarPolicy = policy;
    switch(policy) {
        case Qt::ScrollBarAlwaysOff:
            ui.scrollBar->hide();
            break;
        case Qt::ScrollBarAlwaysOn:
            ui.scrollBar->show();
            break;
        case Qt::ScrollBarAsNeeded:
            ui.scrollBar->setVisible(ui.scrollBar->maximum() > 0);
            break;
    }
}

void ScrollableChart::setStart(int row)
{
    debugPlanChart<<row;
    m_model.setStart(row);
}

void ScrollableChart::setNumRows(int numRows)
{
    const int rowCount = m_model.sourceModel()->rowCount();
    const int value = numRows ? numRows : rowCount;
    debugPlanChart<<"numRows:"<<numRows<<"rowCount:"<<rowCount;
    ui.scrollBar->setRange(0,  rowCount - value);
    ui.scrollBar->setPageStep(value);
    if (m_scrollBarPolicy == Qt::ScrollBarAsNeeded) {
        ui.scrollBar->setVisible(ui.scrollBar->maximum() > 0);
    }
    m_model.setNumRows(numRows);
}

void KPlato::ScrollableChart::setDiagramFlavor(int flavor)
{
    auto bar = qobject_cast<KChart::BarDiagram*>(ui.chart->coordinatePlane()->diagram());
    if (bar) {
        switch(flavor) {
            case 0: // Normal
                bar->setType(KChart::BarDiagram::Normal);
                m_diagramFlavor = flavor;
                updateAxisRange();
                break;
            case 1: // Stacked
                bar->setType(KChart::BarDiagram::Stacked);
                m_diagramFlavor = flavor;
                updateAxisRange();
                break;
            case 2: // Percent
                bar->setType(KChart::BarDiagram::Percent);
                m_diagramFlavor = flavor;
                updateAxisRange();
                break;
            default:
                m_diagramFlavor = -1;
                break;
        }
        return;
    }

}

void KPlato::ScrollableChart::updateAxisRange()
{
    auto plane = qobject_cast<KChart::CartesianCoordinatePlane*>(ui.chart->coordinatePlane());
    if (!plane) {
        warnPlanChart<<"Coordinate plane is not cartesian"<<ui.chart->coordinatePlane();
        return;
    }
    QPair<qreal, qreal> range(0.0, 0.0);
    auto v = m_model.headerData(0, Qt::Vertical, AXISRANGEROLE);
    if (v.isValid()) {
        auto lst = v.toList();
        switch(m_diagramFlavor) {
            case 0: // Normal
                range.first = lst.value(0).toDouble();
                range.second = lst.value(1).toDouble();
                break;
            case 1: // Stacked
                range.first = lst.value(2).toDouble();
                range.second = lst.value(3).toDouble();
                break;
            case 2: // Percent
                break;
            default:
                break;
        }
    }
    plane->setVerticalRange(range);
    plane->layoutPlanes();
}
