/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef INSERTPROJECTCMD_H
#define INSERTPROJECTCMD_H

#include "plankernel_export.h"

#include "MacroCommand.h"

#include <kundo2command.h>


class QString;

/// The main namespace
namespace KPlato
{

class Locale;
class Account;
class Accounts;
class Calendar;
class Node;
class Project;

class PLANKERNEL_EXPORT InsertProjectCmd : public MacroCommand
{
public:
    InsertProjectCmd(Project &project, Node *parent, Node *after, const KUndo2MagicString& name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;

protected:
    void addAccounts(Account *account, Account *parent, QList<Account*> &unused, QMap<QString, Account*> &all);
    void addCalendars(Calendar *calendar, Calendar *parent, QList<Calendar*> &unused, QMap<QString, Calendar*> &all);
    void addChildNodes(Node *node);

private:
    Project *m_project;
    Node *m_parent;
    Node *m_after;

};


}  //KPlato namespace

#endif //INSERTPROJECTCMD_H
