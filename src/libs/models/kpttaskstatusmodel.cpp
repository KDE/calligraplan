/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpttaskstatusmodel.h"

#include "PlanMacros.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptitemmodelbase.h"
#include "kpttaskcompletedelegate.h"
#include "kptcommand.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptnodeitemmodel.h"
#include "kptdebug.h"

#include <KLocalizedString>

#include <QMimeData>
#include <QModelIndex>


namespace KPlato
{


TaskStatusItemModel::TaskStatusItemModel(QObject *parent)
    : ItemModelBase(parent),
    m_period(7),
    m_periodType(UseCurrentDate),
    m_weekday(Qt::Friday)

{
    m_topNames << i18n("Not Started");
    m_topTips << i18n("Tasks that should have been started");
    m_top.append(&m_notstarted);
    
    m_topNames << i18n("Running");
    m_topTips << i18n("Tasks that are running");
    m_top.append(&m_running);
    
    m_topNames << i18n("Finished");
    m_topTips << i18n("Tasks that have finished during this period");
    m_top.append(&m_finished);
    
    m_topNames << i18n("Next Period");
    m_topTips << i18n("Tasks that are scheduled to start next period");
    m_top.append(&m_upcoming);
    
/*    connect(this, SIGNAL(modelAboutToBeReset()), SLOT(slotAboutToBeReset()));
    connect(this, SIGNAL(modelReset()), SLOT(slotReset()));*/
}

TaskStatusItemModel::~TaskStatusItemModel()
{
}
    
void TaskStatusItemModel::slotAboutToBeReset()
{
    debugPlan;
    clear();
}

void TaskStatusItemModel::slotReset()
{
    debugPlan;
    refresh();
}

void TaskStatusItemModel::slotNodeToBeInserted(Node *, int)
{
    //debugPlan<<node->name();
    clear();
}

void TaskStatusItemModel::slotNodeInserted(Node * /*node*/)
{
    //debugPlan<<node->getParent->name()<<"-->"<<node->name();
    refresh();
}

void TaskStatusItemModel::slotNodeToBeRemoved(Node * /*node*/)
{
    //debugPlan<<node->name();
    clear();
}

void TaskStatusItemModel::slotNodeRemoved(Node * /*node*/)
{
    //debugPlan<<node->name();
    refresh();
}

void TaskStatusItemModel::slotNodeToBeMoved(Node *node, int pos, Node *newParent, int newPos)
{
    Q_UNUSED(node);
    Q_UNUSED(pos);
    Q_UNUSED(newParent);
    Q_UNUSED(newPos);
    clear();
}

void TaskStatusItemModel::slotNodeMoved(Node * /*node*/)
{
    //debugPlan<<node->name();
    refresh();
}

void TaskStatusItemModel::setProject(Project *project)
{
    beginResetModel();
    clear();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &TaskStatusItemModel::projectDeleted);
        disconnect(m_project, &Project::localeChanged, this, &TaskStatusItemModel::slotLayoutChanged);
        disconnect(m_project, &Project::wbsDefinitionChanged, this, &TaskStatusItemModel::slotWbsDefinitionChanged);
        disconnect(m_project, &Project::nodeChanged, this, &TaskStatusItemModel::slotNodeChanged);
        disconnect(m_project, &Project::nodeToBeAdded, this, &TaskStatusItemModel::slotNodeToBeInserted);
        disconnect(m_project, &Project::nodeToBeRemoved, this, &TaskStatusItemModel::slotNodeToBeRemoved);
        disconnect(m_project, &Project::nodeToBeMoved, this, &TaskStatusItemModel::slotNodeToBeMoved);
    
