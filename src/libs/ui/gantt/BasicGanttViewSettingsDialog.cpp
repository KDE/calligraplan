/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <BasicGanttViewSettingsDialog.h>

#include "kptviewbase.h"
#include "GanttViewBase.h"
#include <kptdebug.h>

#include <QWidget>
#include <QTabWidget>

#include <KoPageLayoutWidget.h>

using namespace KPlato;

BasicGanttViewSettingsDialog::BasicGanttViewSettingsDialog(GanttViewBase *gantt, ViewBase *view, bool selectPrint)
    : ItemViewSettupDialog(view, gantt->treeView(), true, view)
    , m_gantt(gantt)
{
    QTabWidget *tab = new QTabWidget();
    QWidget *w = ViewBase::createPageLayoutWidget(view);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);

    m_printingoptions = new GanttPrintingOptionsWidget(gantt, this);
    tab->addTab(m_printingoptions, m_printingoptions->windowTitle());
    KPageWidgetItem *page = insertWidget(-1, tab, i18n("Printing"), i18n("Printing Options"));
    if (selectPrint) {
        setCurrentPage(page);
    }
    connect(this, SIGNAL(accepted()), this, SLOT(slotOk()));
}

void BasicGanttViewSettingsDialog::slotOk()
{
    debugPlan;
    m_gantt->setPrintingOptions(m_printingoptions->options());
    ItemViewSettupDialog::slotOk();
}
