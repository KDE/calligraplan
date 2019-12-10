/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// clazy:excludeall=qstring-arg
#include "InsertTaskModuleCommand.h"

#include "kptaccount.h"
#include "kptappointment.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptcalendar.h"
#include "kptrelation.h"
#include "kptresource.h"
#include "kptdocuments.h"
#include "kptlocale.h"
#include "kptdebug.h"

#include <QApplication>


const QLoggingCategory &PLANCMDINSTASKMODULE_LOG()
{
    static const QLoggingCategory category("calligra.plan.command.inserttaskmodule");
    return category;
}

#define debugPlanInsertTaskModule qCDebug(PLANCMDINSTASKMODULE_LOG)
#define warnPlanInsertTaskModule qCWarning(PLANCMDINSTASKMODULE_LOG)
#define errorPlanInsertTaskModule qCCritical(PLANCMDINSTASKMODULE_LOG)

using namespace KPlato;

class AddTaskCommand : public NamedCommand
{
public:
    AddTaskCommand(Project *project, Node *parent, Node *node, Node *after, const KUndo2MagicString& name = KUndo2MagicString());
    ~AddTaskCommand() override;
    void execute() override;
    void unexecute() override;
    
private:
    Project *m_project;
    Node *m_parent;
    Node *m_node;
    Node *m_after;
    bool m_added;
};

