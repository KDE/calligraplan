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
#include "SummaryView.h"
#include "Factory.h"
#include "SummaryModel.h"
#include "MainDocument.h"

#include <kptnodechartmodel.h>

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
#include <QHeaderView>

SummaryView::SummaryView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    if (doc && doc->isReadWrite()) {
        setXMLFile("SummaryViewUi.rc");
    } else {
        setXMLFile("SummaryViewUi_readonly.rc");
    }
    setupGui();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    m_view = new QTreeView(this);
    m_view->setRootIsDecorated(false);
    layout->addWidget(m_view);

    SummaryModel *model = new SummaryModel(m_view);
    model->setPortfolio(qobject_cast<MainDocument*>(doc));
    m_view->setModel(model);

    m_view->header()->moveSection(1, model->columnCount()-1); // Description
    const QList<int> hide = QList<int>()
    << KPlato::ChartItemModel::BCWSCost
    << KPlato::ChartItemModel::BCWPCost
    << KPlato::ChartItemModel::ACWPCost
    << KPlato::ChartItemModel::BCWSEffort
    << KPlato::ChartItemModel::BCWPEffort
    << KPlato::ChartItemModel::ACWPEffort;
    for (int c : hide) {
        m_view->header()->setSectionHidden(model->proxyColumnForExtraColumn(c), true);
    }
    m_view->setItemDelegateForColumn(2, new KPlato::EnumDelegate(m_view)); // Schedule
}

SummaryView::~SummaryView()
{
}

void SummaryView::setupGui()
{
}

void SummaryView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * SummaryView::popupMenu(const QString& name)
{
    return nullptr;
}

QPrintDialog *SummaryView::createPrintDialog(KoPrintJob *printJob, QWidget *parent)
{
    return nullptr;
}
