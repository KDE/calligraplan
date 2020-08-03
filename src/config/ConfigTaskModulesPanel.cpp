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

// clazy:excludeall=qstring-arg
#include "ConfigTaskModulesPanel.h"

#include "calligraplansettings.h"

#include <QFileDialog>
#include <QDebug>

using namespace KPlato;

ConfigTaskModulesPanel::ConfigTaskModulesPanel(QWidget *parent)
{
    setObjectName("ConfigWidget");
    ui.setupUi(this);
    model.setStringList(KPlatoSettings::taskModulePaths());
    ui.taskModulesView->setModel(&model);

    connect(ui.insertModule, &QToolButton::clicked, this, &ConfigTaskModulesPanel::slotInsertClicked);
    connect(ui.removeModule, &QToolButton::clicked, this, &ConfigTaskModulesPanel::slotRemoveClicked);

    connect(&model, &QStringListModel::dataChanged, this, &ConfigTaskModulesPanel::settingsChanged);
}

bool ConfigTaskModulesPanel::hasChanged() const
{
    bool changed = KPlatoSettings::taskModulePaths() != model.stringList();
    return changed;
}

void ConfigTaskModulesPanel::updateSettings()
{
    KPlatoSettings::setTaskModulePaths(model.stringList());
}

void ConfigTaskModulesPanel::updateWidgets()
{
    model.setStringList(KPlatoSettings::taskModulePaths());
}

void ConfigTaskModulesPanel::slotInsertClicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, i18n("Task Modules Path"));
    if (!dirName.isEmpty()) {
        model.setStringList(model.stringList() << dirName);
    }
    emit settingsChanged();
}

void ConfigTaskModulesPanel::slotRemoveClicked()
{
    QList<QModelIndex> lst = ui.taskModulesView->selectionModel()->selectedRows();
    for (const QModelIndex &idx : lst) {
        model.removeRow(idx.row(), idx.parent());
    }
    emit settingsChanged();
}

