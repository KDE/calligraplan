/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptpertcpmmodel.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptnode.h"
#include "kptschedule.h"
#include "kptdebug.h"

#include <KLocalizedString>

#include <QStringList>
#include <QMimeData>
#include <QLocale>


namespace KPlato
{

class Project;
class Node;
class Task;

typedef QList<Node*> NodeList;

// TODO: find some better values
static const quintptr ListItemId = static_cast<quintptr>(-1);
static const quintptr ProjectItemId  = static_cast<quintptr>(-2);

CriticalPathItemModel::CriticalPathItemModel(QObject *parent)
    : ItemModelBase(parent),
    m_manager(nullptr)
{
/*    connect(this, SIGNAL(modelAboutToBeReset()), SLOT(slotAboutToBeReset()));
    connect(this, SIGNAL(modelReset()), SLOT(slotReset()));*/
}

CriticalPathItemModel::~CriticalPathItemModel()
{
}

void CriticalPathItemModel::slotNodeToBeInserted(Node *, int)
{
    //debugPlan<<node->name();
}

void CriticalPathItemModel::slotNodeInserted(Node * /*node*/)
{
    //debugPlan<<node->getParent->name()<<"-->"<<node->name();
}

void CriticalPathItemModel::slotNodeToBeRemoved(Node *node)
{
    Q_UNUSED(node);
    //debugPlan<<node->name();
/*    if (m_path.contains(node)) {
    }*/
}

void CriticalPathItemModel::slotNodeRemoved(Node *node)
{
    Q_UNUSED(node);
    //debugPlan<<node->name();
}

void CriticalPathItemModel::setProject(Project *project)
{
    beginResetModel();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &CriticalPathItemModel::projectDeleted);
        disconnect(m_project, &Project::nodeChanged, this, &CriticalPathItemModel::slotNodeChanged);
        disconnect(m_project, &Project::nodeToBeAdded, this, &CriticalPathItemModel::slotNodeToBeInserted);
        disconnect(m_project, &Project::nodeToBeRemoved, this, &CriticalPathItemModel::slotNodeToBeRemoved);
        disconnect(m_project, &Project::nodeToBeMoved, this, &CriticalPathItemModel::slotLayoutToBeChanged);

        disconnect(m_project, &Project::nodeAdded, this, &CriticalPathItemModel::slotNodeInserted);
        disconnect(m_project, &Project::nodeRemoved, this, &CriticalPathItemModel::slotNodeRemoved);
        disconnect(m_project, &Project::nodeMoved, this, &CriticalPathItemModel::slotLayoutChanged);
    }
    m_project = project;
    m_nodemodel.setProject(project);
    if (project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &CriticalPathItemModel::projectDeleted);
        connect(m_project, &Project::nodeChanged, this, &CriticalPathItemModel::slotNodeChanged);
        connect(m_project, &Project::nodeToBeAdded, this, &CriticalPathItemModel::slotNodeToBeInserted);
        connect(m_project, &Project::nodeToBeRemoved, this, &CriticalPathItemModel::slotNodeToBeRemoved);
        connect(m_project, &Project::nodeToBeMoved, this, &CriticalPathItemModel::slotLayoutToBeChanged);

        connect(m_project, &Project::nodeAdded, this, &CriticalPathItemModel::slotNodeInserted);
        connect(m_project, &Project::nodeRemoved, this, &CriticalPathItemModel::slotNodeRemoved);
        connect(m_project, &Project::nodeMoved, this, &CriticalPathItemModel::slotLayoutChanged);
    }
    endResetModel();
}

void CriticalPathItemModel::setManager(ScheduleManager *sm)
{
    beginResetModel();
    debugPlan<<this;
    m_manager = sm;
    m_nodemodel.setManager(sm);
    if (m_project == nullptr || m_manager == nullptr) {
        m_path.clear();
    } else {
        m_path = m_project->criticalPath(m_manager->scheduleId(), 0);
    }
    debugPlan<<m_path;
    endResetModel();
}

QModelIndex CriticalPathItemModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

QModelIndex CriticalPathItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    if (parent.isValid()) {
        return QModelIndex();
    }
    Node *n = m_path.value(row);
    QModelIndex i = createIndex(row, column, n);
    return i;
}

Duration::Unit CriticalPathItemModel::presentationUnit(const Duration &dur) const
{
    if (dur.toDouble(Duration::Unit_d) < 1.0) {
        return Duration::Unit_h;
    }
    return Duration::Unit_d;
}

