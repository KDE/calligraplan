/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTDOCUMENTSEDITOR_H
#define KPTDOCUMENTSEDITOR_H

#include "planui_export.h"

#include <kptviewbase.h>
#include <kptitemmodelbase.h>
#include <kptdocumentmodel.h>

class QPoint;

class KoDocument;

namespace KPlato
{

class PLANUI_EXPORT DocumentTreeView : public TreeViewBase
{
    Q_OBJECT
public:
    explicit DocumentTreeView(QWidget *parent);

    DocumentItemModel *model() const { return static_cast<DocumentItemModel*>(TreeViewBase::model()); }

    Documents *documents() const { return model()->documents(); }
    void setDocuments(Documents *docs) { model()->setDocuments(docs); }

    Document *currentDocument() const;
    QList<Document*> selectedDocuments() const;
    
    QModelIndexList selectedRows() const;
    
    using QTreeView::selectionChanged;
Q_SIGNALS:
    void selectedIndexesChanged(const QModelIndexList&);
    
protected Q_SLOTS:
    void slotSelectionChanged(const QItemSelection &selected);
};

class PLANUI_EXPORT DocumentsEditor : public ViewBase
{
    Q_OBJECT
public:
    DocumentsEditor(KoPart *part, KoDocument *doc, QWidget *parent);
    
    void setupGui();
    using ViewBase::draw;
    virtual void draw(Documents &docs);
    void draw() override;

    DocumentItemModel *model() const { return m_view->model(); }
    
    void updateReadWrite(bool readwrite) override;

    virtual Document *currentDocument() const;
    
    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;
    
    DocumentTreeView *view() const { return m_view; }
    
Q_SIGNALS:
    void addDocument();
    void deleteDocumentList(const QList<KPlato::Document*>&);
    void editDocument(KPlato::Document *doc);
    void viewDocument(KPlato::Document *doc);
    
public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

protected Q_SLOTS:
    void slotOptions() override;

protected:
    void updateActionsEnabled(bool on = true);

private Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotHeaderContextMenuRequested(const QPoint &pos) override;
    
    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&);
    void slotEnableActions(bool on);

    void slotEditDocument();
    void slotViewDocument();
    void slotAddDocument();
    void slotDeleteSelection();

private:
    DocumentTreeView *m_view;

    QAction *actionEditDocument;
    QAction *actionViewDocument;
    QAction *actionAddDocument;
    QAction *actionDeleteSelection;

};

}  //KPlato namespace

#endif
