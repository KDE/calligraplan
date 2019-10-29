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
        SaveNodes = 0x2
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
        return (options & SaveNodes) && nodes.contains(relation->parent()) && nodes.contains(relation->child());
    }
    void save() const {
        document = createDocument();
        QDomElement doc = document.documentElement();
        if (m_project) {
            m_project->save(doc, *this);
        }
        if (options & SaveNodes) {
            QDomElement element = doc.elementsByTagName("project").item(0).toElement();
            Q_ASSERT(!element.isNull());
            QListIterator<const Node*> it(nodes);
            while (it.hasNext()) {
                it.next()->save(element, *this);
            }
        }
    }

    mutable QDomDocument document;
    SaveOptions options;
    QList<const Node*> nodes;

private:
    Project *m_project;
};

} //namespace KPlato

#endif
