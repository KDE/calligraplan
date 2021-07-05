/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef RELATIONMODEL_H
#define RELATIONMODEL_H

#include "kptitemmodelbase.h"
#include "kptschedule.h"


class QModelIndex;
class KUndo2Command;

namespace KPlato
{

class Project;
class Node;
class Relation;

class PLANMODELS_EXPORT RelationModel : public QObject
{
    Q_OBJECT
public:
    RelationModel()
        : QObject()
    {}
    ~RelationModel() override {}
    
    QVariant data(const Relation *relation, int property, int role = Qt::DisplayRole) const; 
    
    static QVariant headerData(int section, int role = Qt::DisplayRole);

    static int propertyCount();
    
    QVariant parentName(const Relation *r, int role) const;
    QVariant childName(const Relation *r, int role) const;
    QVariant type(const Relation *r, int role) const;
    QVariant lag(const Relation *r, int role) const;

};

class PLANMODELS_EXPORT RelationItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit RelationItemModel(QObject *parent = nullptr);
    ~RelationItemModel() override;
    
    void setProject(Project *project) override;
    virtual void setNode(Node *node);
    
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    
    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    
    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 
    
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;

    
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    Relation *relation(const QModelIndex &index) const;
    QAbstractItemDelegate *createDelegate(int column, QWidget *parent) const override;

protected Q_SLOTS:
    void slotNodeChanged(KPlato::Node*);
    void slotNodeToBeRemoved(KPlato::Node *node);
    void slotNodeRemoved(KPlato::Node *node);
    void slotRelationToBeRemoved(KPlato::Relation *r);
    void slotRelationRemoved(KPlato::Relation *r);
    void slotRelationToBeAdded(KPlato::Relation *r, int, int);
    void slotRelationAdded(KPlato::Relation *r);
    void slotRelationModified(KPlato::Relation *r);

    void slotLayoutChanged() override;
    
protected:
    bool setType(Relation *r, const QVariant &value, int role);
    bool setLag(Relation *r, const QVariant &value, int role);
    
private:
    Node *m_node;
    RelationModel m_relationmodel;
    
    Relation *m_removedRelation; // to control endRemoveRows()
};

} //namespace KPlato

#endif
