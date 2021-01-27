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
#include "View.h"
#include "Factory.h"
#include "SummaryView.h"
#include "PortfolioView.h"
#include "PerformanceView.h"
#include "DetailsView.h"
#include "SchedulingView.h"
#include "MainDocument.h"
#include "GanttView.h"

#include <kptproject.h>

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
    setComponentName(Factory::global().componentName(), Factory::global().componentDisplayName());
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
    connect(sv, &SchedulingView::openDocument, this, &View::slotOpenDocument);

    item = m_views->addPage(new SummaryView(part, doc, m_views), i18n("Summary"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("view-time-schedule"));
    if (!doc->isEmpty()) {
        m_views->setCurrentPage(item);
    }
    
    item = m_views->addPage(new PerformanceView(part, doc, m_views), i18n("Performance"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("office-chart-bar"));

    item = m_views->addPage(new DetailsView(part, doc, m_views), i18n("Details"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("calligraplan"));

    item = m_views->addPage(new GanttView(part, doc, m_views), i18n("Gantt"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("calligraplan"));

    connect(m_views, &KPageWidget::currentPageChanged, this, &View::slotCurrentPageChanged);
}

View::~View()
{
}

void View::setupActions(void)
{
    if (koDocument()->isReadWrite()) {
        setXMLFile("ViewUi.rc", true);
    } else {
        setXMLFile("ViewUi_readonly.rc");
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

void View::slotOpenDocument(KoDocument *doc)
{
    KoPart *part = doc->documentPart();
    Q_ASSERT(part);
    KoMainWindow *mainWindow = part->createMainWindow();
    mainWindow->setRootDocument(doc, part);
    mainWindow->setNoCleanup(true);
    mainWindow->show();
}
