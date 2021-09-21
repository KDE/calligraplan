/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPLATO_SCROLLABLECHART_H
#define KPLATO_SCROLLABLECHART_H

#include "planui_export.h"

#include "ui_ScrollableChart.h"
#include "ScrollableChartFilterModel.h"

#include <KChartChart>

class QAbstractItemModel;

namespace KChart {
    class Chart;
    class Legend;
    class AbstractCartesianDiagram;
}

namespace KPlato
{

class PLANUI_EXPORT ScrollableChart : public QWidget
{
    Q_OBJECT

public:
    explicit ScrollableChart(QWidget *parent = nullptr);
    ~ScrollableChart() override;

    KChart::Chart *chart() const;
    KChart::Legend *legend() const;

    void addDiagram(KChart::AbstractCartesianDiagram *diagram);

    ScrollableChartFilterModel &filterModel();

    void setDataModel(QAbstractItemModel *model);

    void setScrollBarPolicy(Qt::ScrollBarPolicy policy);

public Q_SLOTS:
    /// Set first row to be shown to @p row
    void setStart(int row);
    /// Set number of rows to be shown to @p numRows
    /// If @p numRows is 0, all rows are shown
    void setNumRows(int numRows);
    /// Set diagram flavor to @p flavor (Normal = 0, Stacked = 0, Percent = 2)
    void setDiagramFlavor(int flavor);

private Q_SLOTS:
    void updateAxisRange();

private:
    Ui_ScrollableChart ui;
    ScrollableChartFilterModel m_model;
    Qt::ScrollBarPolicy m_scrollBarPolicy;
    int m_diagramFlavor;
};

} // namespace KPlato
#endif
