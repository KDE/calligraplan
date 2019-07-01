/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2007 - 2010, 2012 Dag Andersen <danders@get2net.dk>
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
#include "ProjectStatusView.h"
#include "PerformanceStatusBase.h"

#include "kptglobal.h"
#include "kptlocale.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kptschedule.h"
#include "kpteffortcostmap.h"
#include "Help.h"
#include "kptdebug.h"

#include <KoXmlReader.h>
#include "KoDocument.h"
#include "KoPageLayoutWidget.h"

#include <QDrag>
#include <QMimeData>
#include <QPixmap>
#include <QMouseEvent>

using namespace KChart;

using namespace KPlato;


ProjectStatusView::ProjectStatusView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent),
    m_project(0)
{
    debugPlan<<"-------------------- creating ProjectStatusView -------------------";
    QVBoxLayout * l = new QVBoxLayout(this);
    l->setMargin(0);
    m_view = new PerformanceStatusBase(this);
    l->addWidget(m_view);

    setupGui();

    connect(m_view, &QWidget::customContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);

    Help::add(this,
              xi18nc("@info:whatsthis", 
                     "<title>Project Performance View</title>"
                     "<para>"
                     "Displays performance data aggregated to the project level."
                     "</para><para>"
                     "This view supports configuration and printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", Help::page("Manual/Project_Performance_View")));
}

void ProjectStatusView::setScheduleManager(ScheduleManager *sm)
{
    //debugPlan;
    m_view->setScheduleManager(sm);
    m_view->model()->clearNodes();
    if (m_project) {
        m_view->setNodes(QList<Node*>() << m_project);
    }
}

void ProjectStatusView::setProject(Project *project)
{
    m_project = project;
    m_view->model()->clearNodes();
    m_view->setProject(project);
}

void ProjectStatusView::setGuiActive(bool activate)
{
    debugPlan<<activate;
//    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
}

void ProjectStatusView::setupGui()
{
    // Add the context menu actions for the view options
    createOptionActions(ViewBase::OptionPrint | ViewBase::OptionPrintPreview | ViewBase::OptionPrintConfig | ViewBase::OptionViewConfig);
    addActionList("viewmenu", contextActionList());
}

void ProjectStatusView::slotOptions()
{
    ProjectStatusViewSettingsDialog *dlg = new ProjectStatusViewSettingsDialog(this, m_view, this, sender()->objectName() == "print options");
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

bool ProjectStatusView::loadContext(const KoXmlElement &context)
{
    debugPlan;
    ViewBase::loadContext(context);
    return m_view->loadContext(context);
}

void ProjectStatusView::saveContext(QDomElement &context) const
{
    ViewBase::saveContext(context);
    m_view->saveContext(context);
}

KoPrintJob *ProjectStatusView::createPrintJob()
{
    return m_view->createPrintJob(this);
}

void ProjectStatusView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    }
    ViewBase::mousePressEvent(event);
}

void ProjectStatusView::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        ViewBase::mouseMoveEvent(event);
        return;
    }
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        ViewBase::mouseMoveEvent(event);
        return;
    }
    event->accept();
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    QPixmap pixmap(m_view->size());
    m_view->render(&pixmap);
    mimeData->setImageData(pixmap);
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}

//-----------------
ProjectStatusViewSettingsDialog::ProjectStatusViewSettingsDialog(ViewBase *base, PerformanceStatusBase *view, QWidget *parent, bool selectPrint)
    : KPageDialog(parent),
    m_base(base)
{
    PerformanceStatusViewSettingsPanel *panel = new PerformanceStatusViewSettingsPanel(view, this);
    KPageWidgetItem *page = new KPageWidgetItem(panel, i18n("Chart"));
    page->setHeader(i18n("Chart Settings"));
    addPage(page);
    
    QTabWidget *tab = new QTabWidget();
    
    QWidget *w = ViewBase::createPageLayoutWidget(base);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);
    
    m_headerfooter = ViewBase::createHeaderFooterWidget(base);
    m_headerfooter->setOptions(base->printingOptions());
    tab->addTab(m_headerfooter, m_headerfooter->windowTitle());
    
    page = addPage(tab, i18n("Printing"));
    page->setHeader(i18n("Printing Options"));
    if (selectPrint) {
        setCurrentPage(page);
    }
    connect(this, &QDialog::accepted, panel, &PerformanceStatusViewSettingsPanel::slotOk);
    //TODO: there was no default button configured, should there?
    //     connect(button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked(bool)), panel, SLOT(setDefault()));
    connect(this, &QDialog::accepted, this, &ProjectStatusViewSettingsDialog::slotOk);
}

void ProjectStatusViewSettingsDialog::slotOk()
{
    debugPlan;
    m_base->setPageLayout(m_pagelayout->pageLayout());
    m_base->setPrintingOptions(m_headerfooter->options());
}

