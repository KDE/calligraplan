/* This file is part of the KDE project
  Copyright (C) 2009, 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>

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
  Boston, MA 02110-1301, USA.
*/

// clazy:excludeall=qstring-arg
#include "taskworkpackagemodel.h"

#include "part.h"
#include "workpackage.h"

#include "kptglobal.h"
#include "kptresource.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kpttaskcompletedelegate.h"

#include <KoIcon.h>

#include <KGanttGlobal>

#include <QTextEdit>
#include <QModelIndex>
#include <QMetaEnum>
#include <QObject>
#include <QAbstractItemDelegate>

#include "debugarea.h"

using namespace KPlato;

namespace KPlatoWork
{

TaskWorkPackageModel::TaskWorkPackageModel(Part *part, QObject *parent)
    : ItemModelBase(parent),
    m_part(part)
{
    m_packages = m_part->workPackages().values();
    connect(part, &Part::workPackageAdded, this, &TaskWorkPackageModel::addWorkPackage);
    connect(part, &Part::workPackageRemoved, this, &TaskWorkPackageModel::removeWorkPackage);
}

Qt::ItemFlags TaskWorkPackageModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    flags &= ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    Node *n = nodeForIndex(index);
    if (n == 0) {
        return flags;
    }
    if (n->type() != Node::Type_Task && n->type() != Node::Type_Milestone) {
        return flags;
    }
    Task *t = static_cast<Task*>(n);
    if (! t->completion().isStarted()) {
        switch (index.column()) {
            case NodeActualStart:
                //flags |= Qt::ItemIsEditable;
                break;
            case NodeCompleted:
                flags |= Qt::ItemIsEditable;
                break;
            default: break;
        }
    } else if (! t->completion().isFinished()) {
        // task is running
        switch (index.column()) {
            case NodeActualFinish:
            case NodeCompleted:
            case NodeRemainingEffort:
                flags |= Qt::ItemIsEditable;
            case NodeActualEffort:
                break;
            default: break;
        }
    }
    return flags;
}

void TaskWorkPackageModel::slotNodeToBeInserted(Node *parent, int row)
{
    //debugPlanWork<<parent->name()<<"; "<<row;
    beginInsertRows(indexForNode(parent), row, row);
}

void TaskWorkPackageModel::slotNodeInserted(Node */*node*/)
{
    //debugPlanWork<<node->parentNode()->name()<<"-->"<<node->name();
    endInsertRows();
}

void TaskWorkPackageModel::slotNodeToBeRemoved(Node *node)
{
    //debugPlanWork<<node->name();
    int row = indexForNode(node).row();
    beginRemoveRows(indexForNode(node->parentNode()), row, row);
}

void TaskWorkPackageModel::slotNodeRemoved(Node */*node*/)
{
    //debugPlanWork<<node->name();
    endRemoveRows();
}

void TaskWorkPackageModel::slotNodeChanged(Node *node)
{
    if (node == 0 || node->type() == Node::Type_Project) {
        return;
    }
    int row = indexForNode(node).row();
    debugPlanWork<<node->name()<<row;
    emit dataChanged(createIndex(row, 0, node->parentNode()), createIndex(row, columnCount()-1, node->parentNode()));
}

void TaskWorkPackageModel::slotDocumentAdded(Node *node, Document */*doc*/, int row)
{
    QModelIndex parent = indexForNode(node);
    if (parent.isValid()) {
        beginInsertRows(parent, row, row);
        endInsertRows();
    }
}

void TaskWorkPackageModel::slotDocumentRemoved(Node *node, Document */*doc*/, int row)
{
    QModelIndex parent = indexForNode(node);
    if (parent.isValid()) {
        beginRemoveRows(parent, row, row);
        endRemoveRows();
    }
}

void TaskWorkPackageModel::slotDocumentChanged(Node *node, Document */*doc*/, int row)
{
    QModelIndex parent = indexForNode(node);
    if (parent.isValid()) {
        emit dataChanged(index(row, 0, parent), index(row, columnCount(parent), parent));
    }
}