        disconnect(m_project, &Project::nodeAdded, this, &TaskStatusItemModel::slotNodeInserted);
        disconnect(m_project, &Project::nodeRemoved, this, &TaskStatusItemModel::slotNodeRemoved);
        disconnect(m_project, &Project::nodeMoved, this, &TaskStatusItemModel::slotNodeMoved);
    }
    m_project = project;
    m_nodemodel.setProject(project);
    if (project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &TaskStatusItemModel::projectDeleted);
        connect(m_project, &Project::localeChanged, this, &TaskStatusItemModel::slotLayoutChanged);
        connect(m_project, &Project::wbsDefinitionChanged, this, &TaskStatusItemModel::slotWbsDefinitionChanged);
        connect(m_project, &Project::nodeChanged, this, &TaskStatusItemModel::slotNodeChanged);
        connect(m_project, &Project::nodeToBeAdded, this, &TaskStatusItemModel::slotNodeToBeInserted);
        connect(m_project, &Project::nodeToBeRemoved, this, &TaskStatusItemModel::slotNodeToBeRemoved);
        connect(m_project, &Project::nodeToBeMoved, this, &TaskStatusItemModel::slotNodeToBeMoved);

        connect(m_project, &Project::nodeAdded, this, &TaskStatusItemModel::slotNodeInserted);
        connect(m_project, &Project::nodeRemoved, this, &TaskStatusItemModel::slotNodeRemoved);
        connect(m_project, &Project::nodeMoved, this, &TaskStatusItemModel::slotNodeMoved);

    }
    endResetModel();
}

void TaskStatusItemModel::setScheduleManager(ScheduleManager *sm)
{
    beginResetModel();
    if (sm == m_nodemodel.manager()) {
        return;
    }
    clear();
    if (m_nodemodel.manager()) {
    }
    m_nodemodel.setManager(sm);
    ItemModelBase::setScheduleManager(sm);
    if (sm) {
    }
    endResetModel();
    refresh();
}

void TaskStatusItemModel::clear()
{
    for (NodeMap *l : std::as_const(m_top)) {
        int c = l->count();
        if (c > 0) {
            //FIXME: gives error msg:
            // Can't select indexes from different model or with different parents
            QModelIndex i = index(l);
            debugPlan<<i<<0<<c-1;
            beginRemoveRows(i, 0, c-1);
            l->clear();
            endRemoveRows();
        }
    }
}

void TaskStatusItemModel::setNow()
{
    switch (m_periodType) {
        case UseWeekday: {
            QDate date = QDate::currentDate();
            int wd = date.dayOfWeek();
            date = date.addDays(m_weekday - wd);
            if (wd < m_weekday) {
                date = date.addDays(-7);
            }
            m_nodemodel.setNow(date);
            break; }
        case UseCurrentDate: m_nodemodel.setNow(QDate::currentDate()); break;
        default: 
            m_nodemodel.setNow(QDate::currentDate());
            break;
    }
}

void TaskStatusItemModel::refresh()
{
    clear();
    if (m_project == nullptr) {
        return;
    }
    m_id = m_nodemodel.id();
    if (m_id == -1) {
        return;
    }
    setNow();
    const QDate begin = m_nodemodel.now().addDays(-m_period);
    const QDate end = m_nodemodel.now().addDays(m_period);

    const QList<Node*> nodes = m_project->allNodes();
    for (Node* n : nodes) {
        if (n->type() != Node::Type_Task && n->type() != Node::Type_Milestone) {
            continue;
        }
        Task *task = static_cast<Task*>(n);
        const TaskStatus status = taskStatus(task, begin, end);
        if (status != TaskUnknownStatus) {
            m_top.at(status)->insert(task->wbsCode(), task);
        }
    }
    for (NodeMap *l : std::as_const(m_top)) {
        int c = l->count();
        if (c > 0) {
            debugPlan<<index(l)<<0<<c-1;
            beginInsertRows(index(l), 0, c-1);
            endInsertRows();
        }
    }
    Q_EMIT layoutChanged(); //HACK to get views updated
}

