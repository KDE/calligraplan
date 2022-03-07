/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ConfigTaskModulesPanel.h"

#include "calligraplansettings.h"

#include <QFileDialog>
#include <QDebug>

using namespace KPlato;

ConfigTaskModulesPanel::ConfigTaskModulesPanel(QWidget *parent)
{
    setObjectName(QStringLiteral("ConfigWidget"));
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
    Q_EMIT settingsChanged();
}

void ConfigTaskModulesPanel::slotRemoveClicked()
{
    const QList<QModelIndex> lst = ui.taskModulesView->selectionModel()->selectedRows();
    for (const QModelIndex &idx : lst) {
        model.removeRow(idx.row(), idx.parent());
    }
    Q_EMIT settingsChanged();
}