void TaskWorkPackageModel::addWorkPackage(WorkPackage *package, int /*row*/)
{
    int row = m_packages.count();
    beginInsertRows(QModelIndex(), row, row);
    m_packages.append(package);
    Project *project = package->project();
    endInsertRows();
    if (project) {
        connect(project, &KPlato::Project::nodeChanged, this, &TaskWorkPackageModel::slotNodeChanged);
        connect(project, &KPlato::Project::nodeToBeAdded, this, &TaskWorkPackageModel::slotNodeToBeInserted);
        connect(project, &KPlato::Project::nodeToBeRemoved, this, &TaskWorkPackageModel::slotNodeToBeRemoved);

        connect(project, &KPlato::Project::nodeAdded, this, &TaskWorkPackageModel::slotNodeInserted);
        connect(project, &KPlato::Project::nodeRemoved, this, &TaskWorkPackageModel::slotNodeRemoved);

        connect(project, &KPlato::Project::documentAdded, this, &TaskWorkPackageModel::slotDocumentAdded);
        connect(project, &KPlato::Project::documentRemoved, this, &TaskWorkPackageModel::slotDocumentRemoved);
        connect(project, &KPlato::Project::documentChanged, this, &TaskWorkPackageModel::slotDocumentChanged);
    }
}

void TaskWorkPackageModel::removeWorkPackage(WorkPackage *package, int /*row*/)
{
    int row = m_packages.indexOf(package);
    beginRemoveRows(QModelIndex(), row, row);
    m_packages.removeAt(row);
    Project *project = package->project();
    debugPlanWork<<package->project();
    if (project) {
        disconnect(project, &KPlato::Project::nodeChanged, this, &TaskWorkPackageModel::slotNodeChanged);
        disconnect(project, &KPlato::Project::nodeToBeAdded, this, &TaskWorkPackageModel::slotNodeToBeInserted);
        disconnect(project, &KPlato::Project::nodeToBeRemoved, this, &TaskWorkPackageModel::slotNodeToBeRemoved);

        disconnect(project, &KPlato::Project::nodeAdded, this, &TaskWorkPackageModel::slotNodeInserted);
        disconnect(project, &KPlato::Project::nodeRemoved, this, &TaskWorkPackageModel::slotNodeRemoved);

        disconnect(project, &KPlato::Project::documentAdded, this, &TaskWorkPackageModel::slotDocumentAdded);
        disconnect(project, &KPlato::Project::documentRemoved, this, &TaskWorkPackageModel::slotDocumentRemoved);
        disconnect(project, &KPlato::Project::documentChanged, this, &TaskWorkPackageModel::slotDocumentChanged);
    }
    endRemoveRows();
}

