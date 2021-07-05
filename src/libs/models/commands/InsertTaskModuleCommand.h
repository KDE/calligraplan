/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    void createCmdRequests(const KoXmlElement &projectElement);

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
