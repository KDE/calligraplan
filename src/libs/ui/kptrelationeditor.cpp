/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007-2010, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptrelationeditor.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kptitemviewsettup.h"
#include "kptdebug.h"

#include <KoDocument.h>

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>


namespace KPlato
{

//--------------------
RelationTreeView::RelationTreeView(QWidget *parent)
    : DoubleTreeViewBase(parent)
{
    setViewSplitMode(false);
    RelationItemModel *m = new RelationItemModel(this);
    setModel(m);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setArrowKeyNavigation(true);
    setRootIsDecorated (false);

    createItemDelegates(m);

    //HACK to simulate SingleSelection *and* get indication of current item
    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &RelationTreeView::slotCurrentChanged);
}

void RelationTreeView::slotCurrentChanged(const QModelIndex &curr, const QModelIndex&)
{
    selectionModel()->select(curr, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
}

//-----------------------------------
RelationEditor::RelationEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    debugPlan<<"----------------- Create RelationEditor ----------------------";

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new RelationTreeView(this);
    l->addWidget(m_view);
    //debugPlan<<m_view->actionSplitView();
    setupGui();

    connect(m_view, &DoubleTreeViewBase::currentChanged, this, &RelationEditor::slotCurrentChanged);

    connect(m_view, &DoubleTreeViewBase::selectionChanged, this, &RelationEditor::slotSelectionChanged);

    connect(m_view, &DoubleTreeViewBase::contextMenuRequested, this, &RelationEditor::slotContextMenuRequested);

    connect(m_view, SIGNAL(headerContextMenuRequested(QPoint)), SLOT(slotHeaderContextMenuRequested(QPoint)));

    connect(model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);
}

void RelationEditor::updateReadWrite(bool rw)
{
    m_view->setReadWrite(rw);
}

void RelationEditor::draw(Project &project)
{
    m_view->setProject(&project);
}

void RelationEditor::draw()
{
}

void RelationEditor::setGuiActive(bool /*activate */)
{
}

void RelationEditor::slotCurrentChanged(const QModelIndex &/*curr*/, const QModelIndex &)
{
    //debugPlan<<curr.row()<<","<<curr.column();
    slotEnableActions();
}

void RelationEditor::slotSelectionChanged(const QModelIndexList& /*list*/)
{
    //debugPlan<<list.count();
    slotEnableActions();
}

Relation *RelationEditor::currentRelation() const
{
    //debugPlan;
    return m_view->currentRelation();
}

void RelationEditor::slotContextMenuRequested(const QModelIndex& index, const QPoint& pos)
{
    Relation *rel = m_view->model()->relation(index);
    if (rel == nullptr) {
        slotHeaderContextMenuRequested(pos);
        return;
    }
    QString name = QStringLiteral("relation_popup");
    Q_EMIT requestPopupMenu(name, pos);
}

void RelationEditor::slotHeaderContextMenuRequested(const QPoint &pos)
{
    debugPlan;
    QList<QAction*> lst = contextActionList();
    if (! lst.isEmpty()) {
        QMenu::exec(lst, pos,  lst.first());
    }
}

void RelationEditor::slotEnableActions()
{
    updateActionsEnabled(true);
}

void RelationEditor::updateActionsEnabled(bool /*on */)
{
}

void RelationEditor::setupGui()
{
    // Add the context menu actions for the view options
    connect(m_view->actionSplitView(), &QAction::triggered, this, &RelationEditor::slotSplitView);
    addContextAction(m_view->actionSplitView());

    createOptionActions(ViewBase::OptionExpand | ViewBase::OptionCollapse | ViewBase::OptionViewConfig);
}

void RelationEditor::slotSplitView()
{
    //debugPlan;
    m_view->setViewSplitMode(! m_view->isViewSplit());
}


void RelationEditor::slotOptions()
{
    debugPlan;
    bool col0 = false;
    TreeViewBase *v = m_view->slaveView();
    if (v->isHidden()) {
        v = m_view->masterView();
        col0 = true;
    }
    ItemViewSettupDialog *dlg = new ItemViewSettupDialog(this, v, col0, this);
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void RelationEditor::slotAddRelation()
{
    debugPlan;
}

void RelationEditor::edit(const QModelIndex &i)
{
    if (i.isValid()) {
//        QModelIndex p = m_view->model()->parent(i);
//        m_view->setExpanded(p);
        m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        m_view->edit(i);
    }
}

void RelationEditor::slotDeleteRelation(Relation *r)
{
    Q_EMIT deleteRelation(r);
}

bool RelationEditor::loadContext(const KoXmlElement &context)
{
    debugPlan;
    ViewBase::loadContext(context);
    return m_view->loadContext(m_view->model()->columnMap(), context);
}

void RelationEditor::saveContext(QDomElement &context) const
{
    ViewBase::saveContext(context);
    m_view->saveContext(m_view->model()->columnMap(), context);
}

KoPrintJob *RelationEditor::createPrintJob()
{
    return m_view->createPrintJob(this);
}

} // namespace KPlato