QVariant CriticalPathItemModel::name(int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return i18n("Path");
        case Qt::ToolTipRole:
        case Qt::EditRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant CriticalPathItemModel::duration(int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole: {
            Duration v = m_project->duration(m_manager->scheduleId());
            return QVariant(QLocale().toString(v.toDouble(presentationUnit(v)), 'f', 1) + Duration::unitToString(presentationUnit(v)));
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant CriticalPathItemModel::variance(int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole: {
            double v = 0.0;
            for (Node *n : std::as_const(m_path)) {
                long id = m_manager->scheduleId();
                v += n->variance(id, presentationUnit(m_project->duration(id)));
            }
            return QLocale().toString(v, 'f', 1);
            break;
        }
        case Qt::EditRole: {
            double v = 0.0;
            for (Node *n : std::as_const(m_path)) {
                v += n->variance(m_manager->scheduleId());
            }
            return v;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant CriticalPathItemModel::notUsed(int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return QLatin1String("");
        default:
            return QVariant();
    }
    return QVariant();
}

QVariant CriticalPathItemModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    if (! index.isValid()) {
        return result;
    }
    if (role == Qt::TextAlignmentRole) {
        return alignment(index.column());
    }
    Node *n = node(index);
    if (n == nullptr) {
        switch (index.column()) {
            case NodeModel::NodeName: result = name(role); break;
            case NodeModel::NodeDuration: result = duration(role); break;
            case NodeModel::NodeVarianceDuration: result = variance(role); break;
            default:
                result = notUsed(role); break;
        }
    } else  {
        result = m_nodemodel.data(n, index.column(), role);
    }
    if (result.isValid()) {
        if (role == Qt::DisplayRole && result.type() == QVariant::String && result.toString().isEmpty()) {
            // HACK to show focus in empty cells
            result = ' ';
        }
        return result;
    }
    return result;
}

QVariant CriticalPathItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            return m_nodemodel.headerData(section, role);
        } else if (role == Qt::TextAlignmentRole) {
            return alignment(section);
        }
    }
    if (role == Qt::ToolTipRole) {
        return m_nodemodel.headerData(section, role);
    } else if (role == Qt::WhatsThisRole) {
        return m_nodemodel.headerData(section, role);
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QVariant CriticalPathItemModel::alignment(int column) const
{
    return m_nodemodel.headerData(column, Qt::TextAlignmentRole);
}

int CriticalPathItemModel::columnCount(const QModelIndex &) const
{
    return m_nodemodel.propertyCount();
}

int CriticalPathItemModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    if (m_manager && m_manager->expected() && m_manager->expected()->criticalPathList()) {
        return m_path.count() + 1;
    }
    return 0;
}

Node *CriticalPathItemModel::node(const QModelIndex &index) const
{
    if (! index.isValid()) {
        return nullptr;
    }
    return m_path.value(index.row());
}

void CriticalPathItemModel::slotNodeChanged(Node *node)
{
    debugPlan;
    if (node == nullptr || node->type() == Node::Type_Project || ! m_path.contains(node)) {
        return;
    }
    int row = m_path.indexOf(node);
    Q_EMIT dataChanged(createIndex(row, 0, node), createIndex(row, columnCount() - 1, node));
}


//-----------------------------
PertResultItemModel::PertResultItemModel(QObject *parent)
    : ItemModelBase(parent),
    m_manager(nullptr)
{
/*    connect(this, SIGNAL(modelAboutToBeReset()), SLOT(slotAboutToBeReset()));
    connect(this, SIGNAL(modelReset()), SLOT(slotReset()));*/
}

PertResultItemModel::~PertResultItemModel()
{
}

void PertResultItemModel::slotAboutToBeReset()
{
    debugPlan;
    clear();
}

void PertResultItemModel::slotReset()
{
    debugPlan;
    refresh();
}

void PertResultItemModel::slotNodeToBeInserted(Node *, int)
{
    //debugPlan<<node->name();
    clear();
}

void PertResultItemModel::slotNodeInserted(Node * /*node*/)
{
    //debugPlan<<node->getParent->name()<<"-->"<<node->name();
    refresh();
}

void PertResultItemModel::slotNodeToBeRemoved(Node * /*node*/)
{
    //debugPlan<<node->name();
    clear();
}

void PertResultItemModel::slotNodeRemoved(Node * /*node*/)
{
    //debugPlan<<node->name();
    refresh();
}

void PertResultItemModel::setProject(Project *project)
{
    clear();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &PertResultItemModel::projectDeleted);
        disconnect(m_project, &Project::nodeChanged, this, &PertResultItemModel::slotNodeChanged);
        disconnect(m_project, &Project::nodeToBeAdded, this, &PertResultItemModel::slotNodeToBeInserted);
        disconnect(m_project, &Project::nodeToBeRemoved, this, &PertResultItemModel::slotNodeToBeRemoved);
        disconnect(m_project, &Project::nodeToBeMoved, this, &PertResultItemModel::slotLayoutToBeChanged);

        disconnect(m_project, &Project::nodeAdded, this, &PertResultItemModel::slotNodeInserted);
        disconnect(m_project, &Project::nodeRemoved, this, &PertResultItemModel::slotNodeRemoved);
        disconnect(m_project, &Project::nodeMoved, this, &PertResultItemModel::slotLayoutChanged);
    }
    m_project = project;
    m_nodemodel.setProject(project);
    if (project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &PertResultItemModel::projectDeleted);
        connect(m_project, &Project::nodeChanged, this, &PertResultItemModel::slotNodeChanged);
        connect(m_project, &Project::nodeToBeAdded, this, &PertResultItemModel::slotNodeToBeInserted);
        connect(m_project, &Project::nodeToBeRemoved, this, &PertResultItemModel::slotNodeToBeRemoved);
        connect(m_project, &Project::nodeToBeMoved, this, &PertResultItemModel::slotLayoutToBeChanged);

        connect(m_project, &Project::nodeAdded, this, &PertResultItemModel::slotNodeInserted);
        connect(m_project, &Project::nodeRemoved, this, &PertResultItemModel::slotNodeRemoved);
        connect(m_project, &Project::nodeMoved, this, &PertResultItemModel::slotLayoutChanged);
    }
    refresh();
}

