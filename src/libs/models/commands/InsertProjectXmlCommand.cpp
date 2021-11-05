/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "InsertProjectXmlCommand.h"

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


const QLoggingCategory &PLANCMDINSPROJECT_LOG()
{
    static const QLoggingCategory category("calligra.plan.command.insertProjectXml");
    return category;
}

#define debugPlanInsertProjectXml qCDebug(PLANCMDINSPROJECT_LOG)<<Q_FUNC_INFO
#define warnPlanInsertProjectXml qCWarning(PLANCMDINSPROJECT_LOG)<<Q_FUNC_INFO
#define errorPlanInsertProjectXml qCCritical(PLANCMDINSPROJECT_LOG)<<Q_FUNC_INFO

using namespace KPlato;


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

InsertProjectXmlCommand::InsertProjectXmlCommand(Project *project, const QByteArray &data, Node *parent, Node *position, const KUndo2MagicString& name)
        : MacroCommand(name)
        , m_project(project)
        , m_data(data)
        , m_parent(parent)
        , m_position(position)
        , m_first(true)
{
    //debugPlanInsertProjectXml<<cal->name();
    Q_ASSERT(project != nullptr);
    m_context.setProject(project);
    m_context.setProjectTimeZone(project->timeZone()); // from xml doc?
    m_context.setLoadTaskChildren(false);
}

InsertProjectXmlCommand::~InsertProjectXmlCommand()
{
}

void InsertProjectXmlCommand::execute()
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

void InsertProjectXmlCommand::unexecute()
{
    MacroCommand::unexecute();
}

void InsertProjectXmlCommand::createCmdAccounts(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
}

void InsertProjectXmlCommand::createCmdCalendars(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
}

void InsertProjectXmlCommand::createCmdResources(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
}

void InsertProjectXmlCommand::createCmdRequests(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
    debugPlanInsertProjectXml<<m_context.version();
    if (m_context.version() < "0.7.0") {
        return; // Requests loaded by tasks
    }
    KoXmlElement parentElement = projectElement.namedItem("resource-requests").toElement();
    KoXmlElement re;
    forEachElement(re, parentElement) {
        if (re.tagName() != "resource-request") {
            continue;
        }
        Task *task = qobject_cast<Task*>(m_oldIds.value(re.attribute("task-id")));
        if (!task) {
            warnPlanInsertProjectXml<<re.tagName()<<"Failed to find task";
            continue;
        }
        Resource *resource = m_project->findResource(re.attribute("resource-id"));
        if (!resource) {
            warnPlanInsertProjectXml<<re.tagName()<<"Failed to add allocation, resource does not exist";
            continue;
        }
        Q_ASSERT(task);
        if (task) {
            int units = re.attribute("units", "100").toInt();
            ResourceRequest *request = new ResourceRequest(resource, units);
            int requestId = re.attribute("request-id").toInt();
            Q_ASSERT(requestId > 0);
            request->setId(requestId);
            KUndo2Command *cmd = new AddResourceRequestCmd(&task->requests(), request);
            cmd->redo();
            addCommand(cmd);
            debugPlanInsertProjectXml<<"added resourcerequest:"<<task<<request;
        } else {
            warnPlanInsertProjectXml<<re.tagName()<<"Failed to find task";
        }
    }
    parentElement = projectElement.namedItem("required-resource-requests").toElement();
    forEachElement(re, parentElement) {
        if (re.tagName() != "required-resource-request") {
            continue;
        }
        Task *task = qobject_cast<Task*>(m_oldIds.value(re.attribute("task-id")));
        Q_ASSERT(task);
        if (!task) {
            warnPlanInsertProjectXml<<re.tagName()<<"Failed to find task";
            continue;
        }
        ResourceRequest *request = task->requests().resourceRequest(re.attribute("request-id").toInt());
        Resource *required = m_project->findResource(re.attribute("required-id"));
        if (!required) {
            warnPlanInsertProjectXml<<re.tagName()<<"Failed to add required resource, resource does not exist";
            continue;
        }
        QList<Resource*> lst;
        if (request->resource() != required) {
            lst << required;
        }
        KUndo2Command *cmd = new ModifyResourceRequestRequiredCmd(request, lst);
        cmd->redo();
        addCommand(cmd);
        debugPlanInsertProjectXml<<"added requiredrequest:"<<task<<request<<lst;
    }
    parentElement = projectElement.namedItem("alternative-requests").toElement();
    forEachElement(re, parentElement) {
        if (re.tagName() != "alternative-request") {
            continue;
        }
        Task *task = qobject_cast<Task*>(m_oldIds.value(re.attribute("task-id")));
        if (!task) {
            warnPlanInsertProjectXml<<re.tagName()<<"Failed to find task";
            continue;
        }
        Resource *resource = m_project->findResource(re.attribute("resource-id"));
        if (!resource) {
            warnPlanInsertProjectXml<<re.tagName()<<"Failed to find resource";
            continue;
        }
        ResourceRequest *request = task->requests().resourceRequest(re.attribute("request-id").toInt());
        if (!request) {
            warnPlanInsertProjectXml<<re.tagName()<<"Failed to find request";
            continue;
        }
        ResourceRequest *alternative = new ResourceRequest(resource, re.attribute("units", "100").toInt());
        request->addAlternativeRequest(alternative);
        debugPlanInsertProjectXml<<"added alternative-request:"<<task<<request<<alternative;
    }
}

void InsertProjectXmlCommand::createCmdTasks(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
    createCmdTask(projectElement, m_parent, m_position);
}

void InsertProjectXmlCommand::createCmdTask(const KoXmlElement &parentElement, Node *parent, Node *position)
{
    KoXmlElement taskElement;
    forEachElement(taskElement, parentElement) {
        if (taskElement.tagName() != "task") {
            continue;
        }
        Task *task = m_project->createTask();
        QString id = task->id();
        m_context.loader()->load(task, taskElement, m_context);
        m_oldIds.insert(task->id(), task);
        task->setId(id);
        NamedCommand *cmd = new AddTaskCommand(m_project, parent, task, position);
        cmd->execute();
        addCommand(cmd);

        createCmdTask(taskElement, task); // add children
    }
}

void InsertProjectXmlCommand::createCmdRelations(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
    auto relations = projectElement.namedItem("relations");
    if (relations.isNull()) {
        debugPlanInsertProjectXml<<"No relations";
        return;
    }
    KoXmlElement relationElement;
    forEachElement(relationElement, relations) {
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
