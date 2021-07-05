/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTSTANDARDWORKTIMEDIALOG_H
#define KPTSTANDARDWORKTIMEDIALOG_H

#include "planui_export.h"

#include "ui_standardworktimedialogbase.h"

#include "kptcalendar.h"

#include <KoDialog.h>


namespace KPlato
{

class Project;
class MacroCommand;

class StandardWorktimeDialogImpl : public QWidget, public Ui::StandardWorktimeDialogBase {
    Q_OBJECT
public:
    StandardWorktimeDialogImpl (StandardWorktime *std, QWidget *parent);

    StandardWorktime *standardWorktime() { return m_std; }
    double inYear() const { return m_year; }
    double inMonth() const { return m_month; }
    double inWeek() const { return m_week; }
    double inDay() const { return m_day; }
    
private Q_SLOTS:
    void slotCheckAllFieldsFilled();
    void slotEnableButtonOk(bool on);

    void slotYearChanged(double);
    void slotMonthChanged(double);
    void slotWeekChanged(double);
    void slotDayChanged(double);

Q_SIGNALS:
    void obligatedFieldsFilled(bool yes);
    void enableButtonOk(bool);

private:
    StandardWorktime *m_std;
    double m_year;
    double m_month;
    double m_week;
    double m_day;
};

class PLANUI_EXPORT StandardWorktimeDialog : public KoDialog {
    Q_OBJECT
public:
    explicit StandardWorktimeDialog(Project &project, QWidget *parent=nullptr);
    
    MacroCommand *buildCommand();

protected Q_SLOTS:
    void slotOk();
    
private:
    Project &project;
    StandardWorktimeDialogImpl *dia;
    StandardWorktime *m_original;
};

} //KPlato namespace

#endif // STANDARDWORKTIMEDIALOG_H
