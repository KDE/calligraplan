/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ConfigDialog.h"

#include <portfoliosettings.h>
#include "Part.h"

#include <Help.h>
#include <KoIcon.h>
#include <config/KoConfigDocumentPage.h>
#include <config/ConfigDocumentationPanel.h>
#include <Help.h>

#include <KConfigSkeleton>
#include <KLocalizedString>

#include <QDebug>


ConfigDialog::ConfigDialog(Part *part, const QString& name, KConfigSkeleton *config)
    : KConfigDialog(part->currentMainwindow(), name, config)
{
    auto docPage = new KoConfigDocumentPage(part);
    m_pages << addPage(docPage, i18nc("@title:tab Document settings page", "Document"));
    m_pages.last()->setIcon(koIcon("document-properties"));

//     m_pages << addPage(new KPlato::ConfigDocumentationPanel(), i18n("Documentation"), koIconName("documents"));
}

void ConfigDialog::updateSettings()
{
    Q_EMIT updateWidgetsSettings();
    PortfolioSettings::self()->save();
    Q_EMIT settingsUpdated();
}

void ConfigDialog::updateWidgets()
{
    Q_EMIT updateWidgetsData();
}

bool ConfigDialog::hasChanged()
{
    QWidget *w = currentPage()->widget()->findChild<QWidget*>(QStringLiteral("ConfigWidget"));
    return w ? w->property("hasChanged").toBool() : false;
}

void ConfigDialog::showHelp()
{
    KPlato::Help::instance()->invokeContext(QUrl(QStringLiteral("portfolio:configure-portfolio-dialog")));
}

