/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptdocumentseditor.h"

#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptdocuments.h"
#include "kptdatetime.h"
#include "kptitemviewsettup.h"
#include "kptdebug.h"

#include <KoIcon.h>

#include <QMenu>
#include <QList>
#include <QVBoxLayout>
#include <QAction>
#include <QHeaderView>

#include <KLocalizedString>
#include <KActionCollection>

#include <KoDocument.h>


namespace KPlato
{


//--------------------
DocumentTreeView::DocumentTreeView(QWidget *parent)
    : TreeViewBase(parent)
{
//    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    setStretchLastSection(true);
    
    DocumentItemModel *m = new DocumentItemModel();
    setModel(m);
    
    setRootIsDecorated (false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    createItemDelegates(m);

    setAcceptDrops(true);
    setDropIndicatorShown(true);
    
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &DocumentTreeView::slotSelectionChanged);

    setColumnHidden(DocumentModel::Property_Status, true); // not used atm
    header()->moveSection(DocumentModel::Property_Url, model()->columnCount()-1);
}

Document *DocumentTreeView::currentDocument() const
{
    return model()->document(selectionModel()->currentIndex());
}

QModelIndexList DocumentTreeView::selectedRows() const
{
    return selectionModel()->selectedRows();
}

void DocumentTreeView::slotSelectionChanged(const QItemSelection &selected)
{
    Q_EMIT selectedIndexesChanged(selected.indexes());
}

QList<Document*> DocumentTreeView::selectedDocuments() const
{
    QList<Document*> lst;
    const QModelIndexList indexes = selectionModel()->selectedRows();
    for (const QModelIndex &i : indexes) {
        Document *doc = model()->document(i);
        if (doc) {
            lst << doc;
        }
    }
    return lst;
}

//-----------------------------------
DocumentsEditor::DocumentsEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    setupGui();
    
    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new DocumentTreeView(this);
    l->addWidget(m_view);
    
    m_view->setEditTriggers(m_view->editTriggers() | QAbstractItemView::EditKeyPressed);

    connect(model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);

    connect(m_view, &DocumentTreeView::selectedIndexesChanged, this, &DocumentsEditor::slotSelectionChanged);
    connect(m_view, &TreeViewBase::contextMenuRequested, this, &DocumentsEditor::slotContextMenuRequested);
    connect(m_view, &DocumentTreeView::headerContextMenuRequested, this, &DocumentsEditor::slotHeaderContextMenuRequested);

}

void DocumentsEditor::updateReadWrite(bool readwrite)
{
    debugPlan<<isReadWrite()<<"->"<<readwrite;
    ViewBase::updateReadWrite(readwrite);
    m_view->setReadWrite(readwrite);
    updateActionsEnabled(readwrite);
}

void DocumentsEditor::draw(Documents &docs)
{
    m_view->setDocuments(&docs);
}

void DocumentsEditor::draw()
{
}

void DocumentsEditor::setGuiActive(bool activate)
{
    debugPlan<<activate;
    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
    if (activate && !m_view->selectionModel()->currentIndex().isValid()) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }
}

void DocumentsEditor::slotContextMenuRequested(const QModelIndex &index, const QPoint& pos)
{
    //debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
    QString name;
    if (index.isValid()) {
        Document *obj = m_view->model()->document(index);
        if (obj) {
            name = QStringLiteral("documentseditor_popup");
        }
    }
    m_view->setContextMenuIndex(index);
    Q_EMIT requestPopupMenu(name, pos);
    m_view->setContextMenuIndex(QModelIndex());
}

void DocumentsEditor::slotHeaderContextMenuRequested(const QPoint &pos)
{
    debugPlan;
    QList<QAction*> lst = contextActionList();
    if (! lst.isEmpty()) {
        QMenu::exec(lst, pos,  lst.first());
    }
}

Document *DocumentsEditor::currentDocument() const
{
    return m_view->currentDocument();
}

void DocumentsEditor::slotCurrentChanged(const QModelIndex &)
{
    //debugPlan<<curr.row()<<","<<curr.column();
//    slotEnableActions();
}

