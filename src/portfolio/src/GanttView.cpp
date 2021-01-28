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

#include <gantt/GanttViewBase.h>

#include <KoApplication.h>
#include <KoComponentData.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>
#include <KoFileDialog.h>

#include <KRecentFilesAction>
#include <KActionCollection>

#include <QTreeView>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QHeaderView>

GanttView::GanttView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    if (doc && doc->isReadWrite()) {
        setXMLFile("GanttViewUi.rc");
    } else {
        setXMLFile("GanttViewUi_readonly.rc");
    }
    setupGui();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    m_view = new KPlato::GanttViewBase(this);
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
    qInfo()<<Q_FUNC_INFO<<m_view->leftView();
    QTreeView *v = qobject_cast<QTreeView*>(m_view->leftView());
    v->header()->hideSection(1 /*Type*/);
}

GanttView::~GanttView()
{
}

void GanttView::setupGui()
{
}

void GanttView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * GanttView::popupMenu(const QString& name)
{
    return nullptr;
}

QPrintDialog *GanttView::createPrintDialog(KoPrintJob *printJob, QWidget *parent)
{
    return nullptr;
}
