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
#include "PortfolioView.h"
#include "Factory.h"
#include "PortfolioModel.h"
#include "MainDocument.h"

#include <kptproject.h>

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
#include <QDir>

PortfolioView::PortfolioView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    if (doc && doc->isReadWrite()) {
        setXMLFile("PortfolioViewUi.rc");
    } else {
        setXMLFile("PortfolioViewUi_readonly.rc");
    }
    setupGui();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    m_view = new QTreeView(this);
    m_view->setRootIsDecorated(false);
    layout->addWidget(m_view);

    PortfolioModel *model = new PortfolioModel(m_view);
    model->setPortfolio(qobject_cast<MainDocument*>(doc));
    m_view->setModel(model);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PortfolioView::selectionChanged);
    updateActionsEnabled();
}

PortfolioView::~PortfolioView()
{
}

void PortfolioView::setupGui()
{
    QAction *a = new QAction(koIcon("list-add"), i18n("Add"), this);
    actionCollection()->addAction("add_project", a);
    actionCollection()->setDefaultShortcut(a, Qt::Key_Insert);
    connect(a, &QAction::triggered, this, &PortfolioView::slotAddProject);

    a = new QAction(koIcon("list-remove"), i18n("Remove"), this);
    actionCollection()->addAction("remove_selected", a);
    actionCollection()->setDefaultShortcut(a, Qt::Key_Delete);
    connect(a, &QAction::triggered, this, &PortfolioView::slotRemoveSelected);
}

void PortfolioView::updateActionsEnabled()
{
    bool enable = m_view->selectionModel() && (m_view->selectionModel()->selectedRows().count() == 1);
    actionCollection()->action("remove_selected")->setEnabled(enable);
}

void PortfolioView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)
    updateActionsEnabled();
}

void PortfolioView::slotAddProject()
{
    const QList<QUrl> urls = QFileDialog::getOpenFileUrls(nullptr, i18n("Add Project"), QUrl(), "Plan (*.plan)");
    for (const QUrl &url : urls) {
        loadProject(url);
    }
//     KoFileDialog dlg(nullptr, KoFileDialog::OpenFiles, "Add Project");
//     dlg.setNameFilters(QStringList()<<"Plan (*.plan)");
//     QStringList files = dlg.filenames();
//     if (!files.isEmpty()) {
//         for (const QString &file : files) {
//             loadProject(file);
//         }
//     }

}

void PortfolioView::slotRemoveSelected()
{
    PortfolioModel *m = qobject_cast<PortfolioModel*>(m_view->model());
    QList<KoDocument*> docs;
    const auto selectedRows = m_view->selectionModel()->selectedRows();
    for (const QModelIndex &idx : selectedRows) {
        KoDocument *doc = m->documentFromIndex(idx);
        if (doc) {
            docs << doc;
        }
    }
    for (KoDocument *doc : qAsConst(docs)) {
        m->portfolio()->removeDocument(doc);
    }
}

void PortfolioView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QMenu * PortfolioView::popupMenu(const QString& name)
{
    return nullptr;
}

QPrintDialog *PortfolioView::createPrintDialog(KoPrintJob *printJob, QWidget *parent)
{
    return nullptr;
}

void PortfolioView::loadProject(const QUrl &url)
{
    MainDocument *portfolio = qobject_cast<MainDocument*>(koDocument());
    Q_ASSERT(portfolio);
    KoPart *part = KoApplication::koApplication()->getPartFromUrl(url);
    Q_ASSERT(part);
    if (part) {
        KoDocument *doc = part->createDocument(part);
        doc->setAutoSave(0);
        doc->setProperty(BLOCKSHAREDPROJECTSLOADING, true);
        connect(doc, &KoDocument::sigProgress, mainWindow(), &KoMainWindow::slotProgress);
        connect(doc, &KoDocument::completed, this, &PortfolioView::slotLoadCompleted);
        connect(doc, &KoDocument::canceled, this, &PortfolioView::slotLoadCanceled);
        doc->openUrl(url);
    }
}

void PortfolioView::slotLoadCompleted()
{
    KoDocument *doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    MainDocument *portfolio = qobject_cast<MainDocument*>(koDocument());
    disconnect(doc, &KoDocument::sigProgress, mainWindow(), &KoMainWindow::slotProgress);
    portfolio->setDocumentProperty(doc, ISPORTFOLIO, true);
    if (!portfolio->addDocument(doc)) {
        doc->deleteLater();
    } else {
        auto manager = portfolio->findBestScheduleManager(doc);
        if (manager) {
            portfolio->setDocumentProperty(doc, SCHEDULEMANAGERNAME, manager->name());
        }
    }
}

void PortfolioView::slotLoadCanceled()
{
    KoDocument *doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    doc->deleteLater();
}
