/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2007, 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptsummarytaskgeneralpanel.h"
#include "kptsummarytaskdialog.h"
#include "kpttask.h"
#include "kptcommand.h"
#include "kpttaskdescriptiondialog.h"

#include <KLocalizedString>

#ifdef PLAN_KDEPIMLIBS_FOUND
#include <akonadi/contact/emailaddressselectiondialog.h>
#include <akonadi/contact/emailaddressselectionwidget.h>
#include <akonadi/contact/emailaddressselection.h>
#endif

namespace KPlato
{

SummaryTaskGeneralPanel::SummaryTaskGeneralPanel(Task &task, QWidget *p)
    : QWidget(p),
      m_task(task)
{
    setupUi(this);

#ifndef PLAN_KDEPIMLIBS_FOUND
    chooseLeader->hide();
#endif

    // FIXME
    // [Bug 311940] New: Plan crashes when typing a text in the filter textbox before the textbook is fully loaded when selecting a contact from the addressbook
    chooseLeader->hide();

    m_description = new TaskDescriptionPanel(task, this);
    m_description->namefield->hide();
    m_description->namelabel->hide();
    layout()->addWidget(m_description);

    QString s = i18n("The Work Breakdown Structure introduces numbering for all tasks in the project, according to the task structure.\nThe WBS code is auto-generated.\nYou can define the WBS code pattern using the Define WBS Pattern command in the Tools menu.");
    wbslabel->setWhatsThis(s);
    wbsfield->setWhatsThis(s);

    setStartValues(task);
    
    connect(namefield, &QLineEdit::textChanged, this, &SummaryTaskGeneralPanel::slotObligatedFieldsFilled);
    connect(ui_priority, SIGNAL(valueChanged(int)), this, SLOT(slotObligatedFieldsFilled()));
    connect(leaderfield, &QLineEdit::textChanged, this, &SummaryTaskGeneralPanel::slotObligatedFieldsFilled);
    connect(m_description, &TaskDescriptionPanelImpl::textChanged, this, &SummaryTaskGeneralPanel::slotObligatedFieldsFilled);
    
    connect(chooseLeader, &QAbstractButton::clicked, this, &SummaryTaskGeneralPanel::slotChooseResponsible);

}

void SummaryTaskGeneralPanel::setStartValues(Task &task) {
    namefield->setText(task.name());
    leaderfield->setText(task.leader());
    ui_priority->setValue(task.priority());

    m_description->descriptionfield->setTextOrHtml(task.description());
    wbsfield->setText(task.wbsCode());
    
    namefield->setFocus();
    
}

void SummaryTaskGeneralPanel::slotObligatedFieldsFilled() {
    Q_EMIT obligatedFieldsFilled(true); // never block save
}

MacroCommand *SummaryTaskGeneralPanel::buildCommand() {
    MacroCommand *cmd = new MacroCommand(kundo2_i18n("Modify task"));
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
/*    if (!descriptionfield->isHidden() && 
        m_task.description() != descriptionfield->text()) {
        cmd->addCommand(new NodeModifyDescriptionCmd(m_task, descriptionfield->text()));
        modified = true;
    }*/
    MacroCommand *m = m_description->buildCommand();
    if (m) {
        cmd->addCommand(m);
        modified = true;
    }
    if (!modified) {
        delete cmd;
        return nullptr;
    }
    return cmd;
}

bool SummaryTaskGeneralPanel::ok() {
    return true;
}

void SummaryTaskGeneralPanel::slotChooseResponsible()
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


}  //KPlato namespace