AddTaskCommand::AddTaskCommand(Project *project, Node *parent, Node *node, Node *after, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_project(project)
    , m_parent(parent)
    , m_node(node)
    , m_after(after)
    , m_added(false)
{
}

AddTaskCommand::~AddTaskCommand()
{
    if (!m_added)
        delete m_node;
}

void AddTaskCommand::execute()
{
    m_project->addSubTask(m_node, m_parent->indexOf(m_after), m_parent, true);
    m_added = true;
}

void AddTaskCommand::unexecute()
{
    m_project->takeTask(m_node);
    m_added = false;
}

InsertTaskModuleCommand::InsertTaskModuleCommand(Project *project, const QByteArray &data, Node *parent, Node *position, const QMap<QString, QString> substitute, const KUndo2MagicString& name)
    : MacroCommand(name)
    , m_project(project)
    , m_data(data)
    , m_parent(parent)
    , m_position(position)
    , m_substitute(substitute)
    , m_first(true)
{
    //debugPlan<<cal->name();
    Q_ASSERT(project != 0);
    m_context.setProject(project);
    m_context.setProjectTimeZone(project->timeZone()); // from xml doc?
    m_context.setLoadTaskChildren(false);
}

InsertTaskModuleCommand::~InsertTaskModuleCommand()
{
}

void InsertTaskModuleCommand::execute()
{
    if (m_first) {
        // create and execute commands
        KoXmlDocument doc;
        doc.setContent(m_data);
        m_context.setVersion(doc.documentElement().attribute("plan-version", PLAN_FILE_SYNTAX_VERSION));
        KoXmlElement projectElement = doc.documentElement().namedItem("project").toElement();

        createCmdAccounts(projectElement);
        createCmdCalendars(projectElement);
        createCmdResources(projectElement);
        createCmdTasks(projectElement);
        createCmdRelations(projectElement);
        createCmdRequests(projectElement);
        m_first = false;
        m_data.clear();
    } else {
        MacroCommand::execute();
    }
}

void InsertTaskModuleCommand::unexecute()
{
    MacroCommand::unexecute();
}

void InsertTaskModuleCommand::createCmdAccounts(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
}

void InsertTaskModuleCommand::createCmdCalendars(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
}

void InsertTaskModuleCommand::createCmdResources(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
}

void InsertTaskModuleCommand::createCmdRequests(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
    debugPlanXml<<m_context.version();
    if (m_context.version() < "0.7.0") {
        return; // requests loaded by tasks
    }
    KoXmlElement parentElement = projectElement.namedItem("resourcegroup-requests").toElement();
    KoXmlElement ge;
    forEachElement(ge, parentElement) {
        if (ge.tagName() != "resourcegroup-request") {
            continue;
        }
        Task *task = qobject_cast<Task*>(m_oldIds.value(ge.attribute("task-id")));
        ResourceGroup *group = m_project->findResourceGroup(ge.attribute("group-id"));
        if (task && group) {
            int units = ge.attribute("units", "0").toInt();
            int requestId = ge.attribute("request-id").toInt();
            ResourceGroupRequest *request = new ResourceGroupRequest(group, units);
            request->setId(requestId);
            KUndo2Command *cmd = new AddResourceGroupRequestCmd(*task, request);
            cmd->redo();
            addCommand(cmd);
        } else {
            warnPlanXml<<"Failed to find group or task"<<task<<group;
        }
    }
    parentElement = projectElement.namedItem("resource-requests").toElement();
    KoXmlElement re;
    forEachElement(re, parentElement) {
        if (re.tagName() != "resource-request") {
            continue;
        }
        Task *task = qobject_cast<Task*>(m_oldIds.value(ge.attribute("task-id")));
        if (!task) {
            warnPlanXml<<re.tagName()<<"Failed to find task";
            continue;
        }
        ResourceGroupRequest *group = task->requests().groupRequest(re.attribute("request-id").toInt());
        if (!group) {
            warnPlanXml<<re.tagName()<<"Failed to find group request:"<<re.attribute("request-id");
        }
        Resource *resource = m_project->findResource(re.attribute("resource-id"));
        Q_ASSERT(resource);
        Q_ASSERT(task);
        if (resource && task) {
            int units = re.attribute("units", "100").toInt();
            ResourceRequest *request = new ResourceRequest(resource, units);
            int requestId = re.attribute("request-id").toInt();
            Q_ASSERT(requestId > 0);
            request->setId(requestId);
            KUndo2Command *cmd = new AddResourceRequestCmd(&task->requests(), request);
            cmd->redo();
            addCommand(cmd);
        } else {
            warnPlanXml<<re.tagName()<<"Failed to find resource";
        }
    }
    re = projectElement.namedItem("required-resource-requests").toElement();
    forEachElement(re, parentElement) {
        if (re.tagName() != "required-resource-request") {
            continue;
        }
        Task *task = qobject_cast<Task*>(m_oldIds.value(ge.attribute("task-id")));
        Q_ASSERT(task);
        if (!task) {
            warnPlanXml<<re.tagName()<<"Failed to find task";
            continue;
        }
        ResourceRequest *request = task->requests().resourceRequest(re.attribute("request-id").toInt());
        Resource *required = m_project->findResource(re.attribute("required-id"));
        QList<Resource*> lst;
        if (required && request->resource() != required) {
            lst << required;
        }
        KUndo2Command *cmd = new ModifyResourceRequestRequiredCmd(request, lst);
        cmd->redo();
        addCommand(cmd);
    }
}

void InsertTaskModuleCommand::createCmdTasks(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
    createCmdTask(projectElement, m_parent, m_position);
}

void InsertTaskModuleCommand::createCmdTask(const KoXmlElement &parentElement, Node *parent, Node *position)
{
    KoXmlElement taskElement;
    forEachElement(taskElement, parentElement) {
        if (taskElement.tagName() != "task") {
            continue;
        }
        Task *task = m_project->createTask();
        QString id = task->id();
        task->load(taskElement, m_context);
        m_oldIds.insert(task->id(), task);
        task->setId(id);
        if (!m_substitute.isEmpty()) {
            substitute(task->name());
        }
        NamedCommand *cmd = new AddTaskCommand(m_project, parent, task, position);
        cmd->execute();
        addCommand(cmd);

        createCmdTask(taskElement, task); // add children
    }
}

void InsertTaskModuleCommand::createCmdRelations(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
    KoXmlElement relationElement;
    forEachElement(relationElement, projectElement) {
        if (relationElement.tagName() != "relation") {
            continue;
        }
        Node *parent = m_oldIds.value(relationElement.attribute("parent-id"));
        Node *child = m_oldIds.value(relationElement.attribute("child-id"));
        if (parent && child) {
            Relation *relation = new Relation(parent, child);
            relation->setType(relationElement.attribute("type"));
            relation->setLag(Duration::fromString(relationElement.attribute("lag")));
            AddRelationCmd *cmd = new AddRelationCmd(*m_project, relation);
            cmd->execute();
            addCommand(cmd);
        }
    }
}

void InsertTaskModuleCommand::substitute(QString &text)
{
    QMap<QString, QString>::const_iterator it = m_substitute.constBegin();
    for (; it != m_substitute.constEnd(); ++it) {
        text.replace("[[" + it.key() + "]]", it.value());
    }
}
