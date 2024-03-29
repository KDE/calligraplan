/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <calligra-devel@kde.org

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpttaskdescriptiondialog.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptcommand.h"
#include "RichTextWidget.h"

#include <KLocalizedString>
#include <KTextEdit>
#include <KActionCollection>

namespace KPlato
{

TaskDescriptionPanel::TaskDescriptionPanel(Node &node, QWidget *p, bool readOnly)
    : TaskDescriptionPanelImpl(node, p)
{
    initDescription(readOnly);
    setStartValues(node);

    descriptionfield->setFocus();
}

void TaskDescriptionPanel::setStartValues(Node &node)
{
    namefield->setText(node.name());
    descriptionfield->setTextOrHtml(node.description());
}

MacroCommand *TaskDescriptionPanel::buildCommand()
{
    KUndo2MagicString s = kundo2_i18n("Modify task description");
    if (m_node.type() == Node::Type_Milestone) {
        s = kundo2_i18n("Modify milestone description");
    } else if (m_node.type() == Node::Type_Summarytask) {
        s = kundo2_i18n("Modify summary task description");
    } else if (m_node.type() == Node::Type_Project) {
        s = kundo2_i18n("Modify project description");
    }
    MacroCommand *cmd = new MacroCommand(s);
    bool modified = false;

    if (m_node.description() != descriptionfield->textOrHtml()) {
        cmd->addCommand(new NodeModifyDescriptionCmd(m_node, descriptionfield->textOrHtml()));
        modified = true;
    }
    if (!modified) {
        delete cmd;
        return nullptr;
    }
    return cmd;
}

bool TaskDescriptionPanel::ok() {
    return true;
}

void TaskDescriptionPanel::initDescription(bool readOnly)
{
    toolbar->setVisible(! readOnly);
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    KActionCollection *collection = new KActionCollection(this); //krazy:exclude=tipsandthis

    collection->addActions(descriptionfield->createActions());

    toolbar->addAction(collection->action(QStringLiteral("format_text_bold")));
    toolbar->addAction(collection->action(QStringLiteral("format_text_italic")));
    toolbar->addAction(collection->action(QStringLiteral("format_text_underline")));
    toolbar->addAction(collection->action(QStringLiteral("format_text_strikeout")));
    toolbar->addSeparator();

    toolbar->addAction(collection->action(QStringLiteral("format_list_style")));
    toolbar->addSeparator();

    toolbar->addAction(collection->action(QStringLiteral("format_align_left")));
    toolbar->addAction(collection->action(QStringLiteral("format_align_center")));
    toolbar->addAction(collection->action(QStringLiteral("format_align_right")));
    toolbar->addAction(collection->action(QStringLiteral("format_align_justify")));
    toolbar->addSeparator();
    toolbar->addAction(collection->action(QStringLiteral("manage_link")));
    toolbar->addAction(collection->action(QStringLiteral("open_link")));

    descriptionfield->append(QLatin1String(""));
    descriptionfield->setReadOnly(readOnly);
    descriptionfield->setOverwriteMode(false);
    descriptionfield->setLineWrapMode(KTextEdit::WidgetWidth);
    descriptionfield->setTabChangesFocus(true);
}

//-----------------------------
TaskDescriptionPanelImpl::TaskDescriptionPanelImpl(Node &node, QWidget *p)
    : QWidget(p),
      m_node(node)
{

    setupUi(this);

    CONNECT(descriptionfield, &QTextEdit::textChanged, this, &TaskDescriptionPanelImpl::slotChanged);
}

TaskDescriptionPanelImpl::~TaskDescriptionPanelImpl()
{
    DISCONNECT;
}

void TaskDescriptionPanelImpl::slotChanged()
{
    Q_EMIT textChanged(descriptionfield->textOrHtml() != m_node.description());
}

//-----------------------------
TaskDescriptionDialog::TaskDescriptionDialog(Node &node, QWidget *p, bool readOnly)
    : KoDialog(p)
{
    setCaption(node.type() == Node::Type_Project ? i18n("Project Description")  : i18n("Task Description"));
    if (readOnly) {
        setButtons(Close);
    } else {
        setButtons(Ok|Cancel);
        setDefaultButton(Ok);
    }
    showButtonSeparator(true);

    m_descriptionTab = new TaskDescriptionPanel(node, this, readOnly);
    setMainWidget(m_descriptionTab);

    enableButtonOk(false);

    connect(m_descriptionTab, &TaskDescriptionPanelImpl::textChanged, this, &KoDialog::enableButtonOk);
}

MacroCommand *TaskDescriptionDialog::buildCommand()
{
    return m_descriptionTab->buildCommand();
}

void TaskDescriptionDialog::slotButtonClicked(int button)
{
    if (button == KoDialog::Ok) {
        if (! m_descriptionTab->ok()) {
            return;
        }
        accept();
    } else {
        KoDialog::slotButtonClicked(button);
    }
}


}  //KPlato namespace
