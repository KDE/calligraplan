/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTTASKCOSTPANEL_H
#define KPTTASKCOSTPANEL_H

#include "planui_export.h"

#include "ui_kpttaskcostpanelbase.h"

namespace KPlato
{

class Locale;
class TaskCostPanel;
class Account;
class Accounts;
class Task;
class MacroCommand;

class TaskCostPanelImpl : public QWidget, public Ui_TaskCostPanelBase {
    Q_OBJECT
public:
    explicit TaskCostPanelImpl(QWidget *parent=nullptr, const char *name=nullptr);

Q_SIGNALS:
    void changed();

public Q_SLOTS:
    void slotChanged();
};

class TaskCostPanel : public TaskCostPanelImpl {
    Q_OBJECT
public:
    TaskCostPanel(Task &task, Accounts &accounts, QWidget *parent=nullptr, const char *name=nullptr);
    ~TaskCostPanel() override;

    MacroCommand *buildCommand();

    bool ok();

    void setStartValues(Task &task);

protected:
    void setCurrentItem(QComboBox *box, const QString& name);
    
private:
    Task &m_task;
    Accounts &m_accounts;
    QStringList m_accountList;
    Account *m_oldrunning;
    Account *m_oldstartup;
    Account *m_oldshutdown;
    const Locale *m_locale;
    bool m_localeIsOwn;
};

} //KPlato namespace

#endif // TASKCOSTPANEL_H
