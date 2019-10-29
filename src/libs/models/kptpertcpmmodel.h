/* This file is part of the KDE project
  Copyright (C) 2007 Dag Andersen <danders@get2net>

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

#ifndef KPTPERTCPMMODEL_H
#define KPTPERTCPMMODEL_H

#include "planmodels_export.h"

#include <kptitemmodelbase.h>
#include <kptnodeitemmodel.h>


/// The main namespace
namespace KPlato
{

class Duration;
class Node;
class Project;
class ScheduleManager;
class Task;

typedef QList<Node*> NodeList;

class PLANMODELS_EXPORT CriticalPathItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit CriticalPathItemModel(QObject *parent = 0);
    ~CriticalPathItemModel() override;
    
    const QMetaEnum columnMap() const override { return m_nodemodel.columnMap(); }
    
    void setProject(Project *project) override;
    
    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    
    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 
    
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    Node *node(const QModelIndex &index) const;
    void setManager(ScheduleManager *sm);
    ScheduleManager *manager() const { return m_manager; }

    /// Select a proper unit for total path values, dependent on @p duration
    Duration::Unit presentationUnit(const Duration &duration) const;
    
protected Q_SLOTS:
    void slotNodeChanged(KPlato::Node*);
    void slotNodeToBeInserted(KPlato::Node *node, int row);
    void slotNodeInserted(KPlato::Node *node);
    void slotNodeToBeRemoved(KPlato::Node *node);
    void slotNodeRemoved(KPlato::Node *node);

public:
    QVariant alignment(int column) const;
    
    QVariant name(int role) const;
    QVariant duration(int role) const;
    QVariant variance(int role) const;
    
    QVariant notUsed(int role) const;

private:
    ScheduleManager *m_manager;
    QList<Node*> m_path;
    NodeModel m_nodemodel;
};

//--------------------

/**
 This model displays results from project scheduling.
*/
class PLANMODELS_EXPORT PertResultItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit PertResultItemModel(QObject *parent = 0);
    ~PertResultItemModel() override;
    
    const QMetaEnum columnMap() const override { return m_nodemodel.columnMap(); }
    
    void setProject(Project *project) override;
    
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    
    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
//    virtual QModelIndex index(const Node *node) const;
    virtual QModelIndex index(const NodeList *lst) const;
    
    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 
    
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    QMimeData * mimeData(const QModelIndexList & indexes) const override;
    QStringList mimeTypes () const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    NodeList *list(const QModelIndex &index) const;
    Node *node(const QModelIndex &index) const;
    QAbstractItemDelegate *createDelegate(int column, QWidget *parent) const override;
    
    NodeList nodeList(QDataStream &stream);
    using ItemModelBase::dropAllowed;
    bool dropAllowed(Node *on, const QMimeData *data);
    
    void clear();
    void refresh() override;
    
    void setManager(ScheduleManager *sm);
    ScheduleManager *manager() const { return m_manager; }
    
protected Q_SLOTS:
    void slotAboutToBeReset();
    void slotReset();

    void slotNodeChanged(KPlato:: Node*);
    void slotNodeToBeInserted(KPlato:: Node *node, int row);
    void slotNodeInserted(KPlato:: Node *node);
    void slotNodeToBeRemoved(KPlato:: Node *node);
    void slotNodeRemoved(KPlato:: Node *node);

protected:
    QVariant alignment(int column) const;
    
    QVariant name(int row, int role) const;
    QVariant name(const Node *node, int role) const;
    QVariant earlyStart(const Task *node, int role) const;
    QVariant earlyFinish(const Task *node, int role) const;
    QVariant lateStart(const Task *node, int role) const;
    QVariant lateFinish(const Task *node, int role) const;
    QVariant positiveFloat(const Task *node, int role) const;
    QVariant freeFloat(const Task *node, int role) const;
    QVariant negativeFloat(const Task *node, int role) const;
    QVariant startFloat(const Task *node, int role) const;
    QVariant finishFloat(const Task *node, int role) const;

private:
    QStringList m_topNames;
    QList<NodeList*> m_top;
    NodeList m_cp;
    NodeList m_critical;
    NodeList m_noncritical;
    NodeList m_dummyList;
    
    ScheduleManager *m_manager;
    NodeModel m_nodemodel;
};

}  //KPlato namespace

#endif
