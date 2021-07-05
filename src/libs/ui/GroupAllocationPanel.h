/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef GROUPALLOCATIONPANEL_H
#define GROUPALLOCATIONPANEL_H

#include "planui_export.h"

#include <QWidget>
#include <GroupAllocationItemModel.h>

class QTreeView;

namespace KPlato
{

class Task;
class Project;
class MacroCommand;

class GroupAllocationPanel : public QWidget
{
    Q_OBJECT
public:
    GroupAllocationPanel(QWidget *parent, Project &project, Task &task, bool baseline=false);

    /// Builds an undocommand for the current task
    /// only for changes (removals and/or additions)
    MacroCommand *buildCommand();
    /// Builds an undo command for @p task
    /// that clears all current requests and adds the new ones (if any)
    MacroCommand *buildCommand(Task *task);

    bool ok();

Q_SIGNALS:
    void changed();

private:
    GroupAllocationItemModel m_model;
    QTreeView *m_view;
};

}  //KPlato namespace

#endif
