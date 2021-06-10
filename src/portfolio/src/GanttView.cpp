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

// clazy:excludeall=qstring-arg
#include "GanttView.h"
#include "Factory.h"
#include "GanttModel.h"
#include "MainDocument.h"

#include <kptnodechartmodel.h>
#include <KGanttProxyModel>
#include <KGanttTreeViewRowController>

#include <gantt/GanttViewBase.h>

#include <KoApplication.h>
#include <KoComponentData.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>
#include <KoFileDialog.h>

#include <KRecentFilesAction>
#include <KActionCollection>
#include <KXMLGUIFactory>

#include <QTreeView>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QMenu>
#include <QDomDocument>

GanttView::GanttView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    if (doc && doc->isReadWrite()) {
        setXMLFile("PortfolioGanttViewUi.rc");
    } else {
        setXMLFile("PortfolioGanttViewUi_readonly.rc");
    }
    setupGui();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    m_view = new KPlato::GanttViewBase(this);
    auto tv = new KPlato::GanttTreeView(m_view);
    tv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    tv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tv->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); // needed since qt 4.2
    m_view->setLeftView(tv);
    auto rowController = new KGantt::TreeViewRowController(tv, m_view->ganttProxyModel());
    m_view->setRowController(rowController);
    tv->header()->setStretchLastSection(true);
    layout->addWidget(m_view);

    KGantt::ProxyModel *gm = static_cast<KGantt::ProxyModel*>(m_view->ganttProxyModel());
    gm->setRole(KGantt::StartTimeRole, Qt::EditRole); // To provide correct format
    gm->setRole(KGantt::EndTimeRole, Qt::EditRole); // To provide correct format
    gm->setColumn(KGantt::ItemTypeRole, 1);
    gm->setColumn(KGantt::StartTimeRole, 2);
    gm->setColumn(KGantt::EndTimeRole, 3);
    gm->setColumn(KGantt::TaskCompletionRole, 4);

    GanttModel *m = new GanttModel(m_view);
    m->setPortfolio(qobject_cast<MainDocument*>(doc));
    m_view->setModel(m);

    tv->header()->hideSection(1 /*Type*/);
    tv->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tv, &QTreeView::customContextMenuRequested, this, &GanttView::slotCustomContextMenuRequested);
}

GanttView::~GanttView()
{
}

void GanttView::setupGui()
{
    auto a = new QAction(koIcon("view-time-schedule-calculus"), i18n("Open Project"), this);
    actionCollection()->addAction("gantt_open_project", a);
    connect(a, &QAction::triggered, this, &GanttView::openProject);
}

void GanttView::openProject()
{
    QModelIndex idx = m_view->leftView()->selectionModel()->selectedRows().value(0);
    KoDocument *doc = idx.data(DOCUMENT_ROLE).value<KoDocument*>();
    Q_EMIT openDocument(doc);
}

void GanttView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu *GanttView::popupMenu(const QString& name)
{
    return nullptr;
}

void GanttView::slotCustomContextMenuRequested(const QPoint &pos)
{
    auto menu = qobject_cast<QMenu*>(factory()->container("gantt_context_menu", this));
    if (menu && !menu->isEmpty()) {
        menu->exec(m_view->leftView()->mapToGlobal(pos)); // FIXME: mapping incorrect
    }
}


QPrintDialog *GanttView::createPrintDialog(KoPrintJob *printJob, QWidget *parent)
{
    return nullptr;
}
