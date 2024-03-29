/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/


// clazy:excludeall=qstring-arg
#include "kptitemviewsettup.h"
#include "kptitemmodelbase.h"
#include "kptviewbase.h"
#include "kptdebug.h"

#include "KoPageLayoutWidget.h"

#include <QHeaderView>
#include <QPushButton>

namespace KPlato
{

ItemViewSettup::Item::Item(int column, const QString &text)
    : QListWidgetItem(text),
    m_column(column)
{
}

int ItemViewSettup::Item::column() const
{
    return m_column;
}

bool ItemViewSettup::Item::operator<(const QListWidgetItem & other) const
{
    return m_column < static_cast<const Item&>(other).column();
}

//--------------------------
ItemViewSettup::ItemViewSettup(TreeViewBase *view, bool includeColumn0, QWidget *parent)
    : QWidget(parent),
    m_view(view),
    m_includeColumn0(includeColumn0)
{
    setupUi(this);
    
    stretchLastSection->setChecked(view->header()->stretchLastSection());
    
    QAbstractItemModel *model = view->model();

    QMap<int, Item*> map;
    int c = includeColumn0 ? 0 : 1;
    debugPlan<<includeColumn0<<c;
    for (; c < model->columnCount(); ++c) {
        Item *item = new Item(c, model->headerData(c, Qt::Horizontal).toString());
        item->setToolTip(model->headerData(c, Qt::Horizontal, Qt::ToolTipRole).toString());
        if (view->isColumnHidden(c)) {
            selector->availableListWidget()->addItem(item);
        } else {
            map.insert(view->section(c), item);
        }
    }
    const QList<Item*> items = map.values();
    for(Item *i : items) {
        selector->selectedListWidget()->addItem(i);
    }

    connect(stretchLastSection, &QCheckBox::stateChanged, this, &ItemViewSettup::slotChanged);
    
    connect(selector, &KActionSelector::added, this, &ItemViewSettup::slotChanged);
    connect(selector, &KActionSelector::removed, this, &ItemViewSettup::slotChanged);
    connect(selector, &KActionSelector::movedUp, this, &ItemViewSettup::slotChanged);
    connect(selector, &KActionSelector::movedDown, this, &ItemViewSettup::slotChanged);

}

void ItemViewSettup::slotChanged()
{
    Q_EMIT enableButtonOk(true);
}

void ItemViewSettup::slotOk()
{
    debugPlan;
    QListWidget *lst = selector->availableListWidget();
    for (int r = 0; r < lst->count(); ++r) {
        int c = static_cast<Item*>(lst->item(r))->column();
        m_view->hideColumn(c);
    }
    lst = selector->selectedListWidget();
    for (int r = 0; r < lst->count(); ++r) {
        int c = static_cast<Item*>(lst->item(r))->column();
        m_view->mapToSection(c, r);
        m_view->showColumn(c);
    }
    m_view->setStretchLastSection(stretchLastSection->isChecked());
}

void ItemViewSettup::setDefault()
{
    debugPlan;
    selector->availableListWidget()->clear();
    selector->selectedListWidget()->clear();
    QAbstractItemModel *model = m_view->model();
    int c = m_includeColumn0 ? 0 : 1;
    const QList<int> def = m_view->defaultColumns();
    for (; c < model->columnCount(); ++c) {
        if (! def.contains(c)) {
            Item *item = new Item(c, model->headerData(c, Qt::Horizontal).toString());
            item->setToolTip(model->headerData(c, Qt::Horizontal, Qt::ToolTipRole).toString());
            selector->availableListWidget()->addItem(item);
        }
    }
    for (int i : def) {
        Item *item = new Item(i, model->headerData(i, Qt::Horizontal).toString());
        item->setToolTip(model->headerData(i, Qt::Horizontal, Qt::ToolTipRole).toString());
        selector->selectedListWidget()->addItem(item);
    }
}


//---------------------------
ItemViewSettupDialog::ItemViewSettupDialog(ViewBase *view, TreeViewBase *treeview, bool includeColumn0, QWidget *parent)
    : KPageDialog(parent),
    m_view(view),
    m_treeview(treeview),
    m_pagelayout(nullptr),
    m_headerfooter(nullptr)
{
    setWindowTitle(i18n("View Settings"));
    setFaceType(KPageDialog::Plain); // only one page, KPageDialog will use margins
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
    button(QDialogButtonBox::Ok)->setDefault(true);

    button(QDialogButtonBox::RestoreDefaults)->setEnabled(! treeview->defaultColumns().isEmpty());

    m_panel = new ItemViewSettup(treeview, includeColumn0);
    KPageWidgetItem *page = new KPageWidgetItem(m_panel, i18n("Tree View"));
    page->setHeader(i18n("Tree View Column Configuration"));
    addPage(page);
    m_pageList.append(page);

    connect(this, &QDialog::accepted, this, &ItemViewSettupDialog::slotOk);
    connect(this, &QDialog::accepted, m_panel, &ItemViewSettup::slotOk);
    connect(button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, m_panel, &ItemViewSettup::setDefault);
}

void ItemViewSettupDialog::slotOk()
{
    debugPlan<<m_view<<m_pagelayout<<m_headerfooter;
    if (! m_view) {
        return;
    }
    if (m_pagelayout) {
        m_view->setPageLayout(m_pagelayout->pageLayout());
    }
    if (m_headerfooter) {
        m_view->setPrintingOptions(m_headerfooter->options());
    }
}

KPageWidgetItem *ItemViewSettupDialog::insertWidget(int index, QWidget *widget, const QString &name, const QString &header)
{
    KPageWidgetItem *before = m_pageList.value(index);
    KPageWidgetItem *page = new KPageWidgetItem(widget, name);
    page->setHeader(header);
    if (before) {
        insertPage(before, page);
        m_pageList.insert(index, page);
    } else {
        addPage(page);
        m_pageList.append(page);
    }
    return page;
}

void ItemViewSettupDialog::addPrintingOptions(bool setAsCurrent)
{
    if (! m_view) {
        return;
    }
    QTabWidget *tab = new QTabWidget();
    QWidget *w = ViewBase::createPageLayoutWidget(m_view);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);