void PertResultItemModel::setManager(ScheduleManager *sm)
{
    m_manager = sm;
    m_nodemodel.setManager(sm);
    refresh();
}

void PertResultItemModel::clear()
{
    debugPlan<<this;
    for (NodeList *l : std::as_const(m_top)) {
        int c = l->count();
        if (c > 0) {
            // FIXME: gives error msg:
            // Can't select indexes from different model or with different parents
            QModelIndex i = index(l);
            debugPlan<<i<<": "<<c;
//            beginRemoveRows(i, 0, c-1);
//            endRemoveRows();
        }
    }
    m_critical.clear();
    m_noncritical.clear();
    if (! m_top.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_top.count() - 1);
        m_top.clear();
        m_topNames.clear();
        endRemoveRows();
    }
}

void PertResultItemModel::refresh()
{
    clear();
    if (m_project == nullptr) {
        return;
    }
    long id = m_manager == nullptr ? -1 : m_manager->scheduleId();
    debugPlan<<id;
    if (id == -1) {
        return;
    }
    m_topNames << i18n("Project");
    m_top << &m_dummyList; // dummy
    const QList< NodeList > *lst = m_project->criticalPathList(id);
    if (lst) {
        for (int i = 0; i < lst->count(); ++i) {
            m_topNames << i18n("Critical Path");
            m_top.append(const_cast<NodeList*>(&(lst->at(i))));
            debugPlan<<m_topNames.last()<<lst->at(i);
        }
        if (lst->isEmpty()) debugPlan<<"No critical path";
    }
    const QList<Node*> nodes = m_project->allNodes();
    for (Node* n : nodes) {
        if (n->type() != Node::Type_Task && n->type() != Node::Type_Milestone) {
            continue;
        }
        Task *t = static_cast<Task*>(n);
        if (t->inCriticalPath(id)) {
            continue;
        } else if (t->isCritical(id)) {
            m_critical.append(t);
        } else {
            m_noncritical.append(t);
        }
    }
    if (! m_critical.isEmpty()) {
        m_topNames << i18n("Critical");
        m_top.append(&m_critical);
    }
    if (! m_noncritical.isEmpty()) {
        m_topNames << i18n("Non-critical");
        m_top.append(&m_noncritical);
    }
    if (! m_top.isEmpty()) {
        debugPlan<<m_top;
        beginInsertRows(QModelIndex(), 0, m_top.count() -1);
        endInsertRows();
        
        for (NodeList *l : std::as_const(m_top)) {
            int c = l->count();
            if (c > 0) {
                beginInsertRows(index(l), 0, c-1);
                endInsertRows();
            }
        }
    }
}

