/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef XMLSAVECONTEXT_H
#define XMLSAVECONTEXT_H

#include "plankernel_export.h"

#include "kptproject.h"
#include "kptnode.h"
#include "kpttask.h"

#include <QList>
#include <QDomDocument>
#include <QDomElement>

namespace KPlato 
{

class PLANKERNEL_EXPORT XmlSaveContext
{
public:
    XmlSaveContext(Project *project = nullptr)
    : options(SaveAll)
    , m_project(project)
    {}

    static QDomDocument createDocument() {
        QDomDocument document(QStringLiteral("plan"));
        document.appendChild(document.createProcessingInstruction(
            QStringLiteral("xml"),
            QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));
        
        QDomElement doc = document.createElement(QStringLiteral("plan"));
        doc.setAttribute(QStringLiteral("editor"), QStringLiteral("Plan"));
        doc.setAttribute(QStringLiteral("mime"), QStringLiteral("application/x-vnd.kde.plan"));
        doc.setAttribute(QStringLiteral("version"), PLAN_FILE_SYNTAX_VERSION);
        document.appendChild(doc);
        return document;
    }

    enum SaveOptions {
        SaveAll = 0x1,
        SaveSelectedNodes = 0x2,
        SaveRelations = 0x4,
        SaveResources = 0x8,
        SaveRequests = 0x10,
        SaveProject = 0x20
    };
    bool saveAll(const Project *project) const {
        Q_UNUSED(project);
        return options & SaveAll;
    }
    bool saveAll(const Task *task) const {
        Q_UNUSED(task);
        return options & SaveAll;
    }
    bool saveNode(const Node *node) const {
        if (options & SaveAll) {
            return true;
        }
        return nodes.contains(node);
    }
    bool saveChildren(const Node *node) const {
        Q_UNUSED(node);
        if (options & SaveAll) {
            return true;
        }
        return false; // TODO
    }
    bool saveRelation(const Relation *relation) const {
        if (options & SaveAll) {
            return true;
        }
        return (options & SaveSelectedNodes) && nodes.contains(relation->parent()) && nodes.contains(relation->child());
    }
    void save() const {
        Q_ASSERT(m_project);
        if (!m_project) {
            return;
        }
        document = createDocument();
        QDomElement doc = document.documentElement();

        if (options & (SaveAll | SaveProject)) {
            debugPlanXml<<"project";
            m_project->save(doc, *this);
            if (options & SaveAll) {
                return;
            }
        } else {
            doc.appendChild(doc.ownerDocument().createElement(QStringLiteral("project")));
        }
        QDomElement projectElement = doc.elementsByTagName(QStringLiteral("project")).item(0).toElement();
        Q_ASSERT(!projectElement.isNull());
        if (options & SaveSelectedNodes) {
            debugPlanXml<<"tasks:"<<nodes.count();
            QListIterator<const Node*> it(nodes);
            while (it.hasNext()) {
                it.next()->save(projectElement, *this);
            }
        }
        if (options & SaveRelations) {
            QDomElement e = projectElement.ownerDocument().createElement(QStringLiteral("relations"));
            projectElement.appendChild(e);
            QListIterator<const Node*> it(nodes);
            while (it.hasNext()) {
                it.next()->saveRelations(e, *this);
            }
        }
        if (options & SaveResources) {
            const auto resourceGroups = m_project->resourceGroups();
            debugPlanXml<<"resource-groups:"<<resourceGroups.count();
            if (!resourceGroups.isEmpty()) {
                QDomElement ge = projectElement.ownerDocument().createElement(QStringLiteral("resource-groups"));
                projectElement.appendChild(ge);
                QListIterator<ResourceGroup*> git(m_project->resourceGroups());
                while (git.hasNext()) {
                    git.next()->save(ge);
                }
            }
            const auto resourceList = m_project->resourceList();
            debugPlanXml<<"resources:"<<resourceList.count();
            if (!resourceList.isEmpty()) {
                QDomElement re = projectElement.ownerDocument().createElement(QStringLiteral("resources"));
                projectElement.appendChild(re);
                QListIterator<Resource*> rit(resourceList);
                while (rit.hasNext()) {
                    rit.next()->save(re);
                }
            }
            debugPlanXml<<"resource-group-relations";
            if (!resourceList.isEmpty() && !resourceGroups.isEmpty()) {
                QDomElement e = projectElement.ownerDocument().createElement(QStringLiteral("resource-group-relations"));
                projectElement.appendChild(e);
                for (const ResourceGroup *g : resourceGroups) {
                    const QList<Resource*> resources = g->resources();
                    for (const Resource *r : resources) {
                        QDomElement re = e.ownerDocument().createElement(QStringLiteral("resource-group-relation"));
                        e.appendChild(re);
                        re.setAttribute(QStringLiteral("group-id"), g->id());
                        re.setAttribute(QStringLiteral("resource-id"), r->id());
                    }
                }
            }
            debugPlanXml<<"required-resources";
            if (resourceList.count() > 1) {
                QList<std::pair<QString, QString> > requiredList;
                for (const Resource *resource : resourceList) {
                    const QStringList requiredIds = resource->requiredIds();
                    for (const QString &required : requiredIds) {
                        requiredList << std::pair<QString, QString>(resource->id(), required);
                    }
                }
                if (!requiredList.isEmpty()) {
                    QDomElement e = projectElement.ownerDocument().createElement(QStringLiteral("required-resources"));
                    projectElement.appendChild(e);
                    for (const std::pair<QString, QString> &pair : qAsConst(requiredList)) {
                        QDomElement re = e.ownerDocument().createElement(QStringLiteral("required-resource"));
                        e.appendChild(re);
                        re.setAttribute(QStringLiteral("resource-id"), pair.first);
                        re.setAttribute(QStringLiteral("required-id"), pair.second);
                    }
                }
            }
            // save resource teams
            debugPlanXml<<"resource-teams";
            QDomElement el = projectElement.ownerDocument().createElement(QStringLiteral("resource-teams"));
            projectElement.appendChild(el);
            for (const Resource *r : resourceList) {
                if (r->type() != Resource::Type_Team) {
                    continue;
                }
                const auto ids = r->teamMemberIds();
                for (const QString &id : ids) {
                    QDomElement e = el.ownerDocument().createElement(QStringLiteral("team"));
                    el.appendChild(e);
                    e.setAttribute(QStringLiteral("team-id"), r->id());
                    e.setAttribute(QStringLiteral("member-id"), id);
                }
            }
        }
        if (options & SaveRequests) {
            // save resource requests
            QHash<Task*, ResourceRequest*> resources;
            const auto allTasks = m_project->allTasks();
            for (Task *task : allTasks) {
                const auto requests = task->requests().resourceRequests(false);
                for (ResourceRequest *rr : requests) {
                    resources.insert(task, rr);
                }
            }
            debugPlanXml<<"resource-requests:"<<resources.count();
            QMultiHash<Task*, std::pair<ResourceRequest*, Resource*> > required; // QHash<Task*, std::pair<ResourceRequest*, Required*>>
            QMultiHash<Task*, std::pair<ResourceRequest*, ResourceRequest*> > alternativeRequests; // QHash<Task*, std::pair<ResourceRequest*, Alternative*>>
            if (!resources.isEmpty()) {
                QDomElement el = projectElement.ownerDocument().createElement(QStringLiteral("resource-requests"));
                projectElement.appendChild(el);
                QHash<Task*, ResourceRequest*>::const_iterator it;
                for (it = resources.constBegin(); it != resources.constEnd(); ++it) {
                    if (!it.value()->resource()) {
                        continue;
                    }
                    QDomElement re = el.ownerDocument().createElement(QStringLiteral("resource-request"));
                    el.appendChild(re);
                    re.setAttribute(QStringLiteral("request-id"), it.value()->id());
                    re.setAttribute(QStringLiteral("task-id"), it.key()->id());
                    re.setAttribute(QStringLiteral("resource-id"), it.value()->resource()->id());
                    re.setAttribute(QStringLiteral("units"), QString::number(it.value()->units()));
                    // collect required resources and alternative requests
                    const auto requiredResources = it.value()->requiredResources();
                    for (Resource *r : requiredResources) {
                        required.insert(it.key(), std::pair<ResourceRequest*, Resource*>(it.value(), r));
                    }
                    const auto altRequests = it.value()->alternativeRequests();
                    for (ResourceRequest *r : altRequests) {
                        alternativeRequests.insert(it.key(), std::pair<ResourceRequest*, ResourceRequest*>(it.value(), r));
                    }
                }
            }
            debugPlanXml<<"required-resource-requests:"<<required.count();
            if (!required.isEmpty()) {
                QDomElement reqs = projectElement.ownerDocument().createElement(QStringLiteral("required-resource-requests"));
                projectElement.appendChild(reqs);
                //QHash<Task*, std::pair<ResourceRequest*, Resource*> >::const_iterator it;
                for (auto it = required.constBegin(); it != required.constEnd(); ++it) {
                    QDomElement req = reqs.ownerDocument().createElement(QStringLiteral("required-resource-request"));
                    reqs.appendChild(req);
                    req.setAttribute(QStringLiteral("task-id"), it.key()->id());
                    req.setAttribute(QStringLiteral("request-id"), it.value().first->id());
                    req.setAttribute(QStringLiteral("required-id"), it.value().second->id());
                }
            }
            debugPlanXml<<"alternative-requests:"<<alternativeRequests.count();
            if (!alternativeRequests.isEmpty()) {
                QDomElement reqs = projectElement.ownerDocument().createElement(QStringLiteral("alternative-requests"));
                projectElement.appendChild(reqs);
                //QHash<Task*, std::pair<ResourceRequest*, ResourceRequest*> >::const_iterator it;
                for (auto it = alternativeRequests.constBegin(); it != alternativeRequests.constEnd(); ++it) {
                    QDomElement req = reqs.ownerDocument().createElement(QStringLiteral("alternative-request"));
                    reqs.appendChild(req);
                    req.setAttribute(QStringLiteral("task-id"), it.key()->id());
                    req.setAttribute(QStringLiteral("request-id"), it.value().first->id());
                    req.setAttribute(QStringLiteral("resource-id"), it.value().second->resource()->id());
                    req.setAttribute(QStringLiteral("units"), it.value().second->units());
                }
            }
        }
    }

    bool saveWorkIntervalsCache() {
        if (!m_project) {
            return false;
        }
        const auto resources = m_project->resourceList();
        if (resources.isEmpty()) {
            return false;
        }
        document = createDocument();
        for (const Resource *r : resources) {
            QDomElement doc = document.documentElement();
            QDomElement me = document.createElement(QStringLiteral("resource"));
            doc.appendChild(me);
            me.setAttribute(QStringLiteral("id"), r->id());
            me.setAttribute(QStringLiteral("name"), r->name());
            r->saveCalendarIntervalsCache(me);
        }
        return true;
    }

    mutable QDomDocument document;
    int options;
    QList<const Node*> nodes;

private:
    Project *m_project;
};

} //namespace KPlato

#endif
