/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 20011 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpttaskcostpanel.h"

#include "kptaccount.h"
#include "kpttask.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kptlocale.h"


namespace KPlato
{

TaskCostPanel::TaskCostPanel(Task &task, Accounts &accounts, QWidget *p)
    : TaskCostPanelImpl(p),
      m_task(task),
      m_accounts(accounts)
{
    const Project *project = qobject_cast<const Project*>(task.projectNode());
    if (project) {
        m_locale = project->locale();
        m_localeIsOwn = false;
    } else {
        m_locale = new Locale();
        m_localeIsOwn = true;
    }

    m_accountList << i18n("None");
    m_accountList += accounts.costElements();

    if (task.isBaselined(BASELINESCHEDULE)) {
        runningGroup->setEnabled(false);
        startupGroup->setEnabled(false);
        shutdownGroup->setEnabled(false);
    }
    setStartValues(task);
}

TaskCostPanel::~TaskCostPanel()
{
    if (m_localeIsOwn) {
        delete m_locale;
    }
}


void TaskCostPanel::setStartValues(Task &task) {
    runningAccount->addItems(m_accountList);
    m_oldrunning = m_accounts.findRunningAccount(task);
    if (m_oldrunning) {
        setCurrentItem(runningAccount, m_oldrunning->name());
    }
    
    startupCost->setText(m_locale->formatMoney(task.startupCost()));
    startupAccount->addItems(m_accountList);
    m_oldstartup = m_accounts.findStartupAccount(task);
    if (m_oldstartup) {
        setCurrentItem(startupAccount, m_oldstartup->name());
    }
    
    shutdownCost->setText(m_locale->formatMoney(task.shutdownCost()));
    shutdownAccount->addItems(m_accountList);
    m_oldshutdown = m_accounts.findShutdownAccount(task);
    if (m_oldshutdown) {
        setCurrentItem(shutdownAccount, m_oldshutdown->name());
    }
}

void TaskCostPanel::setCurrentItem(QComboBox *box, const QString& name) {
    box->setCurrentIndex(0);
    for (int i = 0; i < box->count(); ++i) {
        if (name == box->itemText(i)) {
            box->setCurrentIndex(i);
            break;
        }
    }
}

MacroCommand *TaskCostPanel::buildCommand() {
    MacroCommand *cmd = new MacroCommand(kundo2_i18n("Modify Task Cost"));
    bool modified = false;
    
    if ((m_oldrunning == nullptr && runningAccount->currentIndex() != 0) ||
        (m_oldrunning && m_oldrunning->name() != runningAccount->currentText())) {
        cmd->addCommand(new NodeModifyRunningAccountCmd(m_task, m_oldrunning, m_accounts.findAccount(runningAccount->currentText())));
        modified = true;
    }
    if ((m_oldstartup == nullptr && startupAccount->currentIndex() != 0) ||
        (m_oldstartup && m_oldstartup->name() != startupAccount->currentText())) {
        cmd->addCommand(new NodeModifyStartupAccountCmd(m_task, m_oldstartup,  m_accounts.findAccount(startupAccount->currentText())));
        modified = true;
    }
    if ((m_oldshutdown == nullptr && shutdownAccount->currentIndex() != 0) ||
        (m_oldshutdown && m_oldshutdown->name() != shutdownAccount->currentText())) {
        cmd->addCommand(new NodeModifyShutdownAccountCmd(m_task, m_oldshutdown,  m_accounts.findAccount(shutdownAccount->currentText())));
        modified = true;
    }
    double money = m_locale->readMoney(startupCost->text());
    if (money != m_task.startupCost()) {
        cmd->addCommand(new NodeModifyStartupCostCmd(m_task, money));
        modified = true;
    }
    money = m_locale->readMoney(shutdownCost->text());
    if (money != m_task.shutdownCost()) {
        cmd->addCommand(new NodeModifyShutdownCostCmd(m_task, money));
        modified = true;
    }
    if (!modified) {
        delete cmd;
        return nullptr;
    }
    return cmd;
}

bool TaskCostPanel::ok() {
    if (runningAccount->currentIndex() == 0 ||
        m_accounts.findAccount(runningAccount->currentText()) == nullptr) {
        //message
        return false;
    }
    if (startupAccount->currentIndex() == 0 ||
        m_accounts.findAccount(startupAccount->currentText()) == nullptr) {
        //message
        return false;
    }
    if (shutdownAccount->currentIndex() == 0 ||
        m_accounts.findAccount(shutdownAccount->currentText()) == nullptr) {
        //message
        return false;
    }
    return true;
}


TaskCostPanelImpl::TaskCostPanelImpl(QWidget *p)
    : QWidget(p)
{
    setupUi(this);
    
    connect(runningAccount, SIGNAL(activated(int)), SLOT(slotChanged()));
    connect(startupAccount, SIGNAL(activated(int)), SLOT(slotChanged()));
    connect(shutdownAccount, SIGNAL(activated(int)), SLOT(slotChanged()));
    connect(startupCost, &QLineEdit::textChanged, this, &TaskCostPanelImpl::slotChanged);
    connect(shutdownCost, &QLineEdit::textChanged, this, &TaskCostPanelImpl::slotChanged);
}

void TaskCostPanelImpl::slotChanged() {
    Q_EMIT changed();
}

}  //KPlato namespace
