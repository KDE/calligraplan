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
