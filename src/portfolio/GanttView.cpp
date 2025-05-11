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
#include <kptnode.h>

#include <kptganttview.h>
#include <GanttViewBase.h>
#include <BasicGanttViewSettingsDialog.h>

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

#include <KGanttGraphicsView>

GanttView::GanttView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KPlato::ViewBase(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    setupGui();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
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
    tv->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    tv->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tv->header(), &QHeaderView::customContextMenuRequested, this, &GanttView::slotHeaderCustomContextMenuRequested);
    connect(tv, &QTreeView::customContextMenuRequested, this, &GanttView::slotCustomContextMenuRequested);

    setWhatsThis(xi18nc("@info:whatsthis",
        "<title>Gantt Summary</title>"
        "<para>"
        "Shows a gantt chart of all projects in your portfolio."
        "</para><para>"
        "A gantt view for a project can be shown by selecting <interface>Context Menu->Open Project</interface>."
        "<nl/><link url='%1'>More...</link>"
        "</para>", QStringLiteral("portfolio:ganttsummary")
        ));

}

GanttView::~GanttView()
{
}

void GanttView::setupGui()
{
    setXMLFile(QStringLiteral("Portfolio_GanttViewUi.rc"));

    auto a = new QAction(koIcon("view-time-schedule-calculus"), i18n("Open Project"), this);
    actionCollection()->addAction(QStringLiteral("gantt_open_project"), a);
    connect(a, &QAction::triggered, this, &GanttView::openProject, Qt::QueuedConnection);

    createOptionActions(ViewBase::OptionAll);
}

void GanttView::openProject()
{
    QModelIndex idx = m_view->leftView()->selectionModel()->selectedRows().value(0);
    if (idx.isValid()) {
        KoDocument *doc = idx.data(DOCUMENT_ROLE).value<KoDocument*>();
        Q_EMIT openKoDocument(doc);
    }
}

void GanttView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu *GanttView::popupMenu(const QString& name)
{
    Q_UNUSED(name)
    return nullptr;
}

void GanttView::slotHeaderCustomContextMenuRequested(const QPoint &pos)
{
    auto menu = qobject_cast<QMenu*>(factory()->container(QStringLiteral("gantt_context_menu"), this));
    if (menu && !menu->isEmpty()) {
        menu->exec(m_view->leftView()->viewport()->mapToGlobal(pos));
    }
}

void GanttView::slotCustomContextMenuRequested(const QPoint &pos)
{
    QString name;
    const auto idx = m_view->leftView()->indexAt(pos);
    if (idx.isValid()) {
        name = QStringLiteral("gantt_project_context_menu");
    } else {
        name = QStringLiteral("gantt_context_menu");
    }
    auto menu = qobject_cast<QMenu*>(factory()->container(name, this));
    if (menu && !menu->isEmpty()) {
        menu->exec(m_view->leftView()->viewport()->mapToGlobal(pos));
    }
}

void GanttView::slotOptions()
{
    auto dlg = new KPlato::BasicGanttViewSettingsDialog(m_view, this, sender()->objectName() == QStringLiteral("print_options"));
    int result = dlg->exec();
    if (result == QDialog::Accepted) {
        m_view->graphicsView()->updateScene();
        Q_EMIT optionsModified();
    }
}

KoPrintJob *GanttView::createPrintJob()
{
    return new KPlato::GanttPrintingDialog(this, m_view);
}

void GanttView::saveSettings(QDomElement &settings) const
{
    Q_UNUSED(settings)
}

void GanttView::loadSettings(KoXmlElement &settings)
{
    Q_UNUSED(settings)
}
