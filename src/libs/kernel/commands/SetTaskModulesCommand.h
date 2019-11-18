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

#ifndef SETTASKMODULESCOMMAND_H
#define SETTASKMODULESCOMMAND_H

#include "plankernel_export.h"

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
