/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2007, 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpttaskgeneralpanel.h"
#include "kpttaskdialog.h"
#include "kpttask.h"
#include "kptcommand.h"
#include "kptduration.h"
#include "kptdurationspinbox.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptproject.h"

#include <KLocalizedString>

#ifdef PLAN_KDEPIMLIBS_FOUND
#include <akonadi/contact/emailaddressselectiondialog.h>
#include <akonadi/contact/emailaddressselectionwidget.h>
#include <akonadi/contact/emailaddressselection.h>
#endif

#include <QDateTime>

#include <kptdebug.h>

namespace KPlato
{

TaskGeneralPanel::TaskGeneralPanel(Project &project, Task &task, QWidget *p)
    : TaskGeneralPanelImpl(p),
      m_task(task),
      m_project(project)
{
    useTime = true;
    setStartValues(task);
    QString s = i18n("The Work Breakdown Structure introduces numbering for all tasks in the project, according to the task structure.\nThe WBS code is auto-generated.\nYou can define the WBS code pattern using the Define WBS Pattern command in the Tools menu.");
    wbslabel->setWhatsThis(s);
    wbsfield->setWhatsThis(s);

    if (task.isBaselined(BASELINESCHEDULE)) {
        schedulingGroup->setEnabled(false);
    }

}

void TaskGeneralPanel::setStartValues(Task &task) {
    m_estimate = m_duration = task.estimate()->expectedValue();
    namefield->setText(task.name());
    ui_priority->setValue(task.priority());
    leaderfield->setText(task.leader());
    wbsfield->setText(task.wbsCode());

    int cal = 0;
    m_calendars.clear();
    calendarCombo->addItem(i18n("None"));
    m_calendars.insert(0, nullptr);
    const QList<Calendar*> list = m_project.allCalendars();
    int i=1;
    for (Calendar *c : list) {
        calendarCombo->insertItem(i, c->name());
        m_calendars.insert(i, c);
        if (c == task.estimate()->calendar()) {
            cal = i;
        }
        ++i;
    }
    calendarCombo->setCurrentIndex(cal);

    estimate->setMinimumUnit((Duration::Unit)(m_project.config().minimumDurationUnit()));
    estimate->setMaximumUnit((Duration::Unit)(m_project.config().maximumDurationUnit()));
    estimate->setUnit(task.estimate()->unit());
    setEstimateType(task.estimate()->type());
    if (task.estimate()->type() == Estimate::Type_Effort && task.estimate()->expectedEstimate() == 0.0) {
        setEstimateType(2 /*Milestone*/);
    }
    setSchedulingType(task.constraint());
    if (task.constraintStartTime().isValid()) {
        setStartDateTime(task.constraintStartTime());
    } else {
        QDate date = QDate::currentDate();
        setStartDateTime(QDateTime(date, QTime(), Qt::LocalTime));
    }
    if (task.constraintEndTime().isValid()) {
        setEndDateTime(task.constraintEndTime());
    } else {
        setEndDateTime(QDateTime(startDate().addDays(1), QTime(), Qt::LocalTime));
    }
    //debugPlan<<"Estimate:"<<task.estimate()->expected().toString();
    setEstimate(task.estimate()->expectedEstimate());
    setOptimistic(task.estimate()->optimisticRatio());
    setPessimistic(task.estimate()->pessimisticRatio());
    setRisktype(task.estimate()->risktype());
    
    namefield->setFocus();
}

MacroCommand *TaskGeneralPanel::buildCommand() {
    MacroCommand *cmd = new MacroCommand(kundo2_i18n("Modify Task"));
    bool modified = false;

    if ((!namefield->isHidden()) && m_task.name() != namefield->text()) {
        cmd->addCommand(new NodeModifyNameCmd(m_task, namefield->text()));
        modified = true;
    }
    if (ui_priority->value() != m_task.priority()) {
        cmd->addCommand(new NodeModifyPriorityCmd(m_task, m_task.priority(), ui_priority->value()));
        modified = true;
    }
    if ((!leaderfield->isHidden()) && m_task.leader() != leaderfield->text()) {
        cmd->addCommand(new NodeModifyLeaderCmd(m_task, leaderfield->text()));
        modified = true;
    }
    Node::ConstraintType c = (Node::ConstraintType)schedulingType();
    if (c != m_task.constraint()) {
        cmd->addCommand(new NodeModifyConstraintCmd(m_task, c));
        modified = true;
    }
    if (startDateTime() != m_task.constraintStartTime() &&
        (c == Node::FixedInterval || c == Node::StartNotEarlier || c == Node::MustStartOn)) {
        cmd->addCommand(new NodeModifyConstraintStartTimeCmd(m_task, startDateTime()));
        modified = true;
    }
    if (endDateTime() != m_task.constraintEndTime() &&
        (c == Node::FinishNotLater || c == Node::FixedInterval || c == Node::MustFinishOn)) {
        cmd->addCommand(new NodeModifyConstraintEndTimeCmd(m_task, endDateTime()));
        modified = true;
    }
    int et = estimationType();
    if (et == 2 /*Milestome*/) {
        et = 0; /*Effort*/
    }
    if (et != m_task.estimate()->type()) {
        cmd->addCommand(new ModifyEstimateTypeCmd(m_task,  m_task.estimate()->type(), et));
        modified = true;
    }
    bool unitchanged = estimate->unit() != m_task.estimate()->unit();
    if (unitchanged) {
        cmd->addCommand(new ModifyEstimateUnitCmd(m_task, m_task.estimate()->unit(), estimate->unit()));
        modified = true;
    }
    bool expchanged = estimationValue() != m_task.estimate()->expectedEstimate();
    if (expchanged) {
        cmd->addCommand(new ModifyEstimateCmd(m_task, m_task.estimate()->expectedEstimate(), estimationValue()));
        modified = true;
    }
    int x = optimistic();
    if (x != m_task.estimate()->optimisticRatio() || expchanged || unitchanged) {
        cmd->addCommand(new EstimateModifyOptimisticRatioCmd(m_task, m_task.estimate()->optimisticRatio(), x));
        modified = true;
    }
    x = pessimistic();
    if (x != m_task.estimate()->pessimisticRatio() || expchanged || unitchanged) {
        cmd->addCommand(new EstimateModifyPessimisticRatioCmd(m_task, m_task.estimate()->pessimisticRatio(), x));
        modified = true;
    }
    if (m_task.estimate()->risktype() != risktype()) {
        cmd->addCommand(new EstimateModifyRiskCmd(m_task, m_task.estimate()->risktype(), risktype()));
        modified = true;
    }
    if (m_task.estimate()->calendar() != calendar()) {
        cmd->addCommand(new ModifyEstimateCalendarCmd(m_task, m_task.estimate()->calendar(), calendar()));
        modified = true;
    }
    if (!modified) {
        delete cmd;
        return nullptr;
    }
    return cmd;
}

bool TaskGeneralPanel::ok() {
    return true;
}

void TaskGeneralPanel::estimationTypeChanged(int type) {
    if (type == 0 /*Effort*/) {
        estimate->setEnabled(true);
        calendarCombo->setEnabled(false);
    } else if (type == 1 /*Duration*/) {
        calendarCombo->setEnabled(false);
        if (schedulingType() == 6) { /*Fixed interval*/
            estimate->setEnabled(false);
        } else {
            estimate->setEnabled(true);
            calendarCombo->setEnabled(true);
        }
    } else if (type == 2 /* Milestone */) {
        estimate->setValue(0);
        estimate->setEnabled(false);
        calendarCombo->setEnabled(false);
    }
    TaskGeneralPanelImpl::estimationTypeChanged(type);
}

void TaskGeneralPanel::scheduleTypeChanged(int value)
{
    if (value == 6 /*Fixed interval*/) {
        if (estimateType->currentIndex() == 1/*duration*/){
//            setEstimateScales(24);
            estimate->setEnabled(false);
//TODO            setEstimate(DateTime(endDateTime(), KDateTime::UTC) - DateTime(startDateTime(), KDateTime::UTC));
        }
    } else {
        estimate->setEnabled(true);
    }
    TaskGeneralPanelImpl::scheduleTypeChanged(value);
}

//-----------------------------
TaskGeneralPanelImpl::TaskGeneralPanelImpl(QWidget *p)
    : QWidget(p)
{
    setupUi(this);

#ifndef PLAN_KDEPIMLIBS_FOUND
    chooseLeader->hide();
#endif

    // FIXME
    // [Bug 311940] New: Plan crashes when typing a text in the filter textbox before the textbook is fully loaded when selecting a contact from the addressbook
    chooseLeader->hide();

    connect(namefield, &QLineEdit::textChanged, this, &TaskGeneralPanelImpl::checkAllFieldsFilled);
    connect(leaderfield, &QLineEdit::textChanged, this, &TaskGeneralPanelImpl::checkAllFieldsFilled);
    connect(chooseLeader, &QAbstractButton::clicked, this, &TaskGeneralPanelImpl::changeLeader);
    connect(estimateType, SIGNAL(activated(int)), SLOT(estimationTypeChanged(int)));
    connect(scheduleType, SIGNAL(activated(int)), SLOT(scheduleTypeChanged(int)));
    connect(scheduleStartDate, &QDateTimeEdit::dateChanged, this, &TaskGeneralPanelImpl::startDateChanged);
    connect(scheduleStartTime, &QDateTimeEdit::timeChanged, this, &TaskGeneralPanelImpl::startTimeChanged);
    connect(scheduleEndDate, &QDateTimeEdit::dateChanged, this, &TaskGeneralPanelImpl::endDateChanged);
    connect(scheduleEndTime, &QDateTimeEdit::timeChanged, this, &TaskGeneralPanelImpl::endTimeChanged);
    connect(estimate, SIGNAL(valueChanged(double)), SLOT(checkAllFieldsFilled()));
    connect(optimisticValue, SIGNAL(valueChanged(int)), SLOT(checkAllFieldsFilled()));
    connect(pessimisticValue, SIGNAL(valueChanged(int)), SLOT(checkAllFieldsFilled()));
    connect(risk, SIGNAL(activated(int)), SLOT(checkAllFieldsFilled()));
    connect(calendarCombo, SIGNAL(activated(int)), SLOT(calendarChanged(int)));

}

void TaskGeneralPanelImpl::setSchedulingType(int type)
{
    enableDateTime(type);
    scheduleType->setCurrentIndex(type);
    Q_EMIT schedulingTypeChanged(type);
}

int TaskGeneralPanelImpl::schedulingType() const
{
    return scheduleType->currentIndex();
}

void TaskGeneralPanelImpl::changeLeader()
{
#ifdef PLAN_KDEPIMLIBS_FOUND
    QPointer<Akonadi::EmailAddressSelectionDialog> dlg = new Akonadi::EmailAddressSelectionDialog(this);
    if (dlg->exec() && dlg) {
        QStringList names;
        const Akonadi::EmailAddressSelection::List selections = dlg->selectedAddresses();
        for (const Akonadi::EmailAddressSelection &selection : selections) {
            QString s = selection.name();
            if (! selection.email().isEmpty()) {
                if (! selection.name().isEmpty()) {
                    s += " <";
                }
                s += selection.email();
                if (! selection.name().isEmpty()) {
                    s += '>';
                }
                if (! s.isEmpty()) {
                    names << s;
                }
            }
        }
        if (! names.isEmpty()) {
            leaderfield->setText(names.join(", "));
        }
    }
#endif
}

void TaskGeneralPanelImpl::setEstimationType(int type)
{
    estimateType->setCurrentIndex(type);
}

int TaskGeneralPanelImpl::estimationType() const
{
    return estimateType->currentIndex();
}

void TaskGeneralPanelImpl::setOptimistic(int value)
{
    optimisticValue->setValue(value);
}

void TaskGeneralPanelImpl::setPessimistic(int value)
{
    pessimisticValue->setValue(value);
}

int TaskGeneralPanelImpl::optimistic() const
{
    return optimisticValue->value();
}

int TaskGeneralPanelImpl::pessimistic()
{
    return pessimisticValue->value();
}

void TaskGeneralPanelImpl::enableDateTime(int scheduleType)
{
    scheduleStartTime->setEnabled(false);
    scheduleEndTime->setEnabled(false);
    scheduleStartDate->setEnabled(false);
    scheduleEndDate->setEnabled(false);
    switch (scheduleType)
    {
    case 0: //ASAP
    case 1: //ALAP
        break;
    case 2: //Must start on
    case 4: // Start not earlier
        if (useTime) {
            scheduleStartTime->setEnabled(true);
            scheduleEndTime->setEnabled(false);
        }
        scheduleStartDate->setEnabled(true);
        scheduleEndDate->setEnabled(false);
        break;
    case 3: //Must finish on
    case 5: // Finish not later
        if (useTime) {
            scheduleStartTime->setEnabled(false);
            scheduleEndTime->setEnabled(true);
        }
        scheduleStartDate->setEnabled(false);
        scheduleEndDate->setEnabled(true);
        break;
    case 6: //Fixed interval
        if (useTime) {
            scheduleStartTime->setEnabled(true);
            scheduleEndTime->setEnabled(true);
        }
        scheduleStartDate->setEnabled(true);
        scheduleEndDate->setEnabled(true);
        break;
    default:
        break;
    }
}


void TaskGeneralPanelImpl::estimationTypeChanged(int /*type*/)
{
    checkAllFieldsFilled();
}

void TaskGeneralPanelImpl::calendarChanged(int /*index*/)
{
    checkAllFieldsFilled();
}

void TaskGeneralPanelImpl::setEstimate(double duration)
{
    estimate->setValue(duration);
}


void TaskGeneralPanelImpl::setEstimateType(int type)
{
    estimateType->setCurrentIndex(type);
    estimationTypeChanged(type);
}


void TaskGeneralPanelImpl::checkAllFieldsFilled()
{
    Q_EMIT changed();
    Q_EMIT obligatedFieldsFilled(true); // do not block save even if name is not filled
}


double TaskGeneralPanelImpl::estimationValue()
{
    return estimate->value();
}

void TaskGeneralPanelImpl::startDateChanged()
{
    if (!scheduleStartDate->isEnabled()) {
        return;
    }
    QDate date = startDate();
    if (startDateTime() > endDateTime())
    {
        scheduleEndTime->blockSignals(true);
        scheduleEndDate->blockSignals(true);
        setEndDate(date);
        setEndTime(startTime());
        scheduleEndTime->blockSignals(false);
        scheduleEndDate->blockSignals(false);
    }
    if (scheduleType->currentIndex() == 6 /*FixedInterval*/)
    {
        estimationTypeChanged(estimateType->currentIndex());
    }
    checkAllFieldsFilled();
}

void TaskGeneralPanelImpl::startTimeChanged(const QTime &time)
{
    if (!scheduleStartTime->isEnabled()) {
        return;
    }
    if (startDateTime() > endDateTime())
    {
        scheduleEndTime->blockSignals(true);
        setEndTime(time);
        scheduleEndTime->blockSignals(false);
    }
    if (scheduleType->currentIndex() == 6 /*FixedInterval*/)
    {
        estimationTypeChanged(estimateType->currentIndex());
    }
    checkAllFieldsFilled();
}


void TaskGeneralPanelImpl::endDateChanged()
{
    if (!scheduleEndDate->isEnabled()) {
        return;
    }
    QDate date = endDate();
    if (endDateTime() < startDateTime())
    {
        scheduleStartTime->blockSignals(true);
        scheduleStartDate->blockSignals(true);
        setStartDate(date);
        setStartTime(endTime());
        scheduleStartTime->blockSignals(false);
        scheduleStartDate->blockSignals(false);
    }

    if (scheduleType->currentIndex() == 6 /*FixedInterval*/)
    {
        estimationTypeChanged(estimateType->currentIndex());
    }
    checkAllFieldsFilled();
}

void TaskGeneralPanelImpl::endTimeChanged(const QTime &time)
{
    if (!scheduleEndTime->isEnabled()) {
        return;
    }
    if (endDateTime() < startDateTime())
    {
        scheduleStartTime->blockSignals(true);
        setStartTime(time);
        scheduleStartTime->blockSignals(false);
    }

    if (scheduleType->currentIndex() == 6 /*FixedInterval*/)
    {
        estimationTypeChanged(estimateType->currentIndex());
    }
    checkAllFieldsFilled();
}

void TaskGeneralPanelImpl::scheduleTypeChanged(int value)
{
     estimationTypeChanged(estimateType->currentIndex());
     enableDateTime(value);
     checkAllFieldsFilled();
}


QDateTime TaskGeneralPanelImpl::startDateTime()
{
    return QDateTime(startDate(), startTime(), Qt::LocalTime);
}


QDateTime TaskGeneralPanelImpl::endDateTime()
{
    return QDateTime(endDate(), endTime(), Qt::LocalTime);
}

void TaskGeneralPanelImpl::setStartTime(const QTime &time)
{
    scheduleStartTime->setTime(QTime(time.hour(), time.minute(), 0));
}

void TaskGeneralPanelImpl::setEndTime(const QTime &time)
{
    scheduleEndTime->setTime(QTime(time.hour(), time.minute(), 0));
}

QTime TaskGeneralPanelImpl::startTime() const
{
    QTime t = scheduleStartTime->time();
    t.setHMS(t.hour(), t.minute(), 0);
    return t;
}

QTime TaskGeneralPanelImpl::endTime()
{
    QTime t = scheduleEndTime->time();
    t.setHMS(t.hour(), t.minute(), 0);
    return t;
}

QDate TaskGeneralPanelImpl::startDate()
{
    return scheduleStartDate->date();
}


QDate TaskGeneralPanelImpl::endDate()
{
    return scheduleEndDate->date();
}

void TaskGeneralPanelImpl::setStartDateTime(const QDateTime &dt)
{
    setStartDate(dt.date());
    setStartTime(dt.time());
}


void TaskGeneralPanelImpl::setEndDateTime(const QDateTime &dt)
{
    setEndDate(dt.date());
    setEndTime(dt.time());
}

void TaskGeneralPanelImpl::setStartDate(const QDate &date)
{
    scheduleStartDate->setDate(date);
}


void TaskGeneralPanelImpl::setEndDate(const QDate &date)
{
    scheduleEndDate->setDate(date);
}

void TaskGeneralPanelImpl::setRisktype(int r)
{
    risk->setCurrentIndex(r);
}

int TaskGeneralPanelImpl::risktype() const
{
    return risk->currentIndex();
}

Calendar *TaskGeneralPanelImpl::calendar() const
{
    return m_calendars.value(calendarCombo->currentIndex());
}

}  //KPlato namespace
