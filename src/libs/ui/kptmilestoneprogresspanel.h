/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTMILESTONEPROGRESSPANEL_H
#define KPTMILESTONEPROGRESSPANEL_H

#include "planui_export.h"

#include "ui_kptmilestoneprogresspanelbase.h"
#include "kpttask.h"


namespace KPlato
{

class MacroCommand;

class MilestoneProgressPanelImpl : public QWidget, public Ui_MilestoneProgressPanelBase {
    Q_OBJECT
public:
    explicit MilestoneProgressPanelImpl(QWidget *parent=nullptr);
    
    void enableWidgets();

Q_SIGNALS:
    void changed();
    
public Q_SLOTS:
    void slotChanged();
    void slotFinishedChanged(bool state);
};

class MilestoneProgressPanel : public MilestoneProgressPanelImpl {
    Q_OBJECT
public:
    explicit MilestoneProgressPanel(Task &task, QWidget *parent=nullptr);

    MacroCommand *buildCommand();

private:
    Task &m_task;
    Completion &m_completion;
};

}  //KPlato namespace

#endif // MILESTONEPROGRESSPANEL_H
