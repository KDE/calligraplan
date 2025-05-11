/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ProgressView.h"
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

ProgressView::ProgressView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    setXMLFile(QStringLiteral("Portfolio_ProgressViewUi.rc"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
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
    m_detailsView->setWhatsThis(QLatin1String()); // remove default text
    insertChildClient(m_detailsView);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ProgressView::selectionChanged);

    sp->setStretchFactor(1, 10);

    setWhatsThis(xi18nc("@info:whatsthis",
        "<title>Progress</title>"
        "<para>"
        "This view is used to inspect and edit task progress information for the selected project."
        "<nl/>The tasks are divided into groups dependent on the task status:"
        "<list>"
        "<item><emphasis>Not Started:</emphasis> Tasks that should have been started by now.</item>"
        "<item><emphasis>Running:</emphasis> Tasks that has been started, but not yet finished.</item>"
        "<item><emphasis>Finished:</emphasis> Tasks that where finished in this period.</item>"
        "<item><emphasis>Next Period:</emphasis> Tasks that is scheduled to be started in the next period.</item>"
        "</list>"
        "The time period is configurable."
        "</para><para>"
        "This view supports configuration and printing using the context menu."
        "<nl/><link url='%1'>More...</link>"
        "</para>", QStringLiteral("portfolio:progress")
        ));
}

ProgressView::~ProgressView()
{
}

void ProgressView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * ProgressView::popupMenu(const QString& name)
{
    Q_UNUSED(name)
    return nullptr;
}

KoPrintJob *ProgressView::createPrintJob()
{
    return m_detailsView->createPrintJob();
}

void ProgressView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)
    auto selectedIndex = selected.indexes().value(0);
    const ProjectsFilterModel *m = qobject_cast<ProjectsFilterModel*>(m_view->model());
    Q_ASSERT(m);
    KoDocument *doc = m->documentFromIndex(selectedIndex);
    if (doc) {
        m_detailsView->setProject(doc->project());
        m_detailsView->setScheduleManager(m->portfolio()->scheduleManager(doc));
        m_detailsView->setCommandDocument(doc);
        m_detailsView->updateReadWrite(doc->isReadWrite());
        Q_EMIT m_detailsView->expandAll();
    }
}

void ProgressView::saveSettings(QDomElement &settings) const
{
    settings.setAttribute(QStringLiteral("current-row"), m_view->currentIndex().row());
    auto e = settings.ownerDocument().createElement(QStringLiteral("details-view"));
    settings.appendChild(e);
    m_detailsView->saveContext(e);
}

void ProgressView::loadSettings(KoXmlElement &settings)
{
    const auto idx = m_view->model()->index(settings.attribute(QStringLiteral("current-row")).toInt(), 0);
    if (idx.isValid()) {
        m_view->setCurrentIndex(idx);
    } else if (m_view->model()->rowCount()) {
        m_view->setCurrentIndex(m_view->model()->index(0, 0));
    }
    m_detailsView->loadContext(settings.namedItem(QStringLiteral("details-view")).toElement());
}
