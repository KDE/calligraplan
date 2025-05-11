/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptdocumentspanel.h"

#include "kptdocumentseditor.h"
#include "kptdocumentmodel.h"
#include "kptnode.h"
#include "kptcommand.h"
#include "kptdebug.h"

#include <QDialog>
#include <QString>
#include <QModelIndex>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KUrlRequesterDialog>
#include <KMessageBox>
#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>

using namespace KPlato;

DocumentsPanel::DocumentsPanel(Node &node, QWidget *parent)
    : QWidget(parent),
    m_node(node),
    m_docs(node.documents())
{
    widget.setupUi(this);
    widget.nodeNameHolder->hide();
    QVBoxLayout *l = new QVBoxLayout(widget.itemViewHolder);
    m_view = new DocumentTreeView(widget.itemViewHolder);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(m_view);
    m_view->setDocuments(&m_docs);
    m_view->setReadWrite(true);
    
    currentChanged(QModelIndex());
    const QList<Document*> docs = m_docs.documents();
    for (Document *doc : docs) {
        m_orgurl.insert(doc, doc->url());
    }
    slotSelectionChanged(QModelIndexList());

    connect(widget.pbAdd, &QAbstractButton::clicked, this, &DocumentsPanel::slotAddUrl);
    connect(widget.pbChange, &QAbstractButton::clicked, this, &DocumentsPanel::slotChangeUrl);
    connect(widget.pbRemove, &QAbstractButton::clicked, this, &DocumentsPanel::slotRemoveUrl);
    connect(widget.pbView, &QAbstractButton::clicked, this, &DocumentsPanel::slotViewUrl);

    connect(m_view->model(), &QAbstractItemModel::dataChanged, this, &DocumentsPanel::dataChanged);

    connect(m_view, &DocumentTreeView::selectedIndexesChanged, this, &DocumentsPanel::slotSelectionChanged);

    if (model()->rowCount() > 0) {
        m_view->setCurrentIndex(model()->index(0, 0));
    }
}

void DocumentsPanel::addNodeName()
{
    widget.nodeName->setText(m_node.name());
    widget.nodeNameHolder->show();
}

DocumentItemModel *DocumentsPanel::model() const
{
    return m_view->model();
}

void DocumentsPanel::dataChanged(const QModelIndex &index)
{
    Document *doc = m_docs.value(index.row());
    if (doc == nullptr) {
        return;
    }
    m_state.insert(doc, (State)(m_state[ doc ] | Modified));
    Q_EMIT changed(true);
    debugPlan<<index<<doc<<m_state[ doc ];
}

void DocumentsPanel::slotSelectionChanged(const QModelIndexList &)
{
    QModelIndexList list = m_view->selectedRows();
    debugPlan<<list;
    widget.pbChange->setEnabled(list.count() == 1);
    widget.pbRemove->setEnabled(! list.isEmpty());
    widget.pbView->setEnabled(list.count() == 1);
}

void DocumentsPanel::currentChanged(const QModelIndex &index)
{
    Q_UNUSED(index)
//     widget.pbChange->setEnabled(index.isValid());
//     widget.pbRemove->setEnabled(index.isValid());
//     widget.pbView->setEnabled(index.isValid());
}

Document *DocumentsPanel::selectedDocument() const
{
    QList<Document*> lst = m_view->selectedDocuments();
    return lst.isEmpty() ? nullptr : lst.first();
}

void DocumentsPanel::slotAddUrl()
{
    QPointer<KUrlRequesterDialog> dlg = new KUrlRequesterDialog(QUrl(), QString(), this);
    dlg->setWindowTitle(xi18nc("@title:window", "Attach Document"));
    if (dlg->exec() == QDialog::Accepted && dlg) {
        if (m_docs.findDocument(dlg->selectedUrl())) {
            warnPlan<<"Document (url) already exists: "<<dlg->selectedUrl();
            KMessageBox::error(this, xi18nc("@info", "Document is already attached:<nl/><filename>%1</filename>", dlg->selectedUrl().toDisplayString()), xi18nc("@title:window", "Cannot Attach Document"));
        } else {
            Document *doc = new Document(dlg->selectedUrl());
            //DocumentAddCmd *cmd = new DocumentAddCmd(m_docs, doc, kundo2_i18n("Add document"));
            //m_cmds.push(cmd);
            m_docs.addDocument(doc);
            m_state.insert(doc, Added);
            model()->setDocuments(&m_docs); // refresh
            Q_EMIT changed(true);
        }
    }
    delete dlg;
}

