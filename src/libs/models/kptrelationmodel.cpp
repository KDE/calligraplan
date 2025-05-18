/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptrelationmodel.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptduration.h"
#include "kptproject.h"
#include "kptnode.h"
#include "kptrelation.h"
#include "kptdebug.h"

#include <QModelIndex>
#include <QWidget>
#include <QLocale>


namespace KPlato
{

QVariant RelationModel::parentName(const Relation *r, int role) const
{
    //debugPlan<<r<<", "<<role<<'\n';
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::EditRole:
            return r->parent()->name();
        case Qt::TextAlignmentRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant RelationModel::childName(const Relation *r, int role) const
{
    //debugPlan<<r<<", "<<role<<'\n';
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::EditRole:
            return r->child()->name();
        case Qt::TextAlignmentRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant RelationModel::type(const Relation *r, int role) const
{
    //debugPlan<<r<<", "<<role<<'\n';
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return r->typeToString(true);
        case Role::EnumList:
            return r->typeList(true);
        case Qt::EditRole:
        case Role::EnumListValue:
            return (int)r->type();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant RelationModel::lag(const Relation *r, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole: {
            Duration::Unit unit = Duration::Unit_h;
            return QVariant(QLocale().toString(r->lag().toDouble(unit), 'f', 1) +  Duration::unitToString(unit, true));
        }
        case Qt::EditRole:
            return r->lag().toDouble(Duration::Unit_h);
        case Role::DurationUnit:
            return static_cast<int>(Duration::Unit_h);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant RelationModel::data(const Relation *r, int property, int role) const
{
    QVariant result;
    switch (property) {
        case 0: result = parentName(r, role); break;
        case 1: result = childName(r, role); break;
        case 2: result = type(r, role); break;
        case 3: result = lag(r, role); break;
        default:
            //debugPlan<<"Invalid property number: "<<property<<'\n';
            return result;
    }
    return result;
}

int RelationModel::propertyCount()
{
    return 4;
}

QVariant RelationModel::headerData(int section, int role)
{
    if (role == Qt::DisplayRole) {
        switch (section) {
            case 0: return i18n("Parent");
            case 1: return i18n("Child");
            case 2: return i18n("Type");
            case 3: return i18n("Lag");
            default: return QVariant();
        }
    }
    if (role == Qt::ToolTipRole) {
        switch (section) {
            case 0: return ToolTip::relationParent();
            case 1: return ToolTip::relationChild();
            case 2: return ToolTip::relationType();
            case 3: return ToolTip::relationLag();
            default: return QVariant();
        }
    }
    return QVariant();
}

//----------------------------
RelationItemModel::RelationItemModel(QObject *parent)
    : ItemModelBase(parent),
    m_node(nullptr),
    m_removedRelation(nullptr)
{
}

RelationItemModel::~RelationItemModel()
{
}

void RelationItemModel::slotRelationToBeAdded(Relation *relation, int, int)
{
    debugPlan;
    if (m_node == nullptr || m_node != relation->child()) {
        return;
    }
    // relations always appended
    int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
}

void RelationItemModel::slotRelationAdded(Relation *relation)
{
    debugPlan;
    if (m_node == nullptr || m_node != relation->child()) {
        return;
    }
    endInsertRows();
}

void RelationItemModel::slotRelationToBeRemoved(Relation *relation)
{
    if (m_node == nullptr || ! m_node->dependParentNodes().contains(relation)) {
        return;
    }
    m_removedRelation = relation;
    int row = m_node->dependParentNodes().indexOf(relation);
    debugPlan<<row;
    beginRemoveRows(QModelIndex(), row, row);
}

void RelationItemModel::slotRelationRemoved(Relation *relation)
{
    debugPlan;
    if (m_removedRelation != relation) {
        return;
    }
    m_removedRelation = nullptr;
    endRemoveRows();
}

void RelationItemModel::slotRelationModified(Relation *relation)
{
    debugPlan;
    if (m_node == nullptr || ! m_node->dependParentNodes().contains(relation)) {
        return;
    }
    int row = m_node->dependParentNodes().indexOf(relation);
    Q_EMIT dataChanged(createIndex(row, 0), createIndex(row, columnCount()-1));
}

void RelationItemModel::slotNodeToBeRemoved(Node *node)
{
    if (node != m_node) {
        return;
    }
    setNode(nullptr);
}

void RelationItemModel::slotNodeRemoved(Node *node)
{
    Q_UNUSED(node);
}

void RelationItemModel::slotLayoutChanged()
{
    //debugPlan<<node->name()<<'\n';
    Q_EMIT layoutAboutToBeChanged();
    Q_EMIT layoutChanged();
}

void RelationItemModel::setProject(Project *project)
{
    beginResetModel();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &RelationItemModel::projectDeleted);
        disconnect(m_project, &Project::nodeChanged, this, &RelationItemModel::slotNodeChanged);
        disconnect(m_project, &Project::nodeToBeRemoved, this, &RelationItemModel::slotNodeToBeRemoved);

        disconnect(m_project, &Project::relationToBeAdded, this, &RelationItemModel::slotRelationToBeAdded);
        disconnect(m_project, &Project::relationAdded, this, &RelationItemModel::slotRelationAdded);

        disconnect(m_project, &Project::relationToBeRemoved, this, &RelationItemModel::slotRelationToBeRemoved);
        disconnect(m_project, &Project::relationRemoved, this, &RelationItemModel::slotRelationRemoved);

        disconnect(m_project, &Project::relationModified, this, &RelationItemModel::slotRelationModified);
    }
    m_project = project;
    if (project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &RelationItemModel::projectDeleted);
        connect(m_project, &Project::nodeChanged, this, &RelationItemModel::slotNodeChanged);
        connect(m_project, &Project::nodeToBeRemoved, this, &RelationItemModel::slotNodeToBeRemoved);

        connect(m_project, &Project::relationToBeAdded, this, &RelationItemModel::slotRelationToBeAdded);
        connect(m_project, &Project::relationAdded, this, &RelationItemModel::slotRelationAdded);

        connect(m_project, &Project::relationToBeRemoved, this, &RelationItemModel::slotRelationToBeRemoved);
        connect(m_project, &Project::relationRemoved, this, &RelationItemModel::slotRelationRemoved);

        connect(m_project, &Project::relationModified, this, &RelationItemModel::slotRelationModified);
    }
    endResetModel();
}

void RelationItemModel::setNode(Node *node)
{
    beginResetModel();
    m_node = node;
    endResetModel();
}

Qt::ItemFlags RelationItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        if (m_readWrite) {
            flags |= Qt::ItemIsDropEnabled;
        }
        return flags;
    }
    if (m_readWrite) {
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        switch (index.column()) {
            case 2: // type
                flags |= Qt::ItemIsEditable;
                break;
            case 3: // lag
                flags |= Qt::ItemIsEditable;
                break;
            default:
                flags &= ~Qt::ItemIsEditable;
                break;
        }
    }
    return flags;
}