Qt::ItemFlags PertResultItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    flags &= ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    return flags;
}


QModelIndex PertResultItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    //debugPlan<<index.internalPointer()<<": "<<index.row()<<", "<<index.column();
    int row = index.internalId();
    if (row < 0) {
        return QModelIndex(); // top level has no parent
    }
    if (m_top.value(row) == nullptr) {
        return QModelIndex();
    }
    return createIndex(row, 0, ListItemId);
}

QModelIndex PertResultItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    if (! parent.isValid()) {
        if (row == 0) {
            QModelIndex idx = createIndex(row, column, ProjectItemId); // project
            return idx;
        }
        if (row >= m_top.count()) {
            return QModelIndex(); // shouldn't happened
        }
        QModelIndex idx = createIndex(row, column, ListItemId);
        //debugPlan<<parent<<", "<<idx;
        return idx;
    }
    if (parent.row() == 0) {
        return QModelIndex();
    }
    NodeList *l = m_top.value(parent.row());
    if (l == nullptr) {
        return QModelIndex();
    }
    QModelIndex i = createIndex(row, column, parent.row());
    return i;
}

// QModelIndex PertResultItemModel::index(const Node *node) const
// {
//     if (m_project == 0 || node == 0) {
//         return QModelIndex();
//     }
//     foreach(NodeList *l, m_top) {
//         int row = l->indexOf(const_cast<Node*>(node));
//         if (row != -1) {
//             return createIndex(row, 0, const_cast<Node*>(node));
//         }
//     }
//     return QModelIndex();
// }

QModelIndex PertResultItemModel::index(const NodeList *lst) const
{
    if (m_project == nullptr || lst == nullptr) {
        return QModelIndex();
    }
    NodeList *l = const_cast<NodeList*>(lst);
    int row = m_top.indexOf(l);
    if (row <= 0) {
        return QModelIndex();
    }
    return createIndex(row, 0, ListItemId);
}

