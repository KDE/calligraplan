/* This file is part of the KDE project
   SPDX-FileCopyrightText: 20079 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptinsertfiledlg.h"
#include "kptnode.h"
#include "kptproject.h"

#include <KLocalizedString>
#include <KIO/StatJob>

namespace KPlato
{

InsertFileDialog::InsertFileDialog(Project &project, Node *currentNode, QWidget *parent)
    : KoDialog(parent)
{
    setCaption(i18n("Insert File"));
    setButtons(KoDialog::Ok | KoDialog::Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    
    m_panel = new InsertFilePanel(project, currentNode, this);

    setMainWidget(m_panel);
    
    enableButtonOk(false);

    connect(m_panel, &InsertFilePanel::enableButtonOk, this, &KoDialog::enableButtonOk);
}

QUrl InsertFileDialog::url() const
{
    return m_panel->url();
}

Node *InsertFileDialog::parentNode() const
{
    return m_panel->parentNode();
}

Node *InsertFileDialog::afterNode() const
{
    return m_panel->afterNode();
}

//------------------------
InsertFilePanel::InsertFilePanel(Project &project, Node *currentNode, QWidget *parent)
    : QWidget(parent),
    m_project(project),
    m_node(currentNode)
{
    ui.setupUi(this);
    
    if (currentNode == nullptr || currentNode->type() == Node::Type_Project) {
        ui.ui_isAfter->setEnabled(false);
        ui.ui_isParent->setEnabled(false);
        ui.ui_useProject->setChecked(true);

        ui.ui_name->setText(project.name());
    } else {
        ui.ui_isAfter->setChecked(true);

        ui.ui_name->setText(currentNode->name());
    }
    connect(ui.ui_url, &KUrlRequester::textChanged, this, &InsertFilePanel::changed);

    connect(ui.ui_url, &KUrlRequester::openFileDialog, this, &InsertFilePanel::slotOpenFileDialog);
}

void InsertFilePanel::slotOpenFileDialog(KUrlRequester *)
{
    ui.ui_url->setNameFilter(QStringLiteral("*.plan"));
}

void InsertFilePanel::changed(const QString &text)
{
    KIO::StatJob* statJob = KIO::stat(QUrl::fromUserInput(text));
    statJob->setSide(KIO::StatJob::SourceSide);

    const bool isUrlReadable = statJob->exec();

    Q_EMIT enableButtonOk(isUrlReadable);
}

QUrl InsertFilePanel::url() const
{
    return ui.ui_url->url();
}

Node *InsertFilePanel::parentNode() const
{
    if (ui.ui_useProject->isChecked()) {
        return &(m_project);
    }
    if (ui.ui_isParent->isChecked()) {
        return m_node;
    }
    if (ui.ui_isAfter->isChecked()) {
        return m_node->parentNode();
    }
    return &(m_project);
}

Node *InsertFilePanel::afterNode() const
{
    if (ui.ui_isAfter->isChecked()) {
        return m_node;
    }
    return nullptr;
}


}  //KPlato namespace
