/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TASKSTATUSMODEL_H
#define TASKSTATUSMODEL_H

#include "planmodels_export.h"

#include "kptitemmodelbase.h"
#include "kptnodeitemmodel.h"

namespace KPlato
{

class Project;
class Node;
class Task;

typedef QMap<QString, Node*> NodeMap;

class PLANMODELS_EXPORT TaskStatusItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit TaskStatusItemModel(QObject *parent = nullptr);
    ~TaskStatusItemModel() override;
    
    enum PeriodType { UseCurrentDate, UseWeekday };
    int periodType() const { return m_periodType; }
    void setPeriodType(int type) { m_periodType = type; }
    
    /// Returns a column number/- name map for this model
    const QMetaEnum columnMap() const override { return m_nodemodel.columnMap(); }

    void setProject(Project *project) override;
    
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    
    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    virtual QModelIndex index(const Node *node) const;
    virtual QModelIndex index(const NodeMap *lst) const;
    
    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 
    
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;

    
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    QMimeData * mimeData(const QModelIndexList & indexes) const override;
    QStringList mimeTypes () const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    NodeMap *list(const QModelIndex &index) const;
    Node *node(const QModelIndex &index) const;
    QAbstractItemDelegate *createDelegate(int column, QWidget *parent) const override;
    
    NodeMap nodeList(QDataStream &stream);
    using ItemModelBase::dropAllowed;
    bool dropAllowed(Node *on, const QMimeData *data);
    
    void clear();
    
    void setNow();
    void setPeriod(int days) { m_period = days; }
    int period() const { return m_period; }
    void setWeekday(int day) { m_weekday = day; }
    int weekday() const { return m_weekday; }
    
    /// Return the sortorder to be used for @p column
    int sortRole(int column) const override;

public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void refresh() override;

protected Q_SLOTS:
    void slotAboutToBeReset();
    void slotReset();

    void slotNodeChanged(KPlato::Node*);
    void slotNodeToBeInserted(KPlato::Node *node, int row);
    void slotNodeInserted(KPlato::Node *node);
    void slotNodeToBeRemoved(KPlato::Node *node);
    void slotNodeRemoved(KPlato::Node *node);
    void slotNodeToBeMoved(KPlato::Node *node, int pos, KPlato::Node *newParent, int newPos);
    void slotNodeMoved(KPlato::Node *node);

    void slotWbsDefinitionChanged();

protected:

    // keep in sync with order in m_top
    enum TaskStatus {
        TaskUnknownStatus = -1,
        TaskNotStarted = 0,
        TaskRunning = 1,
        TaskFinished = 2,
        TaskUpcoming = 3
    };

    QVariant alignment(int column) const;

    QVariant name(int row, int role) const;
    TaskStatusItemModel::TaskStatus taskStatus(const Task *task, const QDate &begin, const QDate &end);

    bool setCompletion(Node *node, const QVariant &value, int role);
    bool setRemainingEffort(Node *node, const QVariant &value, int role);
    bool setActualEffort(Node *node, const QVariant &value, int role);
    bool setStartedTime(Node *node, const QVariant &value, int role);
    bool setFinishedTime(Node *node, const QVariant &value, int role);

private:
    NodeModel m_nodemodel;
    QStringList m_topNames;
    QStringList m_topTips;
    QList<NodeMap*> m_top;
    NodeMap m_notstarted;
    NodeMap m_running;
    NodeMap m_finished;
    NodeMap m_upcoming;
    
    long m_id; // schedule id
    int m_period; // days
    int m_periodType;
    int m_weekday;

};

} //namespace KPlato


#endif
