/* This file is part of the KDE project
 * Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
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

#ifndef PLANPORTFOLIO_PERFORMANCEVIEW_H
#define PLANPORTFOLIO_PERFORMANCEVIEW_H

#include "planportfolio_export.h"

#include "ui_PerformanceView.h"
#include <KoView.h>
#include <KoPageLayout.h>

class KoDocument;
class KoPrintJob;

class QMenu;
class QItemSelection;


namespace KPlato {
    class ChartItemModel;
}
namespace KChart {
    class CartesianAxis;
}

class PLANPORTFOLIO_EXPORT PerformanceView : public KoView
{
    Q_OBJECT

public:
    explicit PerformanceView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~PerformanceView() override;

    QMenu *popupMenu(const QString& name);

    QPrintDialog* createPrintDialog(KoPrintJob*, QWidget*) override;

public Q_SLOTS:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

protected:
    void updateReadWrite(bool readwrite) override;

    void setupCharts();

private:
    bool m_readWrite;
    Ui::PerformanceView ui;
    KPlato::ChartItemModel *m_chartModel;
    QList<KChart::CartesianAxis*> m_yAxes;
};

#endif