Qt::ItemFlags TaskStatusItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    flags &= ~(Qt::ItemIsEditable | Qt::ItemIsDropEnabled);
    flags |= Qt::ItemIsDragEnabled;
    Node *n = node(index);
    if (! m_readWrite || n == nullptr || m_id == -1 || ! n->isScheduled(m_id)) {
        return flags;
    }
    if (n->type() != Node::Type_Task && n->type() != Node::Type_Milestone) {
        return flags;
    }
    Task *t = static_cast<Task*>(n);
    if (! t->completion().isStarted()) {
        switch (index.column()) {
            case NodeModel::NodeActualStart:
                flags |= Qt::ItemIsEditable;
                break;
            case NodeModel::NodeCompleted:
                if (t->state() & Node::State_ReadyToStart) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            default: break;
        }
    } else if (! t->completion().isFinished()) {
        // task is running
        switch (index.column()) {
            case NodeModel::NodeActualFinish:
            case NodeModel::NodeCompleted:
            case NodeModel::NodeRemainingEffort:
                flags |= Qt::ItemIsEditable;
                break;
            case NodeModel::NodeActualEffort:
                if (t->completion().entrymode() == Completion::EnterEffortPerTask || t->completion().entrymode() == Completion::EnterEffortPerResource) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            default: break;
        }
    }
    return flags;
}

    
QModelIndex TaskStatusItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    //debugPlan<<index.internalPointer()<<":"<<index.row()<<","<<index.column();
    int row = m_top.indexOf(static_cast<NodeMap*>(index.internalPointer()));
    if (row != -1) {
        return QModelIndex(); // top level has no parent
    }
    Node *n = node(index);
    if (n == nullptr) {
        return QModelIndex();
    }
    NodeMap *lst = nullptr;
    for (NodeMap *l : std::as_const(m_top)) {
        if (CONTAINS((*l), n)) {
            lst = l;
            break;
        }
    }
    if (lst == nullptr) {
        return QModelIndex();
    }
    return createIndex(m_top.indexOf(lst), 0, lst);
}

QModelIndex TaskStatusItemModel::index(int row, int column, const QModelIndex &parent) const
{
    //debugPlan<<row<<column<<parent;
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    if (! parent.isValid()) {
        if (row >= m_top.count()) {
            return QModelIndex();
        }
        return createIndex(row, column, m_top.value(row));
    }
    NodeMap *l = list(parent);
    if (l == nullptr) {
        return QModelIndex();
    }
    if (row >= rowCount(parent)) {
        warnPlan<<"Row >= rowCount, Qt4.4 asks, so we need to handle it"<<parent<<row<<column;
        return QModelIndex();
    }
    const QList<Node*> &nodes = l->values();
    QModelIndex i = createIndex(row, column, nodes.value(row));
    Q_ASSERT(i.internalPointer() != nullptr);
    return i;
}

QModelIndex TaskStatusItemModel::index(const Node *node) const
{
    if (m_project == nullptr || node == nullptr) {
        return QModelIndex();
    }
    for(NodeMap *l : std::as_const(m_top)) {
        const QList<Node*> &nodes = l->values();
        int row = nodes.indexOf(const_cast<Node*>(node));
        if (row != -1) {
            return createIndex(row, 0, const_cast<Node*>(node));
        }
    }
    return QModelIndex();
}

QModelIndex TaskStatusItemModel::index(const NodeMap *lst) const
{
    if (m_project == nullptr || lst == nullptr) {
        return QModelIndex();
    }
    NodeMap *l = const_cast<NodeMap*>(lst);
    int row = m_top.indexOf(l);
    if (row == -1) {
        return QModelIndex();
    }
    return createIndex(row, 0, l);
}

