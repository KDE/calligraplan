/* This file is part of the KDE project
 * Copyright (C) 2003 - 2007 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#ifndef KPTREQUESTRESOURCESPANEL_H
#define KPTREQUESTRESOURCESPANEL_H

#include "planui_export.h"

#include <ui_ResourceAllocationPanel.h>
#include <kptresourceallocationmodel.h>

#include <QWidget>

class QTreeView;

namespace KPlato
{

class Task;
class Project;
class Resource;
class MacroCommand;

class RequestResourcesPanel : public QWidget
{
    Q_OBJECT
public:
    RequestResourcesPanel(QWidget *parent, Project &project, Task &task, bool baseline=false);

    /// Builds an undocommand for the current task
    /// only for changes (removals and/or additions)
    MacroCommand *buildCommand();
    /// Builds an undo command for @p task
    /// If @p clear is true, clears all current requests and adds the new ones (if any)
    MacroCommand *buildCommand(Task *task, bool clear = false);

    bool ok();

Q_SIGNALS:
    void changed();

private Q_SLOTS:
    void slotCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    Ui::ResourceAllocationPanel ui;
    ResourceAllocationItemModel m_model;
};

}  //KPlato namespace

#endif
