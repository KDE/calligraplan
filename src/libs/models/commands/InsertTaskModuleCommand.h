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

#ifndef INSERTTASKMODULECOMMAND_H
#define INSERTTASKMODULECOMMAND_H

#include "planmodels_export.h"

#include "kptcommand.h"
#include "kptxmlloaderobject.h"
#include "KoXmlReader.h"

#include <QHash>


class QString;

/// The main namespace
namespace KPlato
{

class Project;
class Node;


class PLANMODELS_EXPORT InsertTaskModuleCommand : public MacroCommand
{
public:
    InsertTaskModuleCommand(Project *project, const QByteArray &data, Node *parent, Node *position, const QMap<QString, QString> substitute, const KUndo2MagicString& name = KUndo2MagicString());
    ~InsertTaskModuleCommand() override;
    void execute() override;
    void unexecute() override;

private:
    void createCmdAccounts(const KoXmlElement &projectElement);
    void createCmdCalendars(const KoXmlElement &projectElement);
    void createCmdResources(const KoXmlElement &projectElement);
    void createCmdTasks(const KoXmlElement &projectElement);
    void createCmdTask(const KoXmlElement &parentElement, Node *parent, Node *position = nullptr);
    void createCmdRelations(const KoXmlElement &projectElement);

    void substitute(QString &text);

private:
    Project *m_project;
    QByteArray m_data;
    Node *m_parent;
    Node *m_position;
    QMap<QString, QString> m_substitute;
    bool m_first;
    XMLLoaderObject m_context;
    QHash<QString, Node*> m_oldIds; // QHash<taskid, task>
};


}  //KPlato namespace

#endif