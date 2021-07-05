/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptstandardworktimedialog.h"
#include "kptduration.h"
#include "kptproject.h"
#include "kptcalendar.h"
#include "kptcommand.h"
#include "kptdebug.h"

#include <QTreeWidgetItem>
#include <QLocale>


namespace KPlato
{

class WeekdayListItem : public QTreeWidgetItem
{
public:
    WeekdayListItem(Calendar *cal, int wd, QTreeWidget *parent, const QString& name, QTreeWidgetItem *after)
    : QTreeWidgetItem(parent, after),
      original(cal->weekday(wd)),
      calendar(cal),
      weekday(wd)
    {
        setText(0, name);
        day = new CalendarDay(original);
        if (day->state() == CalendarDay::NonWorking) {
            setHours();
        } else {
            setText(1, QLocale().toString(day->duration().toDouble(Duration::Unit_h), 'f', 2));
        }
    }
    ~WeekdayListItem() override {
        delete day;
    }
    void setHours() {
        setText(1, "-");
        day->clearIntervals();
    }
    void setIntervals(QList<TimeInterval*> intervals) {
        day->setIntervals(intervals);
        setText(1, QLocale().toString(day->duration().toDouble(Duration::Unit_h), 'f', 2));
    }
    void setState(int st) {
        day->setState(st+1);
    }
    
    MacroCommand *save() {
        MacroCommand *cmd=nullptr;
        if (*original != *day) {
            cmd = new MacroCommand();
            cmd->addCommand(new CalendarModifyWeekdayCmd(calendar, weekday, day));
            day = nullptr;
        }
        return cmd;
    }
    CalendarDay *day;
    CalendarDay *original;
    Calendar *calendar;
    int weekday;
};

StandardWorktimeDialog::StandardWorktimeDialog(Project &p, QWidget *parent)
    : KoDialog(parent),
      project(p)
{
    setCaption(i18n("Estimate Conversions"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    //debugPlan<<&p;
    m_original = p.standardWorktime();
    dia = new StandardWorktimeDialogImpl(m_original, this);

    setMainWidget(dia);
    enableButtonOk(false);

    connect(dia, &StandardWorktimeDialogImpl::obligatedFieldsFilled, this, &KoDialog::enableButtonOk);
    connect(dia, &StandardWorktimeDialogImpl::enableButtonOk, this, &KoDialog::enableButtonOk);
    connect(this,&KoDialog::okClicked,this,&StandardWorktimeDialog::slotOk);
}

MacroCommand *StandardWorktimeDialog::buildCommand() {
    //debugPlan;
    KUndo2MagicString n = kundo2_i18n("Modify Estimate Conversions");
    MacroCommand *cmd = nullptr;
    if (m_original->year() != dia->inYear()) {
        if (cmd == nullptr) cmd = new MacroCommand(n);
        cmd->addCommand(new ModifyStandardWorktimeYearCmd(m_original, m_original->year(), dia->inYear()));
    }
    if (m_original->month() != dia->inMonth()) {
        if (cmd == nullptr) cmd = new MacroCommand(n);
        cmd->addCommand(new ModifyStandardWorktimeMonthCmd(m_original, m_original->month(), dia->inMonth()));
    }
    if (m_original->week() != dia->inWeek()) {
        if (cmd == nullptr) cmd = new MacroCommand(n);
        cmd->addCommand(new ModifyStandardWorktimeWeekCmd(m_original, m_original->week(), dia->inWeek()));
    }
    if (m_original->day() != dia->inDay()) {
        if (cmd == nullptr) cmd = new MacroCommand(n);
        cmd->addCommand(new ModifyStandardWorktimeDayCmd(m_original, m_original->day(), dia->inDay()));
    }
    return cmd;

}

void StandardWorktimeDialog::slotOk() {
    accept();
}


StandardWorktimeDialogImpl::StandardWorktimeDialogImpl(StandardWorktime *std, QWidget *parent)
    : QWidget(parent),
      m_std(std) {
    
    setupUi(this);
    if (!std) {
        m_std = new StandardWorktime();
    }
    m_year = m_std->year();
    m_month = m_std->month();
    m_week = m_std->week();
    m_day = m_std->day();

    debugPlan<<"y="<<m_year<<" m="<<m_month<<" w="<<m_week<<" d="<<m_day;
    year->setValue(m_year);
    month->setValue(m_month);
    week->setValue(m_week);
    day->setValue(m_day);
    
    connect(year, SIGNAL(valueChanged(double)), SLOT(slotYearChanged(double)));
    connect(month, SIGNAL(valueChanged(double)), SLOT(slotMonthChanged(double)));
    connect(week, SIGNAL(valueChanged(double)), SLOT(slotWeekChanged(double)));
    connect(day, SIGNAL(valueChanged(double)), SLOT(slotDayChanged(double)));

}


void StandardWorktimeDialogImpl::slotEnableButtonOk(bool on) {
    Q_EMIT enableButtonOk(on);
}

void StandardWorktimeDialogImpl::slotCheckAllFieldsFilled() {
    Q_EMIT obligatedFieldsFilled(true);
}

void StandardWorktimeDialogImpl::slotYearChanged(double value) {
    //debugPlan<<value;
    m_year = value;
    if (month->value() > value)
        month->setValue(value);
    slotEnableButtonOk(true);
}

void StandardWorktimeDialogImpl::slotMonthChanged(double value) {
    m_month = value;
    if (year->value() < value)
        year->setValue(value);
    if (week->value() > value)
        week->setValue(value);
    slotEnableButtonOk(true);
}

void StandardWorktimeDialogImpl::slotWeekChanged(double value) {
    m_week = value;
    if (month->value() < value)
        month->setValue(value);
    if (day->value() > value)
        day->setValue(value);
    slotEnableButtonOk(true);
}

void StandardWorktimeDialogImpl::slotDayChanged(double value) {
    m_day = value;
    if (week->value() < value)
        week->setValue(value);
    slotEnableButtonOk(true);
}


}  //KPlato namespace
