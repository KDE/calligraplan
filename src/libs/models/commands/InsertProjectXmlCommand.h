/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef INSERTPROJECTXMLCOMMAND_H
#define INSERTPROJECTXMLCOMMAND_H

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

class PLANMODELS_EXPORT AddTaskCommand : public NamedCommand
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

class PLANMODELS_EXPORT InsertProjectXmlCommand : public MacroCommand
{
public:
    InsertProjectXmlCommand(Project *project, const QByteArray &data, Node *parent, Node *position,  const KUndo2MagicString& name = KUndo2MagicString());
    ~InsertProjectXmlCommand() override;
    void execute() override;
    void unexecute() override;

private:
    void createCmdAccounts(const KoXmlElement &projectElement);
    void createCmdCalendars(const KoXmlElement &projectElement);
    void createCmdResources(const KoXmlElement &projectElement);
    void createCmdTasks(const KoXmlElement &projectElement);
    void createCmdTask(const KoXmlElement &parentElement, Node *parent, Node *position = nullptr);
    void createCmdRelations(const KoXmlElement &projectElement);
    void createCmdRequests(const KoXmlElement &projectElement);

private:
    Project *m_project;
    QByteArray m_data;
    Node *m_parent;
    Node *m_position;
    bool m_first;
    XMLLoaderObject m_context;
    QHash<QString, Node*> m_oldIds; // QHash<taskid, task>
};


}  //KPlato namespace

#endif
