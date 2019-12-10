/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation;
 * version 2 of the License.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
    XmlSaveContext(Project *project = 0)
    : options(SaveAll)
    , m_project(project)
    {}

    static QDomDocument createDocument() {
        QDomDocument document("plan");
        document.appendChild(document.createProcessingInstruction(
            "xml",
            "version=\"1.0\" encoding=\"UTF-8\"") );
        
        QDomElement doc = document.createElement("plan");
        doc.setAttribute("editor", "Plan");
        doc.setAttribute("mime", "application/x-vnd.kde.plan");
        doc.setAttribute("version", PLAN_FILE_SYNTAX_VERSION);
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
            doc.appendChild(doc.ownerDocument().createElement("project"));
        }
        QDomElement projectElement = doc.elementsByTagName("project").item(0).toElement();
        Q_ASSERT(!projectElement.isNull());
        if (options & SaveSelectedNodes) {
            debugPlanXml<<"tasks:"<<nodes.count();
            QListIterator<const Node*> it(nodes);
            while (it.hasNext()) {
                it.next()->save(projectElement, *this);
            }
        }
        if (options & SaveRelations) {
            QListIterator<const Node*> it(nodes);
            while (it.hasNext()) {
                it.next()->saveRelations(projectElement, *this);
            }
        }
        if (options & SaveResources) {
            debugPlanXml<<"resource-groups:"<<m_project->resourceGroups().count();
            if (!m_project->resourceGroups().isEmpty()) {
                QDomElement ge = projectElement.ownerDocument().createElement("resource-groups");
                projectElement.appendChild(ge);
                QListIterator<ResourceGroup*> git(m_project->resourceGroups());
                while (git.hasNext()) {
                    git.next()->save(ge);
                }
            }
            debugPlanXml<<"resources:"<<m_project->resourceList().count();
            if (!m_project->resourceList().isEmpty()) {
                QDomElement re = projectElement.ownerDocument().createElement("resources");
                projectElement.appendChild(re);
                QListIterator<Resource*> rit(m_project->resourceList());
                while (rit.hasNext()) {
                    rit.next()->save(re);
                }
            }
            debugPlanXml<<"resource-group-relations";
            if (!m_project->resourceList().isEmpty() && !m_project->resourceGroups().isEmpty()) {
                QDomElement e = projectElement.ownerDocument().createElement("resource-group-relations");
                projectElement.appendChild(e);
                for (ResourceGroup *g : m_project->resourceGroups()) {
                    for (Resource *r : g->resources()) {
                        QDomElement re = e.ownerDocument().createElement("resource-group-relation");
                        e.appendChild(re);
                        re.setAttribute("group-id", g->id());
                        re.setAttribute("resource-id", r->id());
                    }
                }
            }
            debugPlanXml<<"required-resources";
            if (m_project->resourceList().count() > 1) {
                QList<std::pair<QString, QString> > requiredList;
                for (Resource *resource : m_project->resourceList()) {
                    for (const QString &required : resource->requiredIds()) {
                        requiredList << std::pair<QString, QString>(resource->id(), required);
                    }
                }
                if (!requiredList.isEmpty()) {
                    QDomElement e = projectElement.ownerDocument().createElement("required-resources");
                    projectElement.appendChild(e);
                    for (const std::pair<QString, QString> pair : requiredList) {
                        QDomElement re = e.ownerDocument().createElement("required-resource");
                        e.appendChild(re);
                        re.setAttribute("resource-id", pair.first);
                        re.setAttribute("required-id", pair.second);
                    }
                }
            }
            // save resource teams
            debugPlanXml<<"resource-teams";
            QDomElement el = projectElement.ownerDocument().createElement("resource-teams");
            projectElement.appendChild(el);
            foreach (Resource *r, m_project->resourceList()) {
                if (r->type() != Resource::Type_Team) {
                    continue;
                }
                foreach (const QString &id, r->teamMemberIds()) {
                    QDomElement e = el.ownerDocument().createElement("team");
                    el.appendChild(e);
                    e.setAttribute("team-id", r->id());
                    e.setAttribute("member-id", id);
                }
            }
        }
        if (options & SaveRequests) {
            // save resource requests
            QHash<Task*, ResourceGroupRequest*> groups;
            QHash<Task*, ResourceRequest*> resources;
            for (Task *task : m_project->allTasks()) {
                const ResourceRequestCollection &requests = task->requests();
                for (ResourceGroupRequest *gr : requests.requests()) {
                    groups.insert(task, gr);
                }
                for (ResourceRequest *rr : requests.resourceRequests(false)) {
                    resources.insert(task, rr);
                }
            }
            debugPlanXml<<"resourcegroup-requests:"<<groups.count();
            if (!groups.isEmpty()) {
                QDomElement el = projectElement.ownerDocument().createElement("resourcegroup-requests");
                projectElement.appendChild(el);
                QHash<Task*, ResourceGroupRequest*>::const_iterator it;
                for (it = groups.constBegin(); it != groups.constEnd(); ++it) {
                    if (!it.value()->group()) {
                        warnPlanXml<<"resourcegroup-request with no group";
                        continue;
                    }
                    QDomElement ge = el.ownerDocument().createElement("resourcegroup-request");
                    el.appendChild(ge);
                    ge.setAttribute("request-id", it.value()->id());
                    ge.setAttribute("task-id", it.key()->id());
                    ge.setAttribute("group-id", it.value()->group()->id());
                    ge.setAttribute("units", QString::number(it.value()->units()));
                }
            }
            QHash<Task*, std::pair<ResourceRequest*, Resource*> > required; // QHash<Task*, std::pair<ResourceRequest*, Required*>>
            debugPlanXml<<"resource-requests:"<<resources.count();
            if (!resources.isEmpty()) {
                QDomElement el = projectElement.ownerDocument().createElement("resource-requests");
                projectElement.appendChild(el);
                QHash<Task*, ResourceRequest*>::const_iterator it;
                for (it = resources.constBegin(); it != resources.constEnd(); ++it) {
                    if (!it.value()->resource()) {
                        continue;
                    }
                    QDomElement re = el.ownerDocument().createElement("resource-request");
                    el.appendChild(re);
                    re.setAttribute("request-id", it.value()->id());
                    re.setAttribute("task-id", it.key()->id());
                    if (it.value()->parent()) {
                        re.setAttribute("group-id", it.value()->parent()->group()->id());
                    }
                    re.setAttribute("resource-id", it.value()->resource()->id());
                    re.setAttribute("units", QString::number(it.value()->units()));
                    // collect required resources
                    for (Resource *r : it.value()->requiredResources()) {
                        required.insert(it.key(), std::pair<ResourceRequest*, Resource*>(it.value(), r));
                    }
                }
            }
        }
    }

    mutable QDomDocument document;
    int options;
    QList<const Node*> nodes;

private:
    Project *m_project;
};

} //namespace KPlato

#endif
