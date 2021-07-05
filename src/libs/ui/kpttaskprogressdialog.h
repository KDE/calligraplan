/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTTASKPROGRESSDIALOG_H
#define KPTTASKPROGRESSDIALOG_H

#include "planui_export.h"

#include <KoDialog.h>


namespace KPlato
{

class TaskProgressPanel;
class Task;
class Node;
class StandardWorktime;
class ScheduleManager;
class MacroCommand;

class PLANUI_EXPORT TaskProgressDialog : public KoDialog {
    Q_OBJECT
public:
    TaskProgressDialog(Task &task, ScheduleManager *sm, StandardWorktime *workTime, QWidget *parent=nullptr);

    MacroCommand *buildCommand();

protected Q_SLOTS:
    void slotChanged();
    void slotNodeRemoved(KPlato::Node *node);

private:
    Node *m_node;
    TaskProgressPanel *m_panel;

};

} //KPlato namespace

#endif // TASKPROGRESSDIALOG_H
