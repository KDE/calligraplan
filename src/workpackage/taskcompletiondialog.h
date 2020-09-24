/* This file is part of the KDE project
   Copyright (C) 2009, 2011 Dag Andersen <dag.andersen@kdemail.net>
   Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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
    explicit TaskCompletionDialog(WorkPackage &package, ScheduleManager *sm, QWidget *parent=nullptr);

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
    explicit TaskCompletionPanel(WorkPackage &package, ScheduleManager *sm, QWidget *parent=nullptr);

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
