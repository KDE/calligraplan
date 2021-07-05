/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TASKSGENERALPANEL_H
#define TASKSGENERALPANEL_H

#include "planui_export.h"

#include "ui_TaskGeneralPanel.h"
#include "kptduration.h"

#include <QWidget>


namespace KPlato
{

class TasksGeneralPanel;
class Task;
class MacroCommand;
class Calendar;
class Project;

class TasksGeneralPanelImpl : public QWidget, public Ui_TasksGeneralPanel
{
    Q_OBJECT
public:
    TasksGeneralPanelImpl(QWidget *parent, const char *name);

    virtual int schedulingType() const;
    virtual int estimationType() const;
    virtual int optimistic() const;
    virtual int pessimistic();
    virtual double estimationValue();
    virtual QDateTime startDateTime();
    virtual QDateTime endDateTime();
    virtual QTime startTime() const;
    virtual QTime endTime();
    virtual QDate startDate();
    virtual QDate endDate();
    virtual int risktype() const;
    virtual Calendar *calendar() const;

public Q_SLOTS:
    virtual void setSchedulingType(int type);
    virtual void changeLeader();
    virtual void setEstimationType(int type);
    virtual void setOptimistic(int value);
    virtual void setPessimistic(int value);
    virtual void enableDateTime(int scheduleType);
    virtual void estimationTypeChanged(int type);
    virtual void setEstimate(double duration);
    virtual void setEstimateType(int type);
    virtual void checkAllFieldsFilled();
//    virtual void setEstimateScales(double day);
    virtual void startDateChanged();
    virtual void startTimeChanged(const QTime & time);
    virtual void endDateChanged();
    virtual void endTimeChanged(const QTime & time);
    virtual void scheduleTypeChanged(int value);
    virtual void setStartTime(const QTime & time);
    virtual void setEndTime(const QTime & time);
    virtual void setStartDateTime(const QDateTime & dt);
    virtual void setEndDateTime(const QDateTime & dt);
    virtual void setStartDate(const QDate & date);
    virtual void setEndDate(const QDate & date);
    virtual void setRisktype(int r);
    virtual void calendarChanged(int /*index*/);

Q_SIGNALS:
    void obligatedFieldsFilled(bool);
    void schedulingTypeChanged(int);
    void changed();

protected:
    bool useTime;
    QList<Calendar*> m_calendars;
};

class TasksGeneralPanel : public TasksGeneralPanelImpl {
    Q_OBJECT
public:
    explicit TasksGeneralPanel(Project &project, QList<Task*> &tasks, QWidget *parent=0, const char *name=0);

    MacroCommand *buildCommand();

    bool ok();

    void setStartValues(Task *task);

public Q_SLOTS:
    virtual void estimationTypeChanged(int type);
    virtual void scheduleTypeChanged(int value);

private:
    QList<Task*> m_tasks;
    Project &m_project;

    Duration m_estimate;
    Duration m_duration;
};

} //KPlato namespace

#endif // TASKSGENERALPANEL_H
