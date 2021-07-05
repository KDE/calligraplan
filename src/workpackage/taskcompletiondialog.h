/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009, 2011 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATOWORK_TASKCOMPLETIONDIALOG_H
#define KPLATOWORK_TASKCOMPLETIONDIALOG_H

#include "planwork_export.h"
#include "ui_taskcompletionpanel.h"

#include "workpackage.h"

#include "kptusedefforteditor.h"

#include <KoDialog.h>

#include <QWidget>

class KUndo2Command;

namespace KPlato {
    class ScheduleManager;
    class TaskProgressPanel;
}

namespace KPlatoWork
{

class TaskCompletionPanel;

class PLANWORK_EXPORT TaskCompletionDialog : public KoDialog
{
    Q_OBJECT
public:
    explicit TaskCompletionDialog(WorkPackage &package, KPlato::ScheduleManager *sm, QWidget *parent=nullptr);

    KUndo2Command *buildCommand();

protected Q_SLOTS:
    void slotChanged(bool);

private:
    TaskCompletionPanel *m_panel;
};

class PLANWORK_EXPORT TaskCompletionPanel : public QWidget
{
    Q_OBJECT
public:
    explicit TaskCompletionPanel(WorkPackage &package, KPlato::ScheduleManager *sm, QWidget *parent=nullptr);

    KUndo2Command *buildCommand();

    void enableWidgets();

Q_SIGNALS:
    void changed(bool);

public Q_SLOTS:
    void slotChanged();

private:
    KPlato::TaskProgressPanel *m_panel;
};

}  //KPlatoWork namespace


#endif
