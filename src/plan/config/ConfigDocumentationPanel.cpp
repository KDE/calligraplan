/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ConfigDocumentationPanel.h"

#include "calligraplansettings.h"

#include <QDebug>

using namespace KPlato;

ConfigDocumentationPanel::ConfigDocumentationPanel(QWidget *parent)
    : ConfigDocumentationPanelImpl(parent)
{
    connect(kcfg_DocumentationPath, &QLineEdit::editingFinished, this, &ConfigDocumentationPanel::slotPathChanged);
    // disable language, probably unusable
    languageLabel->hide();
    kcfg_ContextLanguage->hide();
}

void ConfigDocumentationPanel::slotPathChanged()
{
}

//-----------------------------
ConfigDocumentationPanelImpl::ConfigDocumentationPanelImpl(QWidget *p)
    : QWidget(p)
{
    setupUi(this);
}

