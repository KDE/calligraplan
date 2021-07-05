/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptmilestoneprogresspanel.h"

#include <QCheckBox>
#include <QDateTime>

#include <KLocalizedString>

#include "kpttask.h"
#include "kptcommand.h"
#include "kptdebug.h"


namespace KPlato
{

MilestoneProgressPanel::MilestoneProgressPanel(Task &task, QWidget *parent, const char *name)
    : MilestoneProgressPanelImpl(parent, name),
      m_task(task),
      m_completion(task.completion())

{
    debugPlan;
    finished->setChecked(m_completion.isFinished());
    finishTime->setDateTime(m_completion.finishTime());
    enableWidgets();
    finished->setFocus();
}


MacroCommand *MilestoneProgressPanel::buildCommand() {
    MacroCommand *cmd = nullptr;
    KUndo2MagicString c = kundo2_i18n("Modify milestone completion");
    
    if (m_completion.isFinished() != finished->isChecked()) {
        if (cmd == nullptr) cmd = new MacroCommand(c);
        cmd->addCommand(new ModifyCompletionStartedCmd(m_completion, finished->isChecked()));
        cmd->addCommand(new ModifyCompletionFinishedCmd(m_completion, finished->isChecked()));
    }
    if (m_completion.finishTime() != finishTime->dateTime()) {
        if (cmd == nullptr) cmd = new MacroCommand(c);
        cmd->addCommand(new ModifyCompletionStartTimeCmd(m_completion, finishTime->dateTime()));
        cmd->addCommand(new ModifyCompletionFinishTimeCmd(m_completion, finishTime->dateTime()));
    }
    if (finished->isChecked() && finishTime->dateTime().isValid()) {
        if (cmd == nullptr) cmd = new MacroCommand(c);
        cmd->addCommand(new ModifyCompletionPercentFinishedCmd(m_completion, finishTime->dateTime().date(), 100));
    } else {
        Completion::EntryList::ConstIterator entriesIt = m_completion.entries().constBegin();
        const Completion::EntryList::ConstIterator entriesEnd = m_completion.entries().constEnd();
        for (; entriesIt != entriesEnd; ++entriesIt) {
            const QDate &date = entriesIt.key();
            if (cmd == nullptr) cmd = new MacroCommand(c);
            cmd->addCommand(new RemoveCompletionEntryCmd(m_completion, date));
        }
    }
    return cmd;
}

//-------------------------------------

MilestoneProgressPanelImpl::MilestoneProgressPanelImpl(QWidget *parent, const char *name)
    : QWidget(parent) {
    
    setObjectName(name);
    setupUi(this);
    
    connect(finished, &QAbstractButton::toggled, this, &MilestoneProgressPanelImpl::slotFinishedChanged);
    connect(finished, &QAbstractButton::toggled, this, &MilestoneProgressPanelImpl::slotChanged);

    connect(finishTime, &QDateTimeEdit::dateTimeChanged, this, &MilestoneProgressPanelImpl::slotChanged);
    
}

void MilestoneProgressPanelImpl::slotChanged() {
    Q_EMIT changed();
}

void MilestoneProgressPanelImpl::slotFinishedChanged(bool state) {
    if (state) {
        finishTime->setDateTime(QDateTime::currentDateTime());
    }
    enableWidgets();
}


void MilestoneProgressPanelImpl::enableWidgets() {
    finished->setEnabled(true);
    finishTime->setEnabled(finished->isChecked());
}


}  //KPlato namespace
