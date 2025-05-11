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
#include "ProgressView.h"
#include "ResourceUsageView.h"
#include "SchedulingView.h"
#include "MainDocument.h"
#include "GanttView.h"
#include "portfoliosettings.h"
#include "PlanGroupDebug.h"

#include <kptproject.h>
#include <kptganttview.h>

#include <KoApplication.h>
#include <KoComponentData.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>
#include <performance/ProjectStatusView.h>
#include <ExtraProperties.h>

#include <KRecentFilesAction>
#include <KActionCollection>
#include <KXMLGUIFactory>
#include <KPageWidget>
#include <KPageWidgetItem>

#include <QVBoxLayout>
#include <QDir>


View::View(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, true, parent)
    , m_readWrite(false)
{
    //debugPlanGroup;
    setComponentName(PortfolioFactory::global().componentName(), PortfolioFactory::global().componentDisplayName());

    setupGui();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_views = new KPageWidget(this);
    m_views->setFaceType (KPageView::Tree);
    layout->addWidget(m_views);

    KPageWidgetItem *item;
    item = m_views->addPage(new PortfolioView(part, doc, m_views), i18n("Portfolio Content"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("settings-configure"));
    m_pageItems.insert(QStringLiteral("PortfolioView"), item);

    SchedulingView *sv = new SchedulingView(part, doc, m_views);
    item = m_views->addPage(sv, i18n("Scheduling"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("view-time-schedule-calculus"));
    m_pageItems.insert(QStringLiteral("SchedulingView"), item);
    connect(sv, &SchedulingView::projectCalculated, this, &View::projectCalculated);

    item = m_views->addPage(new SummaryView(part, doc, m_views), i18n("Summary"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("view-time-schedule"));
    m_pageItems.insert(QStringLiteral("SummaryView"), item);
    if (!doc->isEmpty()) {
        m_views->setCurrentPage(item);
    }

    item = m_views->addPage(new ProgressView(part, doc, m_views), i18n("Progress"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("view-time-schedule"));
    m_pageItems.insert(QStringLiteral("ProgressView"), item);

    item = m_views->addPage(new PerformanceView(part, doc, m_views), i18n("Performance"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("office-chart-bar"));
    m_pageItems.insert(QStringLiteral("PerformanceView"), item);

    auto rv = new ResourceUsageView(part, doc, m_views);
    rv->setNumDays(7);
    item = m_views->addPage(rv, i18n("Resource Usage"));
    item->setHeaderVisible(false);
    item->setIcon(koIcon("system-users"));
    m_pageItems.insert(QStringLiteral("ResourceUsageView"), item);

    auto gv = new GanttView(part, doc, m_views);
    m_ganttSummary = m_views->addPage(gv, i18n("Gantt Summary"));
    m_ganttSummary->setHeaderVisible(false);
    m_ganttSummary->setIcon(koIcon("calligraplan"));
    m_pageItems.insert(QStringLiteral("GanttView"), m_ganttSummary);
    connect(gv, &GanttView::openKoDocument, this, &View::slotOpenDocument);

    // NOTE: Adding a new view to KPageWidget outside the c'tor gives problems with resize (shrinking),
    // so atm we create everything now.
    const auto docs = static_cast<MainDocument*>(doc)->documents();
    for (auto d : docs) {
        openDocument(d);
    }
    loadSettings();
    connect(static_cast<MainDocument*>(doc), &MainDocument::saveSettings, this, &View::saveSettings);
    connect(m_views, &KPageWidget::currentPageChanged, this, &View::slotCurrentPageChanged);
    connect(mainWindow(), &KoMainWindow::restoringDone, this, &View::slotUpdateActions);
}

View::~View()
{
}

void View::slotUpdateActions()
{
    const auto mw = mainWindow();
    const auto c = mw->actionCollection();
    const bool disable = static_cast<MainDocument*>(koDocument())->documents().isEmpty();
    c->action(QStringLiteral("file_save"))->setEnabled(!disable);
    c->action(QStringLiteral("file_save_as"))->setEnabled(!disable);
    c->action(QStringLiteral("file_send_file"))->setEnabled(!disable);
    c->action(QStringLiteral("file_close"))->setEnabled(!disable);
    c->action(QStringLiteral("file_documentinfo"))->setEnabled(!disable);
}

void View::saveSettings(QDomDocument &xml)
{
    QDomElement portfolio = xml.documentElement();
    QDomElement views = xml.createElement(QStringLiteral("views"));
    portfolio.appendChild(views);

    auto e = views.ownerDocument().createElement(QStringLiteral("current-view"));
    auto page = m_pageItems.key(m_views->currentPage());
    if (page.isEmpty()) {
        page = m_ganttViews.key(m_views->currentPage());
    }
    e.setAttribute(QStringLiteral("view"), page);
    views.appendChild(e);

    {QHash<QString, KPageWidgetItem*>::const_iterator it = m_pageItems.constBegin();
    for (; it != m_pageItems.constEnd(); ++it) {
        auto pe = views.ownerDocument().createElement(it.key());
        views.appendChild(pe);
        QMetaObject::invokeMethod(it.value()->widget(), "saveSettings", Q_ARG(QDomElement&, pe));
    }}
    if (!m_ganttViews.isEmpty()) {
        auto ganttviews = views.ownerDocument().createElement(QStringLiteral("ganttviews"));
        views.appendChild(ganttviews);
        {QHash<QString, KPageWidgetItem*>::const_iterator it = m_ganttViews.constBegin();
        for (; it != m_ganttViews.constEnd(); ++it) {
            auto ge = ganttviews.ownerDocument().createElement(QStringLiteral("ganttview"));
            ganttviews.appendChild(ge);
            ge.setAttribute(QStringLiteral("id"), it.key());
            static_cast<KPlato::GanttView*>(it.value()->widget())->saveContext(ge);
        }}
    }
}

void View::loadSettings()
{
    const auto doc = static_cast<MainDocument*>(koDocument());
    KoXmlElement views = doc->xmlDocument().documentElement().namedItem("views").toElement();
    if (views.isNull()) {
        return;
    }
    KoXmlElement e = views.namedItem(QStringLiteral("current-view")).toElement();
    auto page = m_pageItems.value(e.attribute(QStringLiteral("view")));
    if (!page) {
        page = m_ganttViews.value(e.attribute(QStringLiteral("view")));
    }
    if (page) {
        m_views->setCurrentPage(page);
    }
    QTimer::singleShot(0, this, [this]() {
        const auto doc = static_cast<MainDocument*>(koDocument());
        KoXmlElement views = doc->xmlDocument().documentElement().namedItem("views").toElement();
        if (views.isNull()) {
            return;
        }
        KoXmlElement e;
        forEachElement(e, views) {
            if (e.tagName() == QStringLiteral("current-view")) {
                continue;
            } else if (e.tagName() == QStringLiteral("ganttviews")) {
                KoXmlElement ge;
                forEachElement(ge, e) {
                    if (ge.tagName() == QStringLiteral("ganttview")) {
                        KPageWidgetItem *page = m_ganttViews.value(ge.attribute(QStringLiteral("id")));
                        if (page) {
                            static_cast<KPlato::GanttView*>(page->widget())->loadContext(ge);
                        }
                    }
                }
                continue;
            }
            auto page = m_pageItems.value(e.tagName());
            if (page) {
                QMetaObject::invokeMethod(page->widget(), "loadSettings", Q_ARG(KoXmlElement&, e));
            } else {
                warnPortfolio<<"Unknown page:"<<e.tagName();
            }
        }
        QApplication::restoreOverrideCursor(); // TODO: why needed?
    });
}

void View::setupGui(void)
{
    setXMLFile(QStringLiteral("Portfolio_ViewUi.rc"), true);
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
            v->guiActivateEvent(false);
            factory()->removeClient(v);
        }
    }
    if (current) {
        KoView *v = qobject_cast<KoView*>(current->widget());
        if (v) {
            factory()->addClient(v);
            v->guiActivateEvent(true);
        }
    }
}

void View::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu *View::popupMenu(const QString& name)
{
    Q_UNUSED(name)
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

KoPrintJob * View::createPrintJob()
{
    KoView *v = qobject_cast<KoView*>(m_views->currentPage()->widget());
    if (v == nullptr) {
        return nullptr;
    }
    return v->createPrintJob();
}

KPageWidgetItem *View::openDocument(KoDocument *doc)
{
    auto part = doc->documentPart();
    Q_ASSERT(part);
    auto project = doc->project();
    auto item = m_ganttViews.value(project->name());
    if (!item) {
        auto v = new KPlato::GanttView(part, doc, m_views);
        if (m_ganttSummary) {
            item = m_views->addSubPage(m_ganttSummary, v, project->name());
        } else {
            item = m_views->addPage(v, project->name());
        }
        m_ganttViews.insert(project->name(), item);
        item->setHeaderVisible(false);
        item->setIcon(koIcon("calligraplan"));

        v->setProject(project);
        v->setScheduleManager(doc->project()->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString()));
        connect(doc, &KoDocument::scheduleManagerChanged, v, &KPlato::GanttView::setScheduleManager);
        connect(this, &View::projectCalculated, v, &KPlato::GanttView::slotProjectCalculated);
    }
    return item;
}

void View::slotOpenDocument(KoDocument *doc)
{
    auto item = openDocument(doc);
    m_views->setCurrentPage(item);
}