QVariant TaskStatusItemModel::name(int row, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return m_topNames.value(row);
        case Qt::ToolTipRole:
            return m_topTips.value(row);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool TaskStatusItemModel::setCompletion(Node *node, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }
    if (node->type() == Node::Type_Task) {
        Completion &c = static_cast<Task*>(node)->completion();
        QDateTime dt = QDateTime::currentDateTime();
        QDate date = dt.date();
        bool stateChanged = false;
        // xgettext: no-c-format
        MacroCommand *m = new MacroCommand(kundo2_i18n("Modify completion"));
        if (! c.isStarted()) {
            m->addCommand(new ModifyCompletionStartedCmd(c, true));
            m->addCommand(new ModifyCompletionStartTimeCmd(c, dt));
            stateChanged = true;
        }
        m->addCommand(new ModifyCompletionPercentFinishedCmd(c, date, value.toInt()));
        if (value.toInt() == 100) {
            m->addCommand(new ModifyCompletionFinishedCmd(c, true));
            m->addCommand(new ModifyCompletionFinishTimeCmd(c, dt));
            stateChanged = true;
        }
        Q_EMIT executeCommand(m); // also adds a new entry if necessary
        if (c.entrymode() != Completion::EnterEffortPerResource) {
            Duration planned = static_cast<Task*>(node)->plannedEffort(m_nodemodel.id());
            Duration actual = (planned * value.toInt()) / 100;
            debugPlan<<planned.toString()<<value.toInt()<<actual.toString();
            NamedCommand *cmd = new ModifyCompletionActualEffortCmd(c, date, actual);
            cmd->execute();
            m->addCommand(cmd);
            cmd = new ModifyCompletionRemainingEffortCmd(c, date, planned - actual);
            cmd->execute();
            m->addCommand(cmd);
        }
        if (stateChanged) {
            Q_EMIT this->stateChanged(node);
        }
        return true;
    }
    if (node->type() == Node::Type_Milestone) {
        Completion &c = static_cast<Task*>(node)->completion();
        if (value.toInt() > 0) {
            QDateTime dt = QDateTime::currentDateTime();
            QDate date = dt.date();
            MacroCommand *m = new MacroCommand(kundo2_i18n("Set finished"));
            m->addCommand(new ModifyCompletionStartedCmd(c, true));
            m->addCommand(new ModifyCompletionStartTimeCmd(c, dt));
            m->addCommand(new ModifyCompletionFinishedCmd(c, true));
            m->addCommand(new ModifyCompletionFinishTimeCmd(c, dt));
            m->addCommand(new ModifyCompletionPercentFinishedCmd(c, date, 100));
            Q_EMIT executeCommand(m); // also adds a new entry if necessary
            Q_EMIT stateChanged(node);
            return true;
        }
        return false;
    }
    return false;
}

bool TaskStatusItemModel::setRemainingEffort(Node *node, const QVariant &value, int role)
{
    if (role == Qt::EditRole && node->type() == Node::Type_Task) {
        Task *t = static_cast<Task*>(node);
        double d(value.toList()[0].toDouble());
        Duration::Unit unit = static_cast<Duration::Unit>(value.toList()[1].toInt());
        Duration dur(d, unit);
        Q_EMIT executeCommand(new ModifyCompletionRemainingEffortCmd(t->completion(), QDate::currentDate(), dur, kundo2_i18n("Modify remaining effort")));
        return true;
    }
    return false;
}

bool TaskStatusItemModel::setActualEffort(Node *node, const QVariant &value, int role)
{
    if (role == Qt::EditRole && node->type() == Node::Type_Task) {
        Task *t = static_cast<Task*>(node);
        double d(value.toList()[0].toDouble());
        Duration::Unit unit = static_cast<Duration::Unit>(value.toList()[1].toInt());
        Duration dur(d, unit);
        Q_EMIT executeCommand(new ModifyCompletionActualEffortCmd(t->completion(), QDate::currentDate(), dur, kundo2_i18n("Modify actual effort")));
        return true;
    }
    return false;
}

bool TaskStatusItemModel::setStartedTime(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Task *t = qobject_cast<Task*>(node);
            if (t == nullptr) {
                return false;
            }
            MacroCommand *m = new MacroCommand(kundo2_i18n("Modify actual start time"));
            if (! t->completion().isStarted()) {
                m->addCommand(new ModifyCompletionStartedCmd(t->completion(), true));
            }
            m->addCommand(new ModifyCompletionStartTimeCmd(t->completion(), value.toDateTime()));
            if (t->type() == Node::Type_Milestone) {
                m->addCommand(new ModifyCompletionFinishedCmd(t->completion(), true));
                m->addCommand(new ModifyCompletionFinishTimeCmd(t->completion(), value.toDateTime()));
                if (t->completion().percentFinished() < 100) {
                    Completion::Entry *e = new Completion::Entry(100, Duration::zeroDuration, Duration::zeroDuration);
                    m->addCommand(new AddCompletionEntryCmd(t->completion(), value.toDate(), e));
                }
            }
            Q_EMIT executeCommand(m);
            return true;
        }
    }
    return false;
}

