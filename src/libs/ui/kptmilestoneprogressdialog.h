/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTMILESTONEPROGRESSDIALOG_H
#define KPTMILESTONEPROGRESSDIALOG_H

#include "planui_export.h"

#include <KoDialog.h>


namespace KPlato
{

class MilestoneProgressPanel;
class Task;
class Node;
class MacroCommand;

class PLANUI_EXPORT MilestoneProgressDialog : public KoDialog {
    Q_OBJECT
public:
    explicit MilestoneProgressDialog(Task &task, QWidget *parent=nullptr);

    MacroCommand *buildCommand();

protected Q_SLOTS:
    void slotChanged();
    void slotNodeRemoved(KPlato::Node *node);

private:
    Node *m_node;
    MilestoneProgressPanel *m_panel;

};

} //KPlato namespace

#endif // MILESTONEPROGRESSDIALOG_H