QVariant TaskWorkPackageModel::name(const Resource *r, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return r->name();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant TaskWorkPackageModel::email(const Resource *r, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return r->email();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant TaskWorkPackageModel::projectName(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole: {
            const Node *proj = node->projectNode();
            return proj == 0 ? QVariant() : proj->name();
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant TaskWorkPackageModel::projectManager(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole: {
            const Node *proj = node->projectNode();
            return proj == 0 ? QVariant() : proj->leader();
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

int TaskWorkPackageModel::rowCount(const QModelIndex &parent) const
{
    if (! parent.isValid()) {
        //debugPlanWork<<parent<<"nodes:"<<m_part->workPackageCount();
        return m_packages.count(); // == no of nodes (1 node pr wp)
    }
    Node *n = nodeForIndex(parent);
    if (n) {
        //debugPlanWork<<parent<<"docs:"<<n->documents().count();
        return n->documents().count();
    }
    //debugPlanWork<<parent<<"rows:"<<0;
    return 0; // documents have no children
}

int TaskWorkPackageModel::columnCount(const QModelIndex &) const
{
    return columnMap().keyCount();
}

QVariant TaskWorkPackageModel::data(const QModelIndex &index, int role) const
{
    if (! index.isValid()) {
        return QVariant();
    }
    Node *n = nodeForIndex(index);
    if (n) {
        if (role == Qt::ToolTipRole && index.column() == NodeName) {
            WorkPackage *wp = workPackage(index.row());
            QTextEdit w(n->description(), nullptr);
            QString description = w.toPlainText();
            if (description.length() > 200) {
                description = description.left(200) + " ...";
                description.replace('\n', "<br/>");
            } else {
                description = n->description();
            }
            w.setHtml(i18n("<p><strong>%1: %2</strong></p><p>%3</p>", wp->wbsCode(), n->name(), description));
            return w.toHtml();
        }
        if (role == Qt::DecorationRole && index.column() == NodeName) {
            WorkPackage *wp = workPackage(index.row());
            if (wp->task()->completion().isFinished()) {
                return koIcon("checkmark");
            }
        }
        return nodeData(n, index.column(), role);
    }
    Document *doc = documentForIndex(index);
    if (doc) {
        return documentData(doc, index.column(), role);
    }
    return QVariant();
}

QVariant TaskWorkPackageModel::actualStart(Node *n, int role) const
{
    QVariant v = m_nodemodel.startedTime(n, role);
    if (role == Qt::EditRole && ! v.toDateTime().isValid()) {
        v = QDateTime::currentDateTime();
    }
    return v;
}

QVariant TaskWorkPackageModel::actualFinish(Node *n, int role) const
{
    QVariant v = m_nodemodel.finishedTime(n, role);
    if (role == Qt::EditRole && ! v.toDateTime().isValid()) {
        v = QDateTime::currentDateTime();
    }
    return v;
}

QVariant TaskWorkPackageModel::plannedEffort(Node *n, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole: {
            Duration v = n->plannedEffort(CURRENTSCHEDULE, ECCT_EffortWork);
            return v.format();
        }
        default:
            break;
    }
    return QVariant();
}

QVariant TaskWorkPackageModel::status(Node *n, int role) const
{
    return m_nodemodel.status(n, role);
}

QVariant TaskWorkPackageModel::nodeData(Node *n, int column, int role) const
{
    if (role >= Qt::UserRole) {
//        debugPlanWork<<this<<n->name()<<column<<role;
        switch (role) {
        case KGantt::ItemTypeRole:
            switch (n->type()) {
            case Node::Type_Task: return KGantt::TypeTask;
            default: break;
            }
            break;
        case KGantt::StartTimeRole:
            debugPlanWork<<this<<n->name()<<"start:"<<n->startTime();
            return m_nodemodel.data(n, NodeModel::NodeStartTime, Qt::EditRole);
        case KGantt::EndTimeRole:
            debugPlanWork<<this<<n->name()<<"end:"<<n->endTime();
            return m_nodemodel.data(n, NodeModel::NodeEndTime, Qt::EditRole);
        default: break;
        }
    }
    switch (column) {
        case NodeName: return m_nodemodel.data(n, NodeModel::NodeName, role);
        case NodeType: return m_nodemodel.data(n, NodeModel::NodeType, role);
        case NodeResponsible: return m_nodemodel.data(n, NodeModel::NodeResponsible, role);
        case NodeDescription: return m_nodemodel.data(n, NodeModel::NodeDescription, role);

        // After scheduling
        case NodeStartTime: return m_nodemodel.data(n, NodeModel::NodeStartTime, role);
        case NodeEndTime: return m_nodemodel.data(n, NodeModel::NodeEndTime, role);
        case NodeAssignments: return m_nodemodel.data(n, NodeModel::NodeAssignments, role);

        // Completion
        case NodeCompleted: return m_nodemodel.data(n, NodeModel::NodeCompleted, role);
        case NodeActualEffort: return m_nodemodel.data(n, NodeModel::NodeActualEffort, role);
        case NodeRemainingEffort: return m_nodemodel.data(n, NodeModel::NodeRemainingEffort, role);
        case NodePlannedEffort: return plannedEffort(n, role);
        case NodeActualStart: return actualStart(n, role);
        case NodeStarted: return m_nodemodel.data(n, NodeModel::NodeStarted, role);
        case NodeActualFinish: return actualFinish(n, role);
        case NodeFinished: return m_nodemodel.data(n, NodeModel::NodeFinished, role);
        case NodeStatus: return status(n, role);
        case NodeStatusNote: return m_nodemodel.data(n, NodeModel::NodeStatusNote, role);

        case ProjectName: return projectName(n, role);
        case ProjectManager: return projectManager(n, role);

        default:
            //debugPlanWork<<"Invalid column number: "<<index.column()<<'\n';
            break;
    }
    return "";
}

QVariant TaskWorkPackageModel::documentData(Document *doc, int column, int role) const
{
    //debugPlanWork<<doc->url().fileName()<<column<<role;
    if (role == Qt::DisplayRole) {
        switch (column) {
            case NodeName: return doc->name();
            case NodeType: return doc->typeToString(doc->type(), true);
            case NodeStatusNote: return doc->status();
            default:
                return "";
        }
    } else if (role == Qt::ToolTipRole) {
        switch (column) {
            case NodeName: {
                QString s = xi18nc("@info:tooltip", "Type: %1<nl/>Url: %2",
                    doc->typeToString(doc->type(), true),
                    doc->url().toDisplayString());
                return s;
            }
            default:
                break;
        }
    }
    return QVariant();
}

bool TaskWorkPackageModel::setCompletion(Node *node, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }
    if (node->type() == Node::Type_Task) {
        Completion &c = static_cast<Task*>(node)->completion();
        QDate date = qMax(c.entryDate(), QDate::currentDate());
        QDateTime dt(date, QTime::currentTime());
        // xgettext: no-c-format
        MacroCommand *m = new MacroCommand(kundo2_i18n("Modify completion"));
        if (! c.isStarted()) {
            m->addCommand(new ModifyCompletionStartedCmd(c, true));
            m->addCommand(new ModifyCompletionStartTimeCmd(c, dt));
        }
        m->addCommand(new ModifyCompletionPercentFinishedCmd(c, date, value.toInt()));
        if (value.toInt() == 100) {
            m->addCommand(new ModifyCompletionFinishedCmd(c, true));
            m->addCommand(new ModifyCompletionFinishTimeCmd(c, dt));
        }
        bool newentry = c.entryDate() < date;
        emit executeCommand(m); // also adds a new entry if necessary
        if (newentry) {
            // new entry so calculate used/remaining based on completion
            Duration planned = static_cast<Task*>(node)->plannedEffort(m_nodemodel.id());
            Duration actual = (planned * value.toInt()) / 100;
            debugPlanWork<<planned.toString()<<value.toInt()<<actual.toString();
            NamedCommand *cmd = new ModifyCompletionActualEffortCmd(c, date, actual);
            cmd->execute();
            m->addCommand(cmd);
            cmd = new ModifyCompletionRemainingEffortCmd(c, date, planned - actual);
            cmd->execute();
            m->addCommand(cmd);
        } else if (c.isFinished() && c.remainingEffort() != 0) {
            ModifyCompletionRemainingEffortCmd *cmd = new ModifyCompletionRemainingEffortCmd(c, date, Duration::zeroDuration);
            cmd->execute();
            m->addCommand(cmd);
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
            emit executeCommand(m); // also adds a new entry if necessary
            return true;
        }
        return false;
    }
    return false;
}

bool TaskWorkPackageModel::setRemainingEffort(Node *node, const QVariant &value, int role)
{
    if (role == Qt::EditRole && node->type() == Node::Type_Task) {
        Task *t = static_cast<Task*>(node);
        double d(value.toList()[0].toDouble());
        Duration::Unit unit = static_cast<Duration::Unit>(value.toList()[1].toInt());
        Duration dur(d, unit);
        emit executeCommand(new ModifyCompletionRemainingEffortCmd(t->completion(), QDate::currentDate(), dur, kundo2_i18n("Modify remaining effort")));
        return true;
    }
    return false;
}

bool TaskWorkPackageModel::setActualEffort(Node *node, const QVariant &value, int role)
{
    if (role == Qt::EditRole && node->type() == Node::Type_Task) {
        Task *t = static_cast<Task*>(node);
        double d(value.toList()[0].toDouble());
        Duration::Unit unit = static_cast<Duration::Unit>(value.toList()[1].toInt());
        Duration dur(d, unit);
        emit executeCommand(new ModifyCompletionActualEffortCmd(t->completion(), QDate::currentDate(), dur, kundo2_i18n("Modify actual effort")));
        return true;
    }
    return false;
}

bool TaskWorkPackageModel::setStartedTime(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Task *t = qobject_cast<Task*>(node);
            if (t == 0) {
                return false;
            }
            MacroCommand *m = new MacroCommand(kundo2_noi18n(headerData(NodeModel::NodeActualStart, Qt::Horizontal, Qt::DisplayRole).toString())); //FIXME: proper description when string freeze is lifted
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
            emit executeCommand(m);
            return true;
        }
    }
    return false;
}

bool TaskWorkPackageModel::setFinishedTime(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Task *t = qobject_cast<Task*>(node);
            if (t == 0) {
                return false;
            }
            MacroCommand *m = new MacroCommand(kundo2_noi18n(headerData(NodeModel::NodeActualFinish, Qt::Horizontal, Qt::DisplayRole).toString())); //FIXME: proper description when string freeze is lifted
            if (! t->completion().isFinished()) {
                m->addCommand(new ModifyCompletionFinishedCmd(t->completion(), true));
                if (t->completion().percentFinished() < 100) {
                    QDate lastdate = t->completion().entryDate();
                    if (! lastdate.isValid() || lastdate < value.toDate()) {
                        Completion::Entry *e = new Completion::Entry(100, Duration::zeroDuration, Duration::zeroDuration);
                        m->addCommand(new AddCompletionEntryCmd(t->completion(), value.toDate(), e));
                    } else {
                        Completion::Entry *e = new Completion::Entry(*(t->completion().entry(lastdate)));
                        e->percentFinished = 100;
                        m->addCommand(new ModifyCompletionEntryCmd(t->completion(), lastdate, e));
                    }
                }
            }
            m->addCommand(new ModifyCompletionFinishTimeCmd(t->completion(), value.toDateTime()));
            if (t->type() == Node::Type_Milestone) {
                m->addCommand(new ModifyCompletionStartedCmd(t->completion(), true));
                m->addCommand(new ModifyCompletionStartTimeCmd(t->completion(), value.toDateTime()));
            }
            emit executeCommand(m);
            return true;
        }
    }
    return false;
}

