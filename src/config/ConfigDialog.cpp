/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "ConfigDialog.h"

#include "ConfigProjectPanel.h"
#include "ConfigWorkVacationPanel.h"
#include "kpttaskdefaultpanel.h"
#include "kptworkpackageconfigpanel.h"
#include "kptcolorsconfigpanel.h"
#include "ConfigDocumentationPanel.h"
#include "ConfigTaskModulesPanel.h"
#include "ConfigProjectTemplatesPanel.h"

#include <calligraplansettings.h>
#include <Help.h>
#include <KoIcon.h>

#include <KConfigSkeleton>
#include <KLocalizedString>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QDebug>

using namespace KPlato;

ConfigDialog::ConfigDialog(QWidget *parent, const QString& name, KConfigSkeleton *config)
: KConfigDialog(parent, name, config)
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
    m_pages << addPage(new ConfigDocumentationPanel(), i18n("Documentation"), koIconName("documents"));

    ConfigProjectTemplatesPanel *p = new ConfigProjectTemplatesPanel();
    m_pages << addPage(p, i18n("Project Templates"), koIconName("calligraplan"));
    connect(p, &ConfigProjectTemplatesPanel::settingsChanged, this, &ConfigDialog::updateButtons);
    connect(this, &ConfigDialog::updateWidgetsSettings, p, &ConfigProjectTemplatesPanel::updateSettings);
    connect(this, &ConfigDialog::updateWidgetsData, p, &ConfigProjectTemplatesPanel::updateWidgets);
}

void ConfigDialog::updateSettings()
{
    emit updateWidgetsSettings();

    KPlatoSettings::self()->save();
    emit settingsUpdated();
}

void ConfigDialog::updateWidgets()
{
    emit updateWidgetsData();
}

bool ConfigDialog::hasChanged()
{
    QWidget *w = currentPage()->widget()->findChild<QWidget*>("ConfigWidget");
    return w ? w->property("hasChanged").toBool() : false;
}

void ConfigDialog::showHelp()
{
    Help::invoke("Configure_Plan_Dialog");
}