    m_headerfooter = ViewBase::createHeaderFooterWidget(m_view);
    tab->addTab(m_headerfooter, m_headerfooter->windowTitle());

    KPageWidgetItem *itm = insertWidget(-1, tab, i18n("Printing"), i18n("Printing Options"));
    if (setAsCurrent) {
        setCurrentPage(itm);
    }
}

//-------------------------------
SplitItemViewSettupDialog::SplitItemViewSettupDialog(ViewBase *view, DoubleTreeViewBase *treeview, QWidget *parent)
    : KPageDialog(parent),
    m_view(view),
    m_treeview(treeview),
    m_pagelayout(nullptr),
    m_headerfooter(nullptr)
{
    setWindowTitle(i18n("View Settings"));
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
    button(QDialogButtonBox::Ok)->setDefault(true);

    bool nodef = treeview->masterView()->defaultColumns().isEmpty() || treeview->slaveView()->defaultColumns().isEmpty();
    button(QDialogButtonBox::Ok)->setEnabled(! nodef);

    m_page1 = new ItemViewSettup(treeview->masterView(), true);
    KPageWidgetItem *page = new KPageWidgetItem(m_page1, i18n("Main View"));
    page->setHeader(i18n("Main View Column Configuration"));
    addPage(page);
    m_pageList.append(page);

    m_page2 = new ItemViewSettup(treeview->slaveView(), true);
    page = new KPageWidgetItem(m_page2, i18n("Auxiliary View"));
    page->setHeader(i18n("Auxiliary View Column Configuration"));
    addPage(page);
    m_pageList.append(page);

    //connect(m_page1, SIGNAL(enableButtonOk(bool)), this, SLOT(enableButtonOk(bool)));
    //connect(m_page2, SIGNAL(enableButtonOk(bool)), this, SLOT(enableButtonOk(bool)));

    connect(this, &QDialog::accepted, this, &SplitItemViewSettupDialog::slotOk);
    connect(this, &QDialog::accepted, m_page1, &ItemViewSettup::slotOk);
    connect(this, &QDialog::accepted, m_page2, &ItemViewSettup::slotOk);
    connect(button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, m_page1, &ItemViewSettup::setDefault);
    connect(button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, m_page2, &ItemViewSettup::setDefault);
}

void SplitItemViewSettupDialog::slotOk()
{
    debugPlan;
    if (! m_view) {
        return;
    }
    m_view->setPageLayout(m_pagelayout->pageLayout());
    m_view->setPrintingOptions(m_headerfooter->options());
}

KPageWidgetItem *SplitItemViewSettupDialog::insertWidget(int index, QWidget *widget, const QString &name, const QString &header)
{
    KPageWidgetItem *before = m_pageList.value(index);
    KPageWidgetItem *page = new KPageWidgetItem(widget, name);
    page->setHeader(header);
    if (before) {
        insertPage(before, page);
        m_pageList.insert(index, page);
    } else {
        addPage(page);
        m_pageList.append(page);
    }
    return page;
}

void SplitItemViewSettupDialog::addPrintingOptions(bool setAsCurrent)
{
    if (! m_view) {
        return;
    }
    QTabWidget *tab = new QTabWidget();
    QWidget *w = ViewBase::createPageLayoutWidget(m_view);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);
    m_pagelayout->setPageLayout(m_view->pageLayout());

    m_headerfooter = ViewBase::createHeaderFooterWidget(m_view);
    tab->addTab(m_headerfooter, m_headerfooter->windowTitle());
    m_headerfooter->setOptions(m_view->printingOptions());

    KPageWidgetItem *itm = insertWidget(-1, tab, i18n("Printing"), i18n("Printing Options"));
    if (setAsCurrent) {
        setCurrentPage(itm);
    }
}

} //namespace KPlato
