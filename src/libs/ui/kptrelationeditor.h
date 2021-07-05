/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTRELATIONEDITOR_H
#define KPTRELATIONEDITOR_H

#include "planui_export.h"

#include "kptglobal.h"
#include "kptviewbase.h"
#include "kptrelationmodel.h"

class KoDocument;

namespace KPlato
{

class Project;
class Node;
class RelationItemModel;
class Relation;

class PLANUI_EXPORT RelationTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit RelationTreeView(QWidget *parent = nullptr);
    
    RelationItemModel *model() const { return static_cast<RelationItemModel*>(DoubleTreeViewBase::model()); }
    
    Project *project() const { return model()->project(); }
    void setProject(Project *project) { model()->setProject(project); }
    
    void setNode(Node *node) { model()->setNode(node); }
    Relation *currentRelation() const { return model()->relation(selectionModel()->currentIndex()); }
Q_SIGNALS:
    void currentColumnChanged(const QModelIndex&, const QModelIndex&);
    
protected Q_SLOTS:
    void slotCurrentChanged(const QModelIndex &curr, const QModelIndex&);
};

class PLANUI_EXPORT RelationEditor : public ViewBase
{
    Q_OBJECT
public:
    /// Create a relation editor
    RelationEditor(KoPart *part, KoDocument *doc, QWidget *parent);
    
    void setupGui();
    void draw(Project &project) override;
    void draw() override;

    Relation *currentRelation() const override;
    Relation *selectedRelation() const;

    void updateReadWrite(bool readwrite) override;

    RelationItemModel *model() const { return m_view->model(); }

    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;
    
    KoPrintJob *createPrintJob() override;

Q_SIGNALS:
    void openNode();
    void addRelation();
    void deleteRelation(KPlato::Relation *);

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

protected Q_SLOTS:
    void slotOptions() override;

protected:
    void updateActionsEnabled(bool on);

private Q_SLOTS:
    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&, const QModelIndex&);
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    
    void slotEnableActions();

    void slotAddRelation();
    void slotDeleteRelation(KPlato::Relation *r);

    void slotSplitView();
    
    void slotHeaderContextMenuRequested(const QPoint&) override;
    
private:
    void edit(const QModelIndex &index);

private:
    RelationTreeView *m_view;
};


} //namespace KPlato

#endif
