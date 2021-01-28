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
#include "DetailsView.h"
#include "Factory.h"
#include "MainDocument.h"
#include "ProjectsModel.h"

#include <kptproject.h>
#include <kpttaskstatusview.h>
#include <kptnodeitemmodel.h>

#include <KoComponentData.h>
#include <KoPart.h>

#include <KRecentFilesAction>

#include <QTreeView>
#include <QVBoxLayout>
#include <QItemSelectionModel>
#include <QSplitter>

DetailsView::DetailsView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    if (doc && doc->isReadWrite()) {
        setXMLFile("DetailsViewUi.rc");
    } else {
        setXMLFile("DetailsViewUi_readonly.rc");
    }
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    QSplitter *sp = new QSplitter(this);
    layout->addWidget(sp);
    m_view = new QTreeView(sp);
    m_view->setRootIsDecorated(false);
    sp->addWidget(m_view);
    ProjectsFilterModel *m = new ProjectsFilterModel(sp);
    m->setPortfolio(qobject_cast<MainDocument*>(doc));
    m->setAcceptedColumns(QList<int>() << KPlato::NodeModel::NodeName);
    m_view->setModel(m);

    m_detailsView = new KPlato::TaskStatusView(part, doc, sp);
    const QList<int> show = QList<int>()
    << KPlato::NodeModel::NodeName
    << KPlato::NodeModel::NodeStatus
    << KPlato::NodeModel::NodeCompleted
    << KPlato::NodeModel::NodeActualEffort
    << KPlato::NodeModel::NodeRemainingEffort
    << KPlato::NodeModel::NodePlannedEffort
    << KPlato::NodeModel::NodePlannedCost
    << KPlato::NodeModel::NodeActualCost
    << KPlato::NodeModel::NodeActualStart
    << KPlato::NodeModel::NodeActualFinish
    << KPlato::NodeModel::NodeDescription;
    m_detailsView->showColumns(show);
    m_detailsView->setViewSplitMode(false);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DetailsView::selectionChanged);

    sp->setStretchFactor(1, 10);
}

DetailsView::~DetailsView()
{
}

void DetailsView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * DetailsView::popupMenu(const QString& name)
{
    return nullptr;
}

QPrintDialog *DetailsView::createPrintDialog(KoPrintJob *printJob, QWidget *parent)
{
    return nullptr;
}

void DetailsView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)
    QModelIndexList indexes = m_view->selectionModel()->selectedRows();
    const ProjectsFilterModel *m = qobject_cast<ProjectsFilterModel*>(m_view->model());
    Q_ASSERT(m);
    KoDocument *doc = m->documentFromIndex(indexes.value(0));
    m_detailsView->setProject(doc->project());
    m_detailsView->setScheduleManager(m->portfolio()->scheduleManager(doc));
}