bool TaskWorkPackageModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (! index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    switch (index.column()) {
        case NodeCompleted:
            return setCompletion(nodeForIndex(index), value, role);
        case NodeRemainingEffort:
            return setRemainingEffort(nodeForIndex(index), value, role);
        case NodeActualEffort:
            return setActualEffort(nodeForIndex(index), value, role);
        case NodeActualStart:
            return setStartedTime(nodeForIndex(index), value, role);
        case NodeActualFinish:
            return setFinishedTime(nodeForIndex(index), value, role);
        default:
            break;
    }
    return false;
}

QVariant TaskWorkPackageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        return section;
    }
    if (role == Qt::DisplayRole) {
        switch (section) {
        case NodeName: return xi18nc("@title:column", "Name");
        case NodeType: return xi18nc("@title:column", "Type");
        case NodeResponsible: return xi18nc("@title:column", "Responsible");
        case NodeDescription: return xi18nc("@title:column", "Description");

        // After scheduling
        case NodeStartTime: return xi18nc("@title:column", "Planned Start");
        case NodeEndTime: return xi18nc("@title:column", "Planned Finish");
        case NodeAssignments: return xi18nc("@title:column", "Resource Assignments");

        // Completion
        case NodeCompleted: return xi18nc("@title:column", "Completion");
        case NodeActualEffort: return xi18nc("@title:column", "Actual Effort");
        case NodeRemainingEffort: return xi18nc("@title:column", "Remaining Effort");
        case NodePlannedEffort: return xi18nc("@title:column", "Planned Effort");
        case NodeActualStart: return xi18nc("@title:column", "Actual Start");
        case NodeStarted: return xi18nc("@title:column", "Started");
        case NodeActualFinish: return xi18nc("@title:column", "Actual Finish");
        case NodeFinished: return xi18nc("@title:column", "Finished");
        case NodeStatus: return xi18nc("@title:column", "Status");
        case NodeStatusNote: return xi18nc("@title:column", "Note");

        case ProjectName: return xi18nc("@title:column", "Project Name");
        case ProjectManager: return xi18nc("@title:column", "Project Manager");

        default:
            //debugPlanWork<<"Invalid column number: "<<index.column()<<'\n';
            break;
    }
    }
    return QVariant();
}