void DocumentsPanel::slotChangeUrl()
{
    Document *doc = m_view->currentDocument();
    if (doc == nullptr) {
        return slotAddUrl();
    }
    KUrlRequesterDialog *dlg = new KUrlRequesterDialog(doc->url(), QString(), this);
    dlg->setWindowTitle(xi18nc("@title:window", "Modify Url"));
    if (dlg->exec() == QDialog::Accepted) {
        if (doc->url() != dlg->selectedUrl()) {
            if (m_docs.findDocument(dlg->selectedUrl())) {
                warnPlan<<"Document url already exists";
                KMessageBox::error(this, i18n("Document url already exists: %1", dlg->selectedUrl().toDisplayString()), i18n("Cannot Modify Url"));
            } else {
                debugPlan<<"Modify url: "<<doc->url()<<" : "<<dlg->selectedUrl();
                doc->setUrl(dlg->selectedUrl());
                m_state.insert(doc, (State)(m_state[ doc ] | Modified));
                model()->setDocuments(&m_docs);
                Q_EMIT changed(true);
                debugPlan<<"State: "<<doc->url()<<" : "<<m_state[ doc ];
            }
        }
    }
    delete dlg;
}

void DocumentsPanel::slotRemoveUrl()
{
    const QList<Document*> lst = m_view->selectedDocuments();
    bool mod = false;
    for (Document *doc : lst) {
        if (doc == nullptr) {
            continue;
        }
        m_docs.takeDocument(doc);
        if (m_state.contains(doc) && m_state[ doc ] & Added) {
            m_state.remove(doc);
        } else {
            m_state.insert(doc, Removed);
        }
        mod = true;
    }
    if (mod) {
        model()->setDocuments(&m_docs); // refresh
        Q_EMIT changed(true);
    }
}

void DocumentsPanel::slotViewUrl()
{
    Document *doc = selectedDocument();
    debugPlan<<"document:"<<doc;
    if (!doc || !doc->isValid()) {
        //KMessageBox::error(0, i18n("Cannot open document. Invalid url: %1", filename.pathOrUrl()));
        return;
    }

    auto *job = new KIO::OpenUrlJob(doc->url());
    job->setUiDelegate(KIO::createDefaultJobUiDelegate());
    job->start();

    return; //NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
}

MacroCommand *DocumentsPanel::buildCommand()
{
    if (m_docs == m_node.documents()) {
        debugPlan<<"No changes to save";
        return nullptr;
    }
    Documents &docs = m_node.documents();
    Document *d = nullptr;
    KUndo2MagicString txt = kundo2_i18n("Modify documents");
    MacroCommand *m = nullptr;
    QMap<Document*, State>::const_iterator i = m_state.constBegin();
    for (; i != m_state.constEnd(); ++i) {
        debugPlan<<i.key()<<i.value();
        if (i.value() & Removed) {
            d = docs.findDocument(m_orgurl[ i.key() ]);
            Q_ASSERT(d);
            if (m == nullptr) m = new MacroCommand(txt);
            debugPlan<<"remove document "<<i.key();
            m->addCommand(new DocumentRemoveCmd(m_node.documents(), d, kundo2_i18n("Remove document")));
        } else if ((i.value() & Added) == 0 && i.value() & Modified) {
            d = docs.findDocument(m_orgurl[ i.key() ]);
            Q_ASSERT(d);
            // do plain modifications before additions
            debugPlan<<"modify document "<<d;
            if (i.key()->url() != d->url()) {
                if (m == nullptr) m = new MacroCommand(txt);
                m->addCommand(new DocumentModifyUrlCmd(d, i.key()->url(), kundo2_i18n("Modify document url")));
            }
            if (i.key()->type() != d->type()) {
                if (m == nullptr) m = new MacroCommand(txt);
                m->addCommand(new DocumentModifyTypeCmd(d, i.key()->type(), kundo2_i18n("Modify document type")));
            }
            if (i.key()->status() != d->status()) {
                if (m == nullptr) m = new MacroCommand(txt);
                m->addCommand(new DocumentModifyStatusCmd(d, i.key()->status(), kundo2_i18n("Modify document status")));
            }
            if (i.key()->sendAs() != d->sendAs()) {
                if (m == nullptr) m = new MacroCommand(txt);
                m->addCommand(new DocumentModifySendAsCmd(d, i.key()->sendAs(), kundo2_i18n("Modify document send control")));
            }
            if (i.key()->name() != d->name()) {
                if (m == nullptr) m = new MacroCommand(txt);
                m->addCommand(new DocumentModifyNameCmd(d, i.key()->name()/*, kundo2_i18n("Modify document name")*/));
            }
        } else if (i.value() & Added) {
            if (m == nullptr) m = new MacroCommand(txt);
            debugPlan<<i.key()<<m_docs.documents();
            d = m_docs.takeDocument(i.key());
            debugPlan<<"add document "<<d;
            m->addCommand(new DocumentAddCmd(docs, d, kundo2_i18n("Add document")));
        }
    }
    return m;
}
