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
{
    ui.setupUi(this);

    auto bar = new KChart::BarDiagram();
    auto axis = new KChart::CartesianAxis();
    axis->setPosition(KChart::CartesianAxis::Bottom);
    bar->addAxis(axis);
    axis = new KChart::CartesianAxis();
    axis->setPosition(KChart::CartesianAxis::Left);
    bar->addAxis(axis);

    bar->setModel(&m_model);
    ui.chart->coordinatePlane()->addDiagram(bar);
    ui.legend->setDiagram(bar);

    connect(ui.scrollBar, &QScrollBar::valueChanged, &m_model, &ScrollableChartFilterModel::setStart);
}

ScrollableChart::~ScrollableChart()
{
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
