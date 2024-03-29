/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ConfigDialog.h"

#include "kptpart.h"
#include "ConfigProjectPanel.h"
#include "ConfigWorkVacationPanel.h"
#include "kpttaskdefaultpanel.h"
#include "kptworkpackageconfigpanel.h"
#include "kptcolorsconfigpanel.h"
#include "ConfigTaskModulesPanel.h"
#include "ConfigProjectTemplatesPanel.h"
#include "ConfigReportTemplatesPanel.h"

#include <calligraplansettings.h>
#include <KoIcon.h>
#include <KoMainWindow.h>
#include <config/KoConfigDocumentPage.h>
// #include <config/ConfigDocumentationPanel.h>
#include <Help.h>

#include <KConfigSkeleton>
#include <KLocalizedString>

#include <QStandardItem>

using namespace KPlato;

ConfigDialog::ConfigDialog(Part *part, const QString& name, KConfigSkeleton *config)
    : KConfigDialog(part->currentMainwindow(), name, config)
{
    m_pages << addPage(new ConfigProjectPanel(), i18n("Project Defaults"), koIconName("calligraplan"));
    m_pages << addPage(new ConfigWorkVacationPanel(), i18n("Work & Vacation"), koIconName("view-calendar"));
    m_pages << addPage(new TaskDefaultPanel(), i18n("Task Defaults"), koIconName("view-task"));
    m_pages << addPage(new ColorsConfigPanel(), i18n("Task Colors"), koIconName("fill-color"));
    ConfigTaskModulesPanel *page = new ConfigTaskModulesPanel();
    m_pages << addPage(page, i18n("Task Modules"), koIconName("calligraplanwork"));
    connect(page, &ConfigTaskModulesPanel::settingsChanged, this, &ConfigDialog::updateButtons);
    connect(this, &ConfigDialog::updateWidgetsSettings, page, &ConfigTaskModulesPanel::updateSettings);
    connect(this, &ConfigDialog::updateWidgetsData, page, &ConfigTaskModulesPanel::updateWidgets);
    m_pages << addPage(new WorkPackageConfigPanel(), i18n("Work Package"), koIconName("calligraplanwork"));

    ConfigProjectTemplatesPanel *p = new ConfigProjectTemplatesPanel();
    m_pages << addPage(p, i18n("Project Templates"), koIconName("calligraplan"));
    connect(p, &ConfigProjectTemplatesPanel::settingsChanged, this, &ConfigDialog::updateButtons);
    connect(this, &ConfigDialog::updateWidgetsSettings, p, &ConfigProjectTemplatesPanel::updateSettings);
    connect(this, &ConfigDialog::updateWidgetsData, p, &ConfigProjectTemplatesPanel::updateWidgets);

    ConfigReportTemplatesPanel *r = new ConfigReportTemplatesPanel();
    m_pages << addPage(r, i18n("Report Templates"), koIconName("calligraplan"));
    connect(r, &ConfigReportTemplatesPanel::settingsChanged, this, &ConfigDialog::updateButtons);
    connect(this, &ConfigDialog::updateWidgetsSettings, r, &ConfigReportTemplatesPanel::updateSettings);
    connect(this, &ConfigDialog::updateWidgetsData, r, &ConfigReportTemplatesPanel::updateWidgets);

    auto docPage = new KoConfigDocumentPage(part);
    m_pages << addPage(docPage, i18nc("@title:tab Document settings page", "Document"));
    m_pages.last()->setIcon(koIcon("document-properties"));
//     connect(this, &ConfigDialog::updateWidgetsSettings, docPage, &KoConfigDocumentPage::apply);

//     m_pages << addPage(new ConfigDocumentationPanel(), i18n("Documentation"), koIconName("documents"));
}

void ConfigDialog::updateSettings()
{
    Q_EMIT updateWidgetsSettings();

    KPlatoSettings::self()->save();
    Q_EMIT settingsUpdated();
//     Help::instance()->setContentsUrl(QUrl(KPlatoSettings::self()->documentationPath()));
//     Help::instance()->setContextUrl(QUrl(KPlatoSettings::self()->contextPath()));
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
    Help::instance()->invokeContext(QUrl(QStringLiteral("plan:configure-plan-dialog")));
}

