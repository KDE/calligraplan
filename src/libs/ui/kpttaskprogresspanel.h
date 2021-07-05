/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTTASKPROGRESSPANEL_H
#define KPTTASKPROGRESSPANEL_H

#include "planui_export.h"

#include "ui_kpttaskprogresspanelbase.h"
#include "kpttask.h"

#include <QWidget>


namespace KPlato
{

class StandardWorktime;
class Duration;
class ScheduleManager;
class MacroCommand;

//------------------------
class PLANUI_EXPORT TaskProgressPanelImpl : public QWidget, public Ui_TaskProgressPanelBase {
    Q_OBJECT
public:
    explicit TaskProgressPanelImpl(Task &task, QWidget *parent=nullptr);

    void enableWidgets();

    void setYear(int year);

Q_SIGNALS:
    void changed();

public Q_SLOTS:
    void slotChanged();
    void slotEditmodeChanged(int idx);
    void slotStartedChanged(bool state);
    void slotFinishedChanged(bool state);
    void slotPercentFinishedChanged(int value);
    void slotStartTimeChanged(const QDateTime &dt);
    void slotFinishTimeChanged(const QDateTime &dt);
    void slotEntryChanged();
    void slotSelectionChanged(const QItemSelection &sel);

    void slotPrevWeekBtnClicked();
    void slotNextWeekBtnClicked();

    void updateResourceCombo();
    void slotEffortChanged(const QDate &date);

protected Q_SLOTS:
    void slotCalculateEffort();
    void slotFillWeekNumbers(int year);

    void updateFinishedDateTime();

protected:
    void setFinished();

    Task &m_task;
    Completion &m_original;
    Completion m_completion;
    int m_dayLength;

    Duration scheduledEffort;
    int m_weekOffset;
    int m_year;
    bool m_firstIsPrevYear;
    bool m_lastIsNextYear;
};

class PLANUI_EXPORT TaskProgressPanel : public TaskProgressPanelImpl {
    Q_OBJECT
public:
    explicit TaskProgressPanel(Task &task, ScheduleManager *sm, StandardWorktime *workTime=nullptr, QWidget *parent=nullptr);

    MacroCommand *buildCommand();

    static MacroCommand *buildCommand(const Project &project, Completion &org, Completion &curr);

protected Q_SLOTS:
    void slotWeekNumberChanged(int);
    void slotEntryAdded(const QDate &date);
    void slotEntryRemoved(const QDate &date);
};

}  //KPlato namespace

#endif // TASKPROGRESSPANEL_H