bool TaskStatusItemModel::setFinishedTime(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Task *t = qobject_cast<Task*>(node);
            if (t == nullptr) {
                return false;
            }
            MacroCommand *m = new MacroCommand(kundo2_i18n("Modify actual finish time"));
            if (! t->completion().isFinished()) {
                m->addCommand(new ModifyCompletionFinishedCmd(t->completion(), true));
                if (t->completion().percentFinished() < 100) {
                    Completion::Entry *e = new Completion::Entry(100, Duration::zeroDuration, Duration::zeroDuration);
                    m->addCommand(new AddCompletionEntryCmd(t->completion(), value.toDate(), e));
                }
            }
            m->addCommand(new ModifyCompletionFinishTimeCmd(t->completion(), value.toDateTime()));
            if (t->type() == Node::Type_Milestone) {
                m->addCommand(new ModifyCompletionStartedCmd(t->completion(), true));
                m->addCommand(new ModifyCompletionStartTimeCmd(t->completion(), value.toDateTime()));
            }
            Q_EMIT executeCommand(m);
            return true;
        }
    }
    return false;
}

QVariant TaskStatusItemModel::data(const QModelIndex &index, int role) const
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
            case NodeModel::NodeName: return name(index.row(), role);
            default: break;
        }
        return QVariant();
    }
    result = m_nodemodel.data(n, index.column(), role);
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case NodeModel::NodeActualStart:
                if (! result.isValid()) {
                    return m_nodemodel.data(n, NodeModel::NodeStatus, role);
                }
            break;
        }
    } else if (role == Qt::EditRole) {
        switch (index.column()) {
            case NodeModel::NodeActualStart:
            case NodeModel::NodeActualFinish:
                if (! result.isValid()) {
                    return QDateTime::currentDateTime();
                }
            break;
        }
    }
    return result;
}


bool TaskStatusItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (! index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    switch (index.column()) {
        case NodeModel::NodeCompleted:
            return setCompletion(node(index), value, role);
        case NodeModel::NodeRemainingEffort:
            return setRemainingEffort(node(index), value, role);
        case NodeModel::NodeActualEffort:
            return setActualEffort(node(index), value, role);
        case NodeModel::NodeActualStart:
            return setStartedTime(node(index), value, role);
        case NodeModel::NodeActualFinish:
            return setFinishedTime(node(index), value, role);
        default:
            break;
    }
    return false;
}

