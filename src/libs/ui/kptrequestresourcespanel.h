/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2003-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