QModelIndex RelationItemModel::parent(const QModelIndex &/*index*/) const
{
    return QModelIndex(); // flat model
}

QModelIndex RelationItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr) {
        return QModelIndex();
    }
    if (parent.isValid()) {
        return QModelIndex(); // flat model
    }
    return createIndex(row, column);
}

bool RelationItemModel::setType(Relation *r, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            Relation::Type v = Relation::Type(value.toInt());
            //debugPlan<<v<<r->type();
            if (v == r->type()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyRelationTypeCmd(r, v, kundo2_i18n("Modify relation type")));
            return true;
    }
    return false;
}

bool RelationItemModel::setLag(Relation *r, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Duration::Unit unit = static_cast<Duration::Unit>(value.toList()[1].toInt());
            Duration d(value.toList()[0].toDouble(), unit);
            debugPlan<<value.toList()[0].toDouble()<<","<<unit<<" ->"<<d.toString();
            if (d == r->lag()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyRelationLagCmd(r, d, kundo2_i18n("Modify relation time lag")));
            return true;
        }
        default:
            break;
    }
    return false;
}

QVariant RelationItemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole) {
        return headerData(index.column(), Qt::Horizontal, role);
    }

    QVariant result;
    Relation *r = relation(index);
    if (r != nullptr) {
        result = m_relationmodel.data(r, index.column(), role);
    }
    if (result.isValid()) {
        if (role == Qt::DisplayRole && result.typeId() == QMetaType::QString && result.toString().isEmpty()) {
            // HACK to show focus in empty cells
            result = ' ';
        }
        return result;
    }
    return result;
}

bool RelationItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (! index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    if ((flags(index) & Qt::ItemIsEditable) == 0 || role != Qt::EditRole) {
        return false;
    }
    Relation *r = relation(index);
    switch (index.column()) {
        case 0: return false;
        case 1: return false;
        case 2: return setType(r, value, role);
        case 3: return setLag(r, value, role);
        default:
            qWarning("data: invalid display value column %d", index.column());
            return false;
    }
    return false;
}

QVariant RelationItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            return m_relationmodel.headerData(section, role);
        } else if (role == Qt::TextAlignmentRole) {
            switch (section) {
                case 2: return Qt::AlignCenter;
                case 3: return Qt::AlignRight;
                default: return QVariant();
            }
        }
    }
    if (role == Qt::ToolTipRole) {
        return RelationModel::headerData(section, role);
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QAbstractItemDelegate *RelationItemModel::createDelegate(int column, QWidget *parent) const
{
    switch (column) {
        case 2: return new EnumDelegate(parent);
        case 3: return new DurationSpinBoxDelegate(parent);
        default: return nullptr;
    }
    return nullptr;
}

int RelationItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return m_relationmodel.propertyCount();
}

int RelationItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == nullptr || m_node == nullptr || parent.isValid()) {
        return 0;
    }
    return m_node->numDependParentNodes();
}

Relation *RelationItemModel::relation(const QModelIndex &index) const
{
    if (! index.isValid() || m_node == nullptr) {
        return nullptr;
    }
    return m_node->dependParentNodes().value(index.row());
}

void RelationItemModel::slotNodeChanged(Node *node)
{
    Q_UNUSED(node);
    beginResetModel();
    endResetModel();
}


} //namespace KPlato
