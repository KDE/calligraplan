/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTINTERVALEDIT_H
#define KPTINTERVALEDIT_H

#include "planui_export.h"

#include "ui_kptintervaleditbase.h"
#include "kptcalendar.h"

#include <KoDialog.h>

namespace KPlato
{

class MacroCommand;

class IntervalEditBase : public QWidget, public Ui::IntervalEditBase
{
public:
    explicit IntervalEditBase(QWidget *parent) : QWidget(parent) {
    setupUi(this);
  }
};


class IntervalEditImpl : public IntervalEditBase {
    Q_OBJECT
public:
    explicit IntervalEditImpl(QWidget *parent);

    QList<TimeInterval*> intervals() const;
    void setIntervals(const QList<TimeInterval*> &intervals);

protected Q_SLOTS:
    void slotClearClicked();
    void slotAddIntervalClicked();
    void slotRemoveIntervalClicked();
    void slotIntervalSelectionChanged();
    void enableButtons();

Q_SIGNALS:
    void changed();
};

class IntervalEdit : public IntervalEditImpl {
    Q_OBJECT
public:
    explicit IntervalEdit(CalendarDay *day, QWidget *parent=nullptr);

};

class PLANUI_EXPORT IntervalEditDialog : public KoDialog
{
    Q_OBJECT
public:
    explicit IntervalEditDialog(Calendar *calendar, const QList<CalendarDay*> &days, QWidget *parent = nullptr);
    
    explicit IntervalEditDialog(Calendar *calendar, const QList<QDate> &dates, QWidget *parent = nullptr);

    MacroCommand *buildCommand();
    QList<TimeInterval*> intervals() const { return m_panel->intervals(); }
    
protected Q_SLOTS:
    void slotChanged();
    void slotCalendarRemoved(const KPlato::Calendar *cal);

protected:
    MacroCommand *buildCommand(Calendar *calendar, CalendarDay *day);

private:
    Calendar *m_calendar;
    QList<CalendarDay*> m_days;
    QList<QDate> m_dates;
    IntervalEdit *m_panel;
};

}  //KPlato namespace

#endif // INTERVALEDIT_H