QModelIndex TaskWorkPackageModel::parent(const QModelIndex &idx) const
{
    if (! idx.isValid()) {
        return QModelIndex();
    }
    if (isDocument(idx)) {
        // a document index has a node as parent
        return indexForNode(ptrToNode(idx));
    }
    // a node index has no parent
    return QModelIndex();
}

QModelIndex TaskWorkPackageModel::index(int row, int column, const QModelIndex &parent) const
{
    if (! parent.isValid()) {
        // create a node index
        return createIndex(row, column, workPackage(row));
    }
    if (isNode(parent)) {
        // create a document index
        return createIndex(row, column, nodeForIndex(parent));
    }
    // documents don't have children, so shouldn't get here
    return QModelIndex();
}

Node *TaskWorkPackageModel::nodeForIndex(const QModelIndex &index) const
{
    WorkPackage *wp = ptrToWorkPackage(index);
    if (wp) {
        //debugPlanWork<<index<<parent->node()->name();
        return wp->node();
    }
    return 0;
}

Document *TaskWorkPackageModel::documentForIndex(const QModelIndex &index) const
{
    if (index.isValid()) {
        Node *parent = ptrToNode(index);
        if (parent && index.row() < parent->documents().count()) {
            //debugPlanWork<<index<<parent->name();
            return parent->documents().value(index.row());
        }
    }
    return 0;
}

