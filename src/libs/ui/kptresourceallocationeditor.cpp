/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009, 2010, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptresourceallocationeditor.h"

#include "kptresourceallocationmodel.h"
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdatetime.h"
#include "kptitemviewsettup.h"
#include "kptdebug.h"

#include <KoDocument.h>

#include <QList>
#include <QVBoxLayout>
#include <QAction>


namespace KPlato
{


ResourceAllocationTreeView::ResourceAllocationTreeView(QWidget *parent)
    : DoubleTreeViewBase(parent)
{
//    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    ResourceAllocationItemModel *m = new ResourceAllocationItemModel(this);
    setModel(m);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    createItemDelegates(m);

    connect(m, &QAbstractItemModel::dataChanged, this, &ResourceAllocationTreeView::dataChanged);
}

QObject *ResourceAllocationTreeView::currentObject() const
{
    return model()->object(selectionModel()->currentIndex());
}

//-----------------------------------
ResourceAllocationEditor::ResourceAllocationEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new ResourceAllocationTreeView(this);
    l->addWidget(m_view);
    setupGui();

    m_view->setEditTriggers(m_view->editTriggers() | QAbstractItemView::EditKeyPressed);

    QList<int> lst1; lst1 << 1 << -1;
    QList<int> lst2; lst2 << 0;
    m_view->hideColumns(lst1, lst2);

    m_view->masterView()->setDefaultColumns(QList<int>() << 0);
    QList<int> show;
    for (int c = 1; c < model()->columnCount(); ++c) {
        show << c;
    }
    m_view->slaveView()->setDefaultColumns(show);

    connect(model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);

    connect(m_view, &DoubleTreeViewBase::currentChanged, this, &ResourceAllocationEditor::slotCurrentChanged);

    connect(m_view, &DoubleTreeViewBase::selectionChanged, this, &ResourceAllocationEditor::slotSelectionChanged);

    connect(m_view, &DoubleTreeViewBase::contextMenuRequested, this, &ResourceAllocationEditor::slotContextMenuRequested);

    connect(m_view, &DoubleTreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);

}

void ResourceAllocationEditor::updateReadWrite(bool readwrite)
{
    m_view->setReadWrite(readwrite);
}

void ResourceAllocationEditor::setGuiActive(bool activate)
{
    debugPlan<<activate;
    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
    if (activate && !m_view->selectionModel()->currentIndex().isValid()) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }
}

void ResourceAllocationEditor::slotContextMenuRequested(const QModelIndex &index, const QPoint& pos)
{
    //debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
    QString name;
    if (index.isValid()) {
        QObject *obj = m_view->model()->object(index);
        Resource *r = qobject_cast<Resource*>(obj);
        if (r) {
            //name = "resourceeditor_resource_popup";
        }
    }
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
    } else {
        openPopupMenu(name, pos);
    }
}

Resource *ResourceAllocationEditor::currentResource() const
{
    return qobject_cast<Resource*>(m_view->currentObject());
}

void ResourceAllocationEditor::slotCurrentChanged(const QModelIndex &)
{
    //debugPlan<<curr.row()<<","<<curr.column();
//    slotEnableActions();
}

void ResourceAllocationEditor::slotSelectionChanged(const QModelIndexList&)
{
    //debugPlan<<list.count();
    updateActionsEnabled();
}

void ResourceAllocationEditor::slotEnableActions(bool on)
{
    updateActionsEnabled(on);
}

void ResourceAllocationEditor::updateActionsEnabled(bool /*on */)
{
}

void ResourceAllocationEditor::setupGui()
{
    // Add the context menu actions for the view options
    connect(m_view->actionSplitView(), &QAction::triggered, this, &ResourceAllocationEditor::slotSplitView);
    addContextAction(m_view->actionSplitView());

    createOptionActions(ViewBase::OptionAll);
}

void ResourceAllocationEditor::slotSplitView()
{
    debugPlan;
    m_view->setViewSplitMode(! m_view->isViewSplit());
    Q_EMIT optionsModified();
}

void ResourceAllocationEditor::slotOptions()
{
    debugPlan;
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog(this, m_view, this);
    dlg->addPrintingOptions(sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}


bool ResourceAllocationEditor::loadContext(const KoXmlElement &context)
{
    debugPlan<<objectName();
    ViewBase::loadContext(context);
    return m_view->loadContext(model()->columnMap(), context);
}

void ResourceAllocationEditor::saveContext(QDomElement &context) const
{
    debugPlan<<objectName();
    ViewBase::saveContext(context);
    m_view->saveContext(model()->columnMap(), context);
}

KoPrintJob *ResourceAllocationEditor::createPrintJob()
{
    return m_view->createPrintJob(this);
}


} // namespace KPlato
