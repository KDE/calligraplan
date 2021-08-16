/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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

    KoPrintJob *createPrintJob() override;

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
