/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "GanttView.h"
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
        setXMLFile("Portfolio_GanttViewUi.rc");
    } else {
        setXMLFile("Portfolio_GanttViewUi_readonly.rc");
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


KoPrintJob *GanttView::createPrintJob()
{
    return nullptr;
}
