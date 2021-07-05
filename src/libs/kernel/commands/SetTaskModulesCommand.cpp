/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "SetTaskModulesCommand.h"

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


const QLoggingCategory &PLANCMDSETTASKMODULES_LOG()
{
    static const QLoggingCategory category("calligra.plan.command.settaskmodules");
    return category;
}

#define debugPlanSetTaskModules qCDebug(PLANCMDSETTASKMODULES_LOG)
#define warnPlanSetTaskModules qCWarning(PLANCMDSETTASKMODULES_LOG)
#define errorPlanSetTaskModules qCCritical(PLANCMDSETTASKMODULES_LOG)

using namespace KPlato;

SetTaskModulesCommand::SetTaskModulesCommand(Project *project, const QList<QUrl> &modules, bool useLocalTaskModules, const KUndo2MagicString& name)
    : MacroCommand(name)
    , m_project(project)
    , newModules(modules)
    , newUseLocalTaskModules(useLocalTaskModules)
{
    oldModules = project->taskModules(false);
    oldUseLocalTaskModules = project->useLocalTaskModules();
}

SetTaskModulesCommand::~SetTaskModulesCommand()
{
}

void SetTaskModulesCommand::execute()
{
    m_project->setTaskModules(newModules, newUseLocalTaskModules);
}

void SetTaskModulesCommand::unexecute()
{
    m_project->setTaskModules(oldModules, oldUseLocalTaskModules);
}