void DocumentsEditor::slotSelectionChanged(const QModelIndexList &list)
{
    debugPlan<<list.count();
    updateActionsEnabled(true);
}

void DocumentsEditor::slotEnableActions(bool on)
{
    updateActionsEnabled(on);
}

void DocumentsEditor::updateActionsEnabled(bool on)
{
    QList<Document*> lst = m_view->selectedDocuments();
    if (lst.isEmpty() || lst.count() > 1) {
        actionEditDocument->setEnabled(false);
        actionViewDocument->setEnabled(false);
        return;
    }
    Document *doc = lst.first();
    actionViewDocument->setEnabled(on);
    actionEditDocument->setEnabled(on && doc->type() == Document::Type_Product && isReadWrite());
}

void DocumentsEditor::setupGui()
{
#if 0
    QString name = "documentseditor_edit_list";
    actionEditDocument  = new QAction(koIcon("document-properties"), i18n("Edit..."), this);
    actionCollection()->addAction("edit_documents", actionEditDocument);
//    actionCollection()->setDefaultShortcut(actionEditDocument, Qt::CTRL + Qt::SHIFT + Qt::Key_I);
    connect(actionEditDocument, &QAction::triggered, this, &DocumentsEditor::slotEditDocument);
    addAction(name, actionEditDocument);

    actionViewDocument  = new QAction(koIcon("document-preview"), xi18nc("@action View a document", "View..."), this);
    actionCollection()->addAction("view_documents", actionViewDocument);
//    actionCollection()->setDefaultShortcut(actionViewDocument, Qt::CTRL + Qt::SHIFT + Qt::Key_I);
    connect(actionViewDocument, &QAction::triggered, this, &DocumentsEditor::slotViewDocument);
    addAction(name, actionViewDocument);

    
/*    actionDeleteSelection  = new QAction(koIcon("edit-delete"), i18n("Delete"), this);
    actionCollection()->addAction("delete_selection", actionDeleteSelection);
    actionCollection()->setDefaultShortcut(actionDeleteSelection, Qt::Key_Delete);
    connect(actionDeleteSelection, SIGNAL(triggered(bool)), SLOT(slotDeleteSelection()));
    addAction(name, actionDeleteSelection);*/
    
    // Add the context menu actions for the view options
    createOptionActions(ViewBase::OptionExpand | ViewBase::OptionCollapse | ViewBase::OptionViewConfig);
#endif
}

void DocumentsEditor::slotOptions()
{
    debugPlan;
    ItemViewSettupDialog dlg(this, m_view/*->masterView()*/);
    dlg.exec();
}

void DocumentsEditor::slotEditDocument()
{
    QList<Document*> dl = m_view->selectedDocuments();
    if (dl.isEmpty()) {
        return;
    }
    debugPlan<<dl;
    Q_EMIT editDocument(dl.first());
}

void DocumentsEditor::slotViewDocument()
{
    QList<Document*> dl = m_view->selectedDocuments();
    if (dl.isEmpty()) {
        return;
    }
    debugPlan<<dl;
    Q_EMIT viewDocument(dl.first());
}

void DocumentsEditor::slotAddDocument()
{
    //debugPlan;
    QList<Document*> dl = m_view->selectedDocuments();
    Document *after = nullptr;
    if (dl.count() > 0) {
        after = dl.last();
    }
    Document *doc = new Document();
    QModelIndex i = m_view->model()->insertDocument(doc, after);
    if (i.isValid()) {
        m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        m_view->edit(i);
    }
}

void DocumentsEditor::slotDeleteSelection()
{
    QList<Document*> lst = m_view->selectedDocuments();
    //debugPlan<<lst.count()<<" objects";
    if (! lst.isEmpty()) {
        Q_EMIT deleteDocumentList(lst);
    }
}

bool DocumentsEditor::loadContext(const KoXmlElement &context)
{
    return m_view->loadContext(m_view->model()->columnMap(), context);
}

void DocumentsEditor::saveContext(QDomElement &context) const
{
    m_view->saveContext(m_view->model()->columnMap(), context);
}


} // namespace KPlato
