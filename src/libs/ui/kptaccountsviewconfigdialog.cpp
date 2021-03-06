/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptaccountsviewconfigdialog.h"

#include "kptaccountsview.h"
#include "kptaccountsmodel.h"
#include "kptviewbase.h"
#include "kptdebug.h"

#include "KoPageLayoutWidget.h"

#include <KLocalizedString>

#include <QPushButton>
#include <QCheckBox>
#include <QString>


namespace KPlato
{

AccountsviewConfigDialog::AccountsviewConfigDialog(ViewBase *view, AccountsTreeView *treeview, QWidget *p, bool selectPrint)
    : KPageDialog(p),
    m_view(view),
    m_treeview(treeview)
{
    setWindowTitle(i18n("Settings"));
    m_panel = new AccountsviewConfigPanel(this);
    switch (treeview->startMode()) {
        case CostBreakdownItemModel::StartMode_Project: 
            m_panel->ui_projectstartBtn->setChecked(true);
            m_panel->ui_startdate->setEnabled(false);
            break;
        case CostBreakdownItemModel::StartMode_Date:
            m_panel->ui_startdateBtn->setChecked(true);
            break;
    }
    switch (treeview->endMode()) {
        case CostBreakdownItemModel::EndMode_Project:
            m_panel->ui_projectendBtn->setChecked(true);
            m_panel->ui_enddate->setEnabled(false);
            break;
        case CostBreakdownItemModel::EndMode_Date:
            m_panel->ui_enddateBtn->setChecked(true);
            break;
        case CostBreakdownItemModel::EndMode_CurrentDate:
            m_panel->ui_currentdateBtn->setChecked(true);
            m_panel->ui_enddate->setEnabled(false);
            break;
    }
    m_panel->ui_startdate->setDate(treeview->startDate());
    m_panel->ui_enddate->setDate(treeview->endDate());
    m_panel->ui_periodBox->setCurrentIndex(treeview->periodType());
    m_panel->ui_cumulative->setChecked(treeview->cumulative());
    m_panel->ui_showBox->setCurrentIndex(treeview->showMode());

    KPageWidgetItem *page = addPage(m_panel, i18n("General"));
    page->setHeader(i18n("View Settings"));

    QTabWidget *tab = new QTabWidget();

    QWidget *w = ViewBase::createPageLayoutWidget(view);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);

    m_headerfooter = ViewBase::createHeaderFooterWidget(view);
    m_headerfooter->setOptions(view->printingOptions());
    tab->addTab(m_headerfooter, m_headerfooter->windowTitle());

    page = addPage(tab, i18n("Printing"));
    page->setHeader(i18n("Printing Options"));
    if (selectPrint) {
        setCurrentPage(page);
    }
    connect(this, &QDialog::accepted, this, &AccountsviewConfigDialog::slotOk);

    connect(m_panel, &AccountsviewConfigPanel::changed, this, &AccountsviewConfigDialog::enableOkButton);
}

void AccountsviewConfigDialog::enableOkButton(bool enabled)
{
    button(QDialogButtonBox::Ok)->setEnabled(enabled);
}


void AccountsviewConfigDialog::slotOk()
{
    debugPlan;
    m_treeview->setPeriodType(m_panel->ui_periodBox->currentIndex());
    m_treeview->setCumulative(m_panel->ui_cumulative->isChecked());
    m_treeview->setShowMode(m_panel->ui_showBox->currentIndex());
    if (m_panel->ui_startdateBtn->isChecked()) {
        m_treeview->setStartDate(m_panel->ui_startdate->date());
        m_treeview->setStartMode(CostBreakdownItemModel::StartMode_Date);
    } else {
        m_treeview->setStartMode(CostBreakdownItemModel::StartMode_Project);
    }

    if (m_panel->ui_enddateBtn->isChecked()) {
        m_treeview->setEndDate(m_panel->ui_enddate->date());
        m_treeview->setEndMode(CostBreakdownItemModel::EndMode_Date);
    } else if (m_panel->ui_currentdateBtn->isChecked()) {
        m_treeview->setEndMode(CostBreakdownItemModel::EndMode_CurrentDate);
    } else {
        m_treeview->setEndMode(CostBreakdownItemModel::EndMode_Project);
    }

    m_view->setPageLayout(m_pagelayout->pageLayout());
    m_view->setPrintingOptions(m_headerfooter->options());
}


//----------------------------
AccountsviewConfigPanel::AccountsviewConfigPanel(QWidget *parent)
    : AccountsviewConfigurePanelBase(parent)
{

    connect(ui_startdate, &QDateTimeEdit::dateChanged, this, &AccountsviewConfigPanel::slotChanged);
    connect(ui_enddate, &QDateTimeEdit::dateChanged, this, &AccountsviewConfigPanel::slotChanged);
    connect(ui_periodBox, SIGNAL(activated(int)), SLOT(slotChanged()));
    connect(ui_cumulative, &QAbstractButton::clicked, this, &AccountsviewConfigPanel::slotChanged);

    connect(ui_projectstartBtn, &QAbstractButton::clicked, this, &AccountsviewConfigPanel::slotChanged);
    connect(ui_startdateBtn, &QAbstractButton::clicked, this, &AccountsviewConfigPanel::slotChanged);
    connect(ui_projectendBtn, &QAbstractButton::clicked, this, &AccountsviewConfigPanel::slotChanged);
    connect(ui_currentdateBtn, &QAbstractButton::clicked, this, &AccountsviewConfigPanel::slotChanged);
    connect(ui_enddateBtn, &QAbstractButton::clicked, this, &AccountsviewConfigPanel::slotChanged);
    connect(ui_showBox, SIGNAL(activated(int)), SLOT(slotChanged()));
    
    connect(ui_startdateBtn, &QAbstractButton::toggled, ui_startdate, &QWidget::setEnabled);
    connect(ui_enddateBtn, &QAbstractButton::toggled, ui_enddate, &QWidget::setEnabled);
}

void AccountsviewConfigPanel::slotChanged() {
    Q_EMIT changed(true);
}


}  //KPlato namespace