QVariant TaskStatusItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            return m_nodemodel.headerData(section, role);
        } else if (role == Qt::TextAlignmentRole) {
            return alignment(section);
        }
    }
    if (role == Qt::ToolTipRole) {
        return m_nodemodel.headerData(section, role);
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QVariant TaskStatusItemModel::alignment(int column) const
{
    return m_nodemodel.headerData(column, Qt::TextAlignmentRole);
}

QAbstractItemDelegate *TaskStatusItemModel::createDelegate(int column, QWidget *parent) const
{
    switch (column) {
        case NodeModel::NodeCompleted: return new TaskCompleteDelegate(parent);
        case NodeModel::NodeRemainingEffort: return new DurationSpinBoxDelegate(parent);
        case NodeModel::NodeActualEffort: return new DurationSpinBoxDelegate(parent);
        default: return nullptr;
    }
    return nullptr;
}

int TaskStatusItemModel::columnCount(const QModelIndex &) const
{
    return m_nodemodel.propertyCount();
}

int TaskStatusItemModel::rowCount(const QModelIndex &parent) const
{
    if (! parent.isValid()) {
        //debugPlan<<"top="<<m_top.count()<<m_top;
        return m_top.count();
    }
    NodeMap *l = list(parent);
    if (l) {
        //debugPlan<<"list"<<parent.row()<<":"<<l->count()<<l<<m_topNames.value(parent.row());
        return l->count();
    }
    //debugPlan<<"node"<<parent.row();
    return 0; // nodes don't have children
}

Qt::DropActions TaskStatusItemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList TaskStatusItemModel::mimeTypes() const
{
    return QStringList()
            << QStringLiteral("text/html")
            << QStringLiteral("text/plain");
}

QMimeData *TaskStatusItemModel::mimeData(const QModelIndexList & indexes) const
{
    return ItemModelBase::mimeData(indexes);
}

bool TaskStatusItemModel::dropAllowed(Node *, const QMimeData *)
{
    return false;
}

bool TaskStatusItemModel::dropMimeData(const QMimeData *, Qt::DropAction , int , int , const QModelIndex &)
{
    return false;
}

NodeMap *TaskStatusItemModel::list(const QModelIndex &index) const
{
    if (index.isValid()) {
        Q_ASSERT(index.internalPointer());
        if (m_top.contains(static_cast<NodeMap*>(index.internalPointer()))) {
            return static_cast<NodeMap*>(index.internalPointer());
        }
    }
    return nullptr;
}

Node *TaskStatusItemModel::node(const QModelIndex &index) const
{
    if (index.isValid()) {
        for (NodeMap *l : std::as_const(m_top)) {
            const QList<Node*> &nodes = l->values();
            int row = nodes.indexOf(static_cast<Node*>(index.internalPointer()));
            if (row != -1) {
                return static_cast<Node*>(index.internalPointer());
            }
        }
    }
    return nullptr;
}

TaskStatusItemModel::TaskStatus TaskStatusItemModel::taskStatus(const Task *task,
                                                                const QDate &begin, const QDate &end)
{
    TaskStatus result = TaskUnknownStatus;

    const Completion &completion = task->completion();
    if (completion.isFinished()) {
        if (completion.finishTime().date() > begin) {
            result = TaskFinished;
        }
    } else if (completion.isStarted()) {
        result = TaskRunning;
    } else if (task->startTime(m_id).date() < m_nodemodel.now()) {
        // should have been started
        result = TaskNotStarted;
    } else if (task->startTime(m_id).date() <= end) {
        // start next period
        result = TaskUpcoming;
    }
    return result;
}

void TaskStatusItemModel::slotNodeChanged(Node *node)
{
    debugPlan;
    if (node == nullptr || node->type() == Node::Type_Project ||
        (node->type() != Node::Type_Task && node->type() != Node::Type_Milestone)) {
        return;
    }

    Task *task = static_cast<Task*>(node);

    const QDate begin = m_nodemodel.now().addDays(-m_period);
    const QDate end = m_nodemodel.now().addDays(m_period);

    const TaskStatus status = taskStatus(task, begin, end);

    int row = -1;
    if (status != TaskUnknownStatus) {
        // find the row of the task
        const QString wbs = node->wbsCode();
        // TODO: not enough to just check the result of indexOf? wbs not unique?
        if (m_top.at(status)->value(wbs) == node) {
            row = m_top.at(status)->keys().indexOf(wbs);
        }
    }

    if (row >= 0) {
        // task in old group, just changed values
        Q_EMIT dataChanged(createIndex(row, 0, node), createIndex(row, columnCount() - 1, node));
    } else {
        // task is new or changed groups
        refresh();
    }
}

void TaskStatusItemModel::slotWbsDefinitionChanged()
{
    debugPlan;
    for (NodeMap *l : std::as_const(m_top)) {
        for (int row = 0; row < l->count(); ++row) {
            const QList<Node*> &nodes = l->values();
            Q_EMIT dataChanged(createIndex(row, NodeModel::NodeWBSCode, nodes.value(row)), createIndex(row, NodeModel::NodeWBSCode, nodes.value(row)));
        }
    }
}

int TaskStatusItemModel::sortRole(int column) const
{
    switch (column) {
        case NodeModel::NodeStartTime:
        case NodeModel::NodeEndTime:
        case NodeModel::NodeActualStart:
        case NodeModel::NodeActualFinish:
        case NodeModel::NodeEarlyStart:
        case NodeModel::NodeEarlyFinish:
        case NodeModel::NodeLateStart:
        case NodeModel::NodeLateFinish:
        case NodeModel::NodeConstraintStart:
        case NodeModel::NodeConstraintEnd:
            return Qt::EditRole;
        default:
            break;
    }
    return Qt::DisplayRole;
}

} // namespace KPlato