QVariant PertResultItemModel::name(int row, int role) const
{

    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return m_topNames.value(row);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::name(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return node->name();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::earlyStart(const Task *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->earlyStart(m_manager->scheduleId());
        case Qt::ToolTipRole:
            return QLocale().toString(node->earlyStart(m_manager->scheduleId()).date(), QLocale::ShortFormat);
        case Qt::EditRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::earlyFinish(const Task *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->earlyFinish(m_manager->scheduleId());
        case Qt::ToolTipRole:
            return QLocale().toString(node->earlyFinish(m_manager->scheduleId()).date(), QLocale::ShortFormat);
        case Qt::EditRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::lateStart(const Task *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->lateStart(m_manager->scheduleId());
        case Qt::ToolTipRole:
            return QLocale().toString(node->lateStart(m_manager->scheduleId()).date(), QLocale::ShortFormat);
        case Qt::EditRole:
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::lateFinish(const Task *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->lateFinish(m_manager->scheduleId());
        case Qt::ToolTipRole:
            return QLocale().toString(node->lateFinish(m_manager->scheduleId()).date(), QLocale::ShortFormat);
        case Qt::EditRole:
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::positiveFloat(const Task *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->positiveFloat(m_manager->scheduleId()).toString(Duration::Format_i18nHourFraction);
        case Qt::ToolTipRole:
            return node->positiveFloat(m_manager->scheduleId()).toString(Duration::Format_i18nDayTime);
        case Qt::EditRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::freeFloat(const Task *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->freeFloat(m_manager->scheduleId()).toString(Duration::Format_i18nHourFraction);
        case Qt::ToolTipRole:
            return node->freeFloat(m_manager->scheduleId()).toString(Duration::Format_i18nDayTime);
        case Qt::EditRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::negativeFloat(const Task *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->negativeFloat(m_manager->scheduleId()).toString(Duration::Format_i18nHourFraction);
        case Qt::ToolTipRole:
            return node->negativeFloat(m_manager->scheduleId()).toString(Duration::Format_i18nDayTime);
        case Qt::EditRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::startFloat(const Task *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->startFloat(m_manager->scheduleId()).toString(Duration::Format_i18nHourFraction);
        case Qt::ToolTipRole:
            return node->startFloat(m_manager->scheduleId()).toString(Duration::Format_i18nDayTime);
        case Qt::EditRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::finishFloat(const Task *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->finishFloat(m_manager->scheduleId()).toString(Duration::Format_i18nHourFraction);
        case Qt::ToolTipRole:
            return node->finishFloat(m_manager->scheduleId()).toString(Duration::Format_i18nDayTime);
        case Qt::EditRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant PertResultItemModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    if (! index.isValid()) {
        return result;
    }
    if (role == Qt::TextAlignmentRole) {
        return alignment(index.column());
    }
    Node *n = node(index);
    if (n == nullptr) {
        switch (index.column()) {
            case 0: return name(index.row(), role);
            default: break;
        }
        return QVariant();
    }
    if (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone) {
        result = m_nodemodel.data(n, index.column(), role);
    }
    if (n->type() == Node::Type_Project) {
        //Project *p = static_cast<Project*>(n);
        switch (index.column()) {
            case NodeModel::NodeName: result = name(NodeModel::NodeName, role); break;
            default:
                //debugPlan<<"data: invalid display value column "<<index.column();
                return QVariant();
        }
    }
    if (result.isValid()) {
        if (role == Qt::DisplayRole && result.type() == QVariant::String && result.toString().isEmpty()) {
            // HACK to show focus in empty cells
            result = ' ';
        }
        return result;
    }
    return QVariant();
}

QVariant PertResultItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            return m_nodemodel.headerData(section, role);
        } else if (role == Qt::TextAlignmentRole) {
            return alignment(section);
        }
    }
    if (role == Qt::ToolTipRole) {
        return m_nodemodel.headerData(section, role);
    } else if (role == Qt::WhatsThisRole) {
        return m_nodemodel.headerData(section, role);
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QVariant PertResultItemModel::alignment(int column) const
{
    return m_nodemodel.headerData(column, Qt::TextAlignmentRole);
}

QAbstractItemDelegate *PertResultItemModel::createDelegate(int column, QWidget * /*parent*/) const
{
    switch (column) {
        default: return nullptr;
    }
    return nullptr;
}

int PertResultItemModel::columnCount(const QModelIndex &) const
{
    return m_nodemodel.propertyCount();
}

int PertResultItemModel::rowCount(const QModelIndex &parent) const
{
    if (! parent.isValid()) {
        //debugPlan<<"top="<<m_top.count();
        return m_top.count();
    }
    NodeList *l = list(parent);
    if (l) {
        //debugPlan<<"list "<<parent.row()<<": "<<l->count();
        return l->count();
    }
    //debugPlan<<"node "<<parent.row();
    return 0; // nodes don't have children
}

Qt::DropActions PertResultItemModel::supportedDropActions() const
{
    return (Qt::DropActions)Qt::CopyAction | Qt::MoveAction;
}


QStringList PertResultItemModel::mimeTypes() const
{
    return QStringList();
}

QMimeData *PertResultItemModel::mimeData(const QModelIndexList &) const
{
    QMimeData *m = new QMimeData();
    return m;
}

bool PertResultItemModel::dropAllowed(Node *, const QMimeData *)
{
    return false;
}

bool PertResultItemModel::dropMimeData(const QMimeData *, Qt::DropAction , int , int , const QModelIndex &)
{
    return false;
}

NodeList *PertResultItemModel::list(const QModelIndex &index) const
{
    if (index.isValid() && index.internalId() == ListItemId) {
        //debugPlan<<index<<"is list: "<<m_top.value(index.row());
        return m_top.value(index.row());
    }
    //debugPlan<<index<<"is not list";
    return nullptr;
}

Node *PertResultItemModel::node(const QModelIndex &index) const
{
    if (! index.isValid()) {
        return nullptr;
    }
    if (index.internalId() == ProjectItemId) {
        return m_project;
    }
    if (index.internalId() == 0) {
        return nullptr;
    }
    NodeList *l = m_top.value(index.internalId());
    if (l) {
        return l->value(index.row());
    }
    return nullptr;
}

void PertResultItemModel::slotNodeChanged(Node *)
{
    debugPlan;
    refresh();
/*    if (node == 0 || node->type() == Node::Type_Project) {
        return;
    }
    int row = node->getParent()->findChildNode(node);
    Q_EMIT dataChanged(createIndex(row, 0, node), createIndex(row, columnCount() - 1, node));*/
}


} // namespace KPlato
