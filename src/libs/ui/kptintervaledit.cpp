/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptintervaledit.h"
#include "intervalitem.h"
#include "kptcommand.h"
#include "kptproject.h"

#include <KoIcon.h>

#include <KLocalizedString>

#include <QTreeWidget>
#include <QList>


namespace KPlato
{

IntervalEdit::IntervalEdit(CalendarDay *day, QWidget *parent)
    : IntervalEditImpl(parent)
{
    //debugPlan;
    if (day) {
        const QList<TimeInterval*> &intervals = day->timeIntervals();
        setIntervals(intervals);
        if (! intervals.isEmpty()) {
            startTime->setTime(intervals.last()->endTime());
            qreal l = (intervals.last()->endTime().msecsTo(QTime().addMSecs(-1)) + 1)  / (1000.0*60.0*60.0);
            length->setValue(qMin(l, (qreal) 8.0));
        }
    }
    enableButtons();
    startTime->setFocus();
}


//--------------------------------------------
IntervalEditImpl::IntervalEditImpl(QWidget *parent)
    : IntervalEditBase(parent) 
{
    intervalList->setColumnCount(2);
    QStringList lst;
    lst << i18nc("Interval start time", "Start")
        << i18nc("Interval length", "Length");
    intervalList->setHeaderLabels(lst);

    intervalList->setRootIsDecorated(false);
    intervalList->setSortingEnabled(true);
    intervalList->sortByColumn(0, Qt::AscendingOrder);

    bAddInterval->setIcon(koIcon("list-add"));
    bRemoveInterval->setIcon(koIcon("list-remove"));
    bClear->setIcon(koIcon("edit-clear-list"));

    connect(bClear, &QAbstractButton::clicked, this, &IntervalEditImpl::slotClearClicked);
    connect(bAddInterval, &QAbstractButton::clicked, this, &IntervalEditImpl::slotAddIntervalClicked);
    connect(bRemoveInterval, &QAbstractButton::clicked, this, &IntervalEditImpl::slotRemoveIntervalClicked);
    connect(intervalList, &QTreeWidget::itemSelectionChanged, this, &IntervalEditImpl::slotIntervalSelectionChanged);
    
    connect(startTime, &QDateTimeEdit::timeChanged, this, &IntervalEditImpl::enableButtons);
    connect(length, SIGNAL(valueChanged(double)), SLOT(enableButtons()));
    
}

void IntervalEditImpl::slotClearClicked() {
    bool c = intervalList->topLevelItemCount() > 0;
    intervalList->clear();
    enableButtons();
    if (c)
        Q_EMIT changed();
}

void IntervalEditImpl::slotAddIntervalClicked() {
    new IntervalItem(intervalList, startTime->time(), (int)(length->value() * 1000. * 60. *60.));
    enableButtons();
    Q_EMIT changed();
}

void IntervalEditImpl::slotRemoveIntervalClicked() {
    IntervalItem *item = static_cast<IntervalItem*>(intervalList->currentItem());
    if (item == nullptr) {
        return;
    }
    intervalList->takeTopLevelItem(intervalList->indexOfTopLevelItem(item));
    delete item;
    enableButtons();
    Q_EMIT changed();
}


void IntervalEditImpl::slotIntervalSelectionChanged() {
    QList<QTreeWidgetItem*> lst = intervalList->selectedItems();
    if (lst.count() == 0)
        return;
    
    IntervalItem *ii = static_cast<IntervalItem *>(lst[0]);
    startTime->setTime(ii->interval().first);
    length->setValue((double)(ii->interval().second) / (1000.*60.*60.));
    
    enableButtons();
}

QList<TimeInterval*> IntervalEditImpl::intervals() const {
    QList<TimeInterval*> l;
    int cnt = intervalList->topLevelItemCount();
    for (int i=0; i < cnt; ++i) {
        IntervalItem *item = static_cast<IntervalItem*>(intervalList->topLevelItem(i));
        l.append(new TimeInterval(item->interval().first, item->interval().second));
    }
    return l;
}

void IntervalEditImpl::setIntervals(const QList<TimeInterval*> &intervals) {
    intervalList->clear();
    for (TimeInterval *i : intervals) {
        new IntervalItem(intervalList, i->first, i->second);
    }
    enableButtons();
}

void IntervalEditImpl::enableButtons() {
    bClear->setEnabled(! intervals().isEmpty());
    
    bRemoveInterval->setEnabled(intervalList->currentItem());
    
    if (length->value() == 0.0) {
        bAddInterval->setEnabled(false);
        return;
    }
    if (QTime(0, 0, 0).secsTo(startTime->time()) + (int)(length->value() * 60. * 60.) > 24 * 60 * 60) {
        bAddInterval->setEnabled(false);
        return;
    }
    TimeInterval ti(startTime->time(),  (int)(length->value() * 1000. * 60. *60.));
    const QList<TimeInterval*> intervals = this->intervals();
    for (TimeInterval *i : intervals) {
        if (i->intersects(ti)) {
            bAddInterval->setEnabled(false);
            return;
        }
    }
    bAddInterval->setEnabled(true);
}

//-------------------------------------------------------------
IntervalEditDialog::IntervalEditDialog(Calendar *calendar, const QList<CalendarDay*> &days, QWidget *parent)
    : KoDialog(parent),
    m_calendar(calendar),
    m_days(days)
{
    //debugPlan;
    setCaption(i18n("Edit Work Intervals"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    //debugPlan<<&p;
    m_panel = new IntervalEdit(days.value(0), this);
    setMainWidget(m_panel);
    enableButtonOk(false);

    connect(m_panel, &IntervalEditImpl::changed, this, &IntervalEditDialog::slotChanged);
    connect(calendar->project(), &Project::calendarRemoved, this, &IntervalEditDialog::slotCalendarRemoved);
}

IntervalEditDialog::IntervalEditDialog(Calendar *calendar, const QList<QDate> &dates, QWidget *parent)
    : KoDialog(parent),
    m_calendar(calendar),
    m_dates(dates)
{
    //debugPlan;
    setCaption(i18n("Edit Work Intervals"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    //debugPlan<<&p;
    for (const QDate &d : dates) {
        CalendarDay *day = calendar->findDay(d);
        if (day) {
            m_days << day;
        }
    }
    m_panel = new IntervalEdit(m_days.value(0), this);
    setMainWidget(m_panel);
    enableButtonOk(false);

    connect(m_panel, &IntervalEditImpl::changed, this, &IntervalEditDialog::slotChanged);
    connect(calendar->project(), &Project::calendarRemoved, this, &IntervalEditDialog::slotCalendarRemoved);
}

void IntervalEditDialog::slotCalendarRemoved(const Calendar *cal)
{
    if (m_calendar == cal) {
        reject();
    }
}

void IntervalEditDialog::slotChanged()
{
    enableButtonOk(true);
}

MacroCommand *IntervalEditDialog::buildCommand()
{
    MacroCommand *cmd = new MacroCommand(kundo2_i18n("Modify Work Interval"));
    for (const QDate &d : std::as_const(m_dates)) {
        // these are dates, weekdays don't have date
        CalendarDay *day = m_calendar->findDay(d);
        if (day == nullptr) {
            // create a new day
            day = new CalendarDay(d);
            cmd->addCommand(new CalendarAddDayCmd(m_calendar, day));
        }
        MacroCommand *c = buildCommand(m_calendar, day);
        if (c) {
            cmd->addCommand(c);
        }
    }
    if (m_dates.isEmpty()) {
        // weekdays
        for (CalendarDay *day : std::as_const(m_days)) {
            MacroCommand *c = buildCommand(m_calendar, day);
            if (c) {
                cmd->addCommand(c);
            }
        }
    }
    if (cmd->isEmpty()) {
        delete cmd;
        return nullptr;
    }
    return cmd;
}

MacroCommand *IntervalEditDialog::buildCommand(Calendar *calendar, CalendarDay *day)
{
    //debugPlan;
    const QList<TimeInterval*> lst = m_panel->intervals();
    if (lst == day->timeIntervals()) {
        return nullptr;
    }
    MacroCommand *cmd = nullptr;
    // Set to Undefined. This will also clear any intervals
    CalendarModifyStateCmd *c = new CalendarModifyStateCmd(calendar, day, CalendarDay::Undefined);
    if (cmd == nullptr) cmd = new MacroCommand(KUndo2MagicString());
    cmd->addCommand(c);
    //debugPlan<<"Set Undefined";

    for (TimeInterval *i : lst) {
        CalendarAddTimeIntervalCmd *c = new CalendarAddTimeIntervalCmd(calendar, day, i);
        if (cmd == nullptr) cmd = new MacroCommand(KUndo2MagicString());
        cmd->addCommand(c);
    }
    if (! lst.isEmpty()) {
        CalendarModifyStateCmd *c = new CalendarModifyStateCmd(calendar, day, CalendarDay::Working);
        if (cmd == nullptr) cmd = new MacroCommand(KUndo2MagicString());
        cmd->addCommand(c);
    }
    if (cmd) {
        cmd->setText(kundo2_i18n("Modify Work Interval"));
    }
    return cmd;
}

}  //KPlato namespace
