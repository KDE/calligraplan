/* This file is part of the KDE project
  Copyright (C) 2007 - 2009 Dag Andersen <dag.andersen@kdemail.net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#ifndef TASKWORKPACKAGEVIEW_H
#define TASKWORKPACKAGEVIEW_H

#include "planwork_export.h"

#include "kptitemmodelbase.h"

#include "kptviewbase.h"
#include "kptganttview.h"
#include "gantt/kptganttitemdelegate.h"

#include <KGanttView>

#include <QSplitter>

class QItemSelection;


namespace KPlato
{

class Project;
class Node;
class Document;

}

namespace KPlatoWork
{
class Part;
class WorkPackage;

class TaskWorkPackageModel;

class PLANWORK_EXPORT TaskWorkPackageTreeView : public KPlato::DoubleTreeViewBase
{
    Q_OBJECT
public:
    TaskWorkPackageTreeView(Part *part, QWidget *parent);
    
    
    //void setSelectionModel(QItemSelectionModel *selectionModel);

    TaskWorkPackageModel *itemModel() const;
    
    KPlato::Project *project() const;
    void setProject(KPlato::Project *project);

    KPlato::Document *currentDocument() const;
    KPlato::Node *currentNode() const;
    QList<KPlato::Node*> selectedNodes() const;

Q_SIGNALS:
    void sectionsMoved();

protected Q_SLOTS:
    void slotActivated(const QModelIndex &index);
    void setSortOrder(int col, Qt::SortOrder order);

protected:
    void dragMoveEvent(QDragMoveEvent *event) override;
};


class PLANWORK_EXPORT AbstractView : public QWidget, public KPlato::ViewActionLists
{
    Q_OBJECT
public:
    AbstractView(Part *part, QWidget *parent);

    /// reimplement
    virtual void updateReadWrite(bool readwrite);
    /// reimplement
    virtual KPlato::Node *currentNode() const;
    /// reimplement
    virtual KPlato::Document *currentDocument() const;
    /// reimplement
    virtual QList<KPlato::Node*> selectedNodes() const;
    
    /// Loads context info into this view. Reimplement.
    virtual bool loadContext();
    /// Save context info from this view. Reimplement.
    virtual void saveContext();

    /// reimplement
    virtual KoPrintJob *createPrintJob();
    
Q_SIGNALS:
    void requestPopupMenu(const QString& name, const QPoint &pos);
    void selectionChanged();

protected Q_SLOTS:
    /// Builds menu from action list
    virtual void slotHeaderContextMenuRequested(const QPoint &pos);

    /// Reimplement if you have index specific context menu, standard calls slotHeaderContextMenuRequested()
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);

    /// Should not need to be reimplemented
    virtual void slotContextMenuRequested(KPlato::Node *node, const QPoint& pos);
    /// Should not need to be reimplemented
    virtual void slotContextMenuRequested(KPlato::Document *doc, const QPoint& pos);

    /// Calls  saveContext(), connect to this to have configuration saved
    virtual void sectionsMoved();

protected:
    Part *m_part;

};

class PLANWORK_EXPORT TaskWorkPackageView : public AbstractView
{
    Q_OBJECT
public:
    TaskWorkPackageView(Part *part, QWidget *parent);
    ~TaskWorkPackageView() override;

    void setupGui();

    TaskWorkPackageModel *itemModel() const { return m_view->itemModel(); }

    void updateReadWrite(bool readwrite) override;
    KPlato::Node *currentNode() const override;
    KPlato::Document *currentDocument() const override;
    QList<KPlato::Node*> selectedNodes() const override;

    /// Loads context info into this view. Reimplement.
    bool loadContext() override;
    /// Save context info from this view. Reimplement.
    void saveContext() override;

    using AbstractView::slotContextMenuRequested;

protected Q_SLOTS:
    void slotOptions();
    void slotSplitView();

    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotSelectionChanged(const QModelIndexList &lst);

protected:
    void updateActionsEnabled(bool on);

private:
    TaskWorkPackageTreeView *m_view;

};

//-------------
class GanttItemDelegate : public KPlato::GanttItemDelegate
{
    Q_OBJECT
public:
    enum Brushes { Brush_Normal, Brush_Late, Brush_NotScheduled, Brush_Finished, Brush_NotReadyToStart, Brush_ReadyToStart };

    explicit GanttItemDelegate(QObject *parent = nullptr);

    void paintGanttItem(QPainter* painter, const KGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) override;
    QString toolTip(const QModelIndex &idx) const override;

protected:
    bool showStatus;
    QMap<int, QBrush> m_brushes;
};

class GanttView : public KPlato::GanttViewBase
{
    Q_OBJECT
public:
    GanttView(Part *part, QWidget *parent);
    ~GanttView() override;

    TaskWorkPackageModel *itemModel() const;
    void setProject(KPlato::Project *project);
    KPlato::Project *project() const { return m_project; }

    GanttItemDelegate *delegate() const { return m_ganttdelegate; }

    QList<KPlato::Node*> selectedNodes() const;
    KPlato::Node *currentNode() const;
    KPlato::Document *currentDocument() const;

    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &context) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &context) const override;

Q_SIGNALS:
    void headerContextMenuRequested(const QPoint&);
    void selectionChanged(const QModelIndexList&);
    void sectionsMoved();

protected Q_SLOTS:
    void slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void slotRowsInserted(const QModelIndex &parent, int start, int end);
    void slotRowsRemoved(const QModelIndex &parent, int start, int end);

    void updateDateTimeGrid(KPlatoWork::WorkPackage *wp);

protected:
    Part *m_part;
    KPlato::Project *m_project;
    GanttItemDelegate *m_ganttdelegate;
    TaskWorkPackageModel *m_itemmodel;
    KGantt::TreeViewRowController *m_rowController;
};

class PLANWORK_EXPORT TaskWPGanttView : public AbstractView
{
    Q_OBJECT
public:
    TaskWPGanttView(Part *part, QWidget *parent);
    ~TaskWPGanttView() override;

    void setupGui();

    TaskWorkPackageModel *itemModel() const { return m_view->itemModel(); }

    KPlato::Node *currentNode() const override;
    QList<KPlato::Node*> selectedNodes() const override;
    KPlato::Document *currentDocument() const override;

    /// Loads context info into this view. Reimplement.
    bool loadContext() override;
    /// Save context info from this view. Reimplement.
    void saveContext() override;

    using AbstractView::slotContextMenuRequested;

protected Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotSelectionChanged(const QModelIndexList &lst);
    void slotOptions();

private:
    GanttView *m_view;

};

} //namespace KPlatoWork


#endif
