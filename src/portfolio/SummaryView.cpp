/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "SummaryView.h"
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
        setXMLFile(QStringLiteral("Portfolio_SummaryViewUi.rc"));
    } else {
        setXMLFile(QStringLiteral("Portfolio_SummaryViewUi_readonly.rc"));
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

KoPrintJob *SummaryView::createPrintJob()
{
    return nullptr;
}
