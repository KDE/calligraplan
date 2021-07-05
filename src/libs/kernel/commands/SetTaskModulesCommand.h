/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SETTASKMODULESCOMMAND_H
#define SETTASKMODULESCOMMAND_H

#include "plankernel_export.h"

#include "MacroCommand.h"
#include "kptxmlloaderobject.h"
#include "KoXmlReader.h"

#include <QHash>


class QString;

/// The main namespace
namespace KPlato
{

class Project;
class Node;


class PLANKERNEL_EXPORT SetTaskModulesCommand : public MacroCommand
{
public:
    SetTaskModulesCommand(Project *project, const QList<QUrl> &modules, bool useLocalTaskModules, const KUndo2MagicString& name = KUndo2MagicString());
    ~SetTaskModulesCommand() override;
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    QList<QUrl> oldModules;
    bool oldUseLocalTaskModules;
    QList<QUrl> newModules;
    bool newUseLocalTaskModules;
};


}  //KPlato namespace

#endif
