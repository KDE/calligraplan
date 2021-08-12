/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "View.h"
#include "PortfolioFactory.h"
#include "SummaryView.h"
#include "PortfolioView.h"
#include "PerformanceView.h"
#include "DetailsView.h"
#include "SchedulingView.h"
#include "MainDocument.h"
#include "GanttView.h"

#include <kptproject.h>
#include <kptganttview.h>

#include <KoApplication.h>
#include <KoComponentData.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>
#include <performance/ProjectStatusView.h>

#include <KRecentFilesAction>
#include <KActionCollection>
#include <KXMLGUIFactory>
#include <KPageWidget>
#include <KPageWidgetItem>

#include <QVBoxLayout>
#include <QDir>


View::View(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlanGroup;
    setComponentName(PortfolioFactory::global().componentName(), PortfolioFactory::global().componentDisplayName());
    setupActions();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    m_views = new KPageWidget(this);
    m_views->setFaceType (KPageView::Auto);
    layout->addWidget(m_views);

    KPageWidgetItem *item;
    item = m_views->addPage(new PortfolioView(part, doc, m_views), i18n("Configuration"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("settings-configure"));

    SchedulingView *sv = new SchedulingView(part, doc, m_views);
    item = m_views->addPage(sv, i18n("Scheduling"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("view-time-schedule-calculus"));

    item = m_views->addPage(new SummaryView(part, doc, m_views), i18n("Summary"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("view-time-schedule"));
    if (!doc->isEmpty()) {
        m_views->setCurrentPage(item);
    }

    item = m_views->addPage(new PerformanceView(part, doc, m_views), i18n("Performance"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("office-chart-bar"));

    item = m_views->addPage(new DetailsView(part, doc, m_views), i18n("Progress"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("view-time-schedule"));

    auto gv = new GanttView(part, doc, m_views);
    item = m_views->addPage(gv, i18n("Gantt Summary"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("calligraplan"));
    connect(gv, &GanttView::openDocument, this, &View::slotOpenDocument);

    // NOTE: Adding a new view to KPageWidget outside the c'tor gives problems with resize (shrinking),
    // so atm we create everything now.
    const auto docs = static_cast<MainDocument*>(doc)->documents();
    for (auto d : docs) {
        openDocument(d);
    }
    connect(m_views, &KPageWidget::currentPageChanged, this, &View::slotCurrentPageChanged);
}

View::~View()
{
}

void View::setupActions(void)
{
    if (koDocument()->isReadWrite()) {
        setXMLFile("Portfolio_ViewUi.rc", true);
    } else {
        setXMLFile("Portfolio_ViewUi_readonly.rc");
    }
    QAction *configureAction = new QAction(koIcon("configure"), i18n("Configure Portfolio..."), this);
    actionCollection()->addAction("configure", configureAction);
    connect(configureAction, &QAction::triggered, mainWindow(), &KoMainWindow::slotConfigure);
}

void View::guiActivateEvent(bool activate)
{
    KoView *v = qobject_cast<KoView*>(m_views->currentPage()->widget());
    Q_ASSERT(v);
    if (activate) {
        factory()->addClient(v);
    } else {
        factory()->removeClient(v);
    }
}

void View::slotCurrentPageChanged(KPageWidgetItem *current, KPageWidgetItem *before)
{
    if (before) {
        KoView *v = qobject_cast<KoView*>(before->widget());
        if (v) {
            factory()->removeClient(v);
        }
    }
    if (current) {
        KoView *v = qobject_cast<KoView*>(current->widget());
        if (v) {
            factory()->addClient(v);
        }
    }
}

void View::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu *View::popupMenu(const QString& name)
{
    return nullptr;
}

KoPageLayout View::pageLayout() const
{
    return KoView::pageLayout();
}

void View::setPageLayout(const KoPageLayout &pageLayout)
{
    KoView::setPageLayout(pageLayout);
}

QPrintDialog *View::createPrintDialog(KoPrintJob *printJob, QWidget *parent)
{
    return nullptr;
}

KPageWidgetItem *View::openDocument(KoDocument *doc)
{
    auto part = doc->documentPart();
    Q_ASSERT(part);
    auto project = doc->project();
    auto item = m_ganttViews.value(project->name());
    if (!item) {
        auto v = new KPlato::GanttView(part, doc, m_views);
        item = m_views->addPage(v, project->name());
        m_ganttViews.insert(project->name(), item);
        item->setHeaderVisible(false);
        item->setIcon(koIcon("calligraplan"));

        v->setProject(project);
        v->setScheduleManager(doc->project()->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString()));
        connect(doc, &KoDocument::scheduleManagerChanged, v, &KPlato::GanttView::setScheduleManager);
    }
    return item;
}

void View::slotOpenDocument(KoDocument *doc)
{
    auto item = openDocument(doc);
    m_views->setCurrentPage(item);
}