QModelIndex TaskWorkPackageModel::indexForNode(Node *node) const
{
    WorkPackage *p = m_part->workPackage(node);
    if (p == 0) {
        return QModelIndex();
    }
    return createIndex(m_packages.indexOf(p), 0, p);
}

WorkPackage *TaskWorkPackageModel::workPackage(int index) const
{
    return m_packages.value(index);
}

QAbstractItemDelegate *TaskWorkPackageModel::createDelegate(int column, QWidget *parent) const
{
    switch (column) {
        case NodeCompleted: return new TaskCompleteDelegate(parent);
        case NodeRemainingEffort: return new DurationSpinBoxDelegate(parent);
        case NodeActualEffort: return new DurationSpinBoxDelegate(parent);
        case NodeActualStart: return new DateTimeCalendarDelegate(parent);
        case NodeActualFinish: return new DateTimeCalendarDelegate(parent);

        default: break;
    }
    return 0;
}

QModelIndex mapToModel(const TaskWorkPackageModel *m, const QModelIndex &indx)
{
    if (indx.model() == m) {
        return indx;
    }
    QModelIndex idx = indx;
    const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>(idx.model());
    while (proxy) {
        idx = proxy->mapToSource(idx);
        proxy = qobject_cast<const QAbstractProxyModel*>(idx.model());
    }
    return idx;
}

WorkPackage *TaskWorkPackageModel::ptrToWorkPackage(const QModelIndex &indx) const
{
    QModelIndex idx = mapToModel(this, indx);
    Q_ASSERT(idx.model() == this);
    return qobject_cast<WorkPackage*>(static_cast<QObject*>(idx.internalPointer()));
}

Node *TaskWorkPackageModel::ptrToNode(const QModelIndex &indx) const
{
    QModelIndex idx = mapToModel(this, indx);
    Q_ASSERT(idx.model() == this);
    return qobject_cast<Node*>(static_cast<QObject*>(idx.internalPointer()));
}

bool TaskWorkPackageModel::isNode(const QModelIndex &indx) const
{
    QModelIndex idx = mapToModel(this, indx);
    Q_ASSERT(idx.model() == this);
    // a node index: ptr is WorkPackage*
    return qobject_cast<WorkPackage*>(static_cast<QObject*>(idx.internalPointer())) != 0;
}

bool TaskWorkPackageModel::isDocument(const QModelIndex &indx) const
{
    QModelIndex idx = mapToModel(this, indx);
    Q_ASSERT(idx.model() == this);
    // a document index: ptr is Node*
    return qobject_cast<Node*>(static_cast<QObject*>(idx.internalPointer())) != 0;
}


} //namespace KPlato
