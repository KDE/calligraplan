/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002 Bo Thorsen bo @sonofthor.dk
   SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpttaskdialog.h"
#include "kpttaskcostpanel.h"
#include "kpttaskgeneralpanel.h"
#include "kptrequestresourcespanel.h"
#include "kptdocumentspanel.h"
#include "kpttaskdescriptiondialog.h"
#include "kptcommand.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptproject.h"

#include <KoVBox.h>

#include <KLocalizedString>

namespace KPlato
{

TaskDialog::TaskDialog(Project &project, Task &task, Accounts &accounts, QWidget *p)
    : KPageDialog(p),
    m_project(project),
    m_node(&task)
{
    setWindowTitle(i18n("Task Settings"));
    setFaceType(KPageDialog::Tabbed);

    KoVBox *page;

    // Create all the tabs.
    page =  new KoVBox();
    addPage(page, i18n("&General"));
    m_generalTab = new TaskGeneralPanel(project, task, page);

    page =  new KoVBox();
    addPage(page, i18n("&Resources"));
    m_resourcesTab = new RequestResourcesPanel(page, project, task);

    page =  new KoVBox();
    addPage(page, i18n("&Documents"));
    m_documentsTab = new DocumentsPanel(task, page);

    page =  new KoVBox();
    addPage(page, i18n("&Cost"));
    m_costTab = new TaskCostPanel(task, accounts, page);

    page =  new KoVBox();
    addPage(page, i18n("D&escription"));
    m_descriptionTab = new TaskDescriptionPanel(task, page);
    m_descriptionTab->namefield->hide();
    m_descriptionTab->namelabel->hide();

    setButtonOkEnabled(false);

    connect(this, &KPageDialog::currentPageChanged, this, &TaskDialog::slotCurrentChanged);

    connect(m_generalTab, &TaskGeneralPanelImpl::obligatedFieldsFilled, this, &TaskDialog::setButtonOkEnabled);
    connect(m_resourcesTab, &RequestResourcesPanel::changed, m_generalTab, &TaskGeneralPanelImpl::checkAllFieldsFilled);
    connect(m_documentsTab, &DocumentsPanel::changed, m_generalTab, &TaskGeneralPanelImpl::checkAllFieldsFilled);
    connect(m_costTab, &TaskCostPanelImpl::changed, m_generalTab, &TaskGeneralPanelImpl::checkAllFieldsFilled);
    connect(m_descriptionTab, &TaskDescriptionPanelImpl::textChanged, m_generalTab, &TaskGeneralPanelImpl::checkAllFieldsFilled);

    connect(&project, &Project::nodeRemoved, this, &TaskDialog::slotTaskRemoved);
}

void TaskDialog::setButtonOkEnabled(bool enabled) {
    buttonBox()->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

void TaskDialog::slotCurrentChanged(KPageWidgetItem *current, KPageWidgetItem *prev)
{
    Q_UNUSED(prev)
    //debugPlan<<current->widget()<<m_descriptionTab->parent();
    // HACK: KPageDialog grabs focus when a tab is clicked.
    // KRichTextWidget still flashes the caret so the user thinks it has the focus.
    // For now, just give the KRichTextWidget focus.
    if (current->widget() == m_descriptionTab->parent()) {
        m_descriptionTab->descriptionfield->setFocus();
    }
}

void TaskDialog::slotTaskRemoved(Node *node)
{
    if (node == m_node) {
        reject();
    }
}

MacroCommand *TaskDialog::buildCommand() {
    MacroCommand *m = new MacroCommand(kundo2_i18n("Modify task"));
    bool modified = false;
    MacroCommand *cmd = m_generalTab->buildCommand();
    if (cmd) {
        m->addCommand(cmd);
        modified = true;
    }
    cmd = m_resourcesTab->buildCommand();
    if (cmd) {
        m->addCommand(cmd);
        modified = true;
    }
    cmd = m_documentsTab->buildCommand();
    if (cmd) {
        m->addCommand(cmd);
        modified = true;
    }
    cmd = m_costTab->buildCommand();
    if (cmd) {
        m->addCommand(cmd);
        modified = true;
    }
    cmd = m_descriptionTab->buildCommand();
    if (cmd) {
        m->addCommand(cmd);
        modified = true;
    }
    if (!modified) {
        delete m;
        return nullptr;
    }
    return m;
}

void TaskDialog::accept() {
    if (!m_generalTab->ok())
        return;
    if (!m_resourcesTab->ok())
        return;
    if (!m_descriptionTab->ok())
        return;
    KPageDialog::accept();
}

//---------------------------
TaskAddDialog::TaskAddDialog(Project &project, Task &task, Node *currentNode, Accounts &accounts, QWidget *p)
    : TaskDialog(project, task, accounts, p)
{
    m_currentnode = currentNode;
    // do not know wbs code yet
    m_generalTab->hideWbs();
    
    connect(&project, &Project::nodeRemoved, this, &TaskAddDialog::slotNodeRemoved);
}

TaskAddDialog::~TaskAddDialog()
{
    delete m_node; // in case of cancel
}

void TaskAddDialog::slotNodeRemoved(Node *node)
{
    if (m_currentnode == node) {
        reject();
    }
}

MacroCommand *TaskAddDialog::buildCommand()
{
    MacroCommand *c = new MacroCommand(kundo2_i18n("Add task"));
    c->addCommand(new TaskAddCmd(&m_project, m_node, m_currentnode));
    MacroCommand *m = TaskDialog::buildCommand();
    if (m) {
        c->addCommand(m);
    }
    m_node = nullptr; // don't delete task
    return c;
}

//---------------------------
SubTaskAddDialog::SubTaskAddDialog(Project &project, Task &task, Node *currentNode, Accounts &accounts, QWidget *p)
    : TaskDialog(project, task, accounts, p)
{
    m_currentnode = currentNode;
    // do not know wbs code yet
    m_generalTab->hideWbs();

    connect(&project, &Project::nodeRemoved, this, &SubTaskAddDialog::slotNodeRemoved);
}

SubTaskAddDialog::~SubTaskAddDialog()
{
    delete m_node; // in case of cancel
}

void SubTaskAddDialog::slotNodeRemoved(Node *node)
{
    if (m_currentnode == node) {
        reject();
    }
}

MacroCommand *SubTaskAddDialog::buildCommand()
{
    KUndo2MagicString s = kundo2_i18n("Add sub-task");
    if (m_currentnode == nullptr) {
        s = kundo2_i18n("Add task"); // it will be added to project
    }
    MacroCommand *c = new MacroCommand(s);
    c->addCommand(new SubtaskAddCmd(&m_project, m_node, m_currentnode));
    MacroCommand *m = TaskDialog::buildCommand();
    if (m) {
        c->addCommand(m);
    }
    m_node = nullptr; // don't delete task
    return c;
}

}  //KPlato namespace
