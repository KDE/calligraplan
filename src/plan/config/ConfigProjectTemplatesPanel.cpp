/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ConfigProjectTemplatesPanel.h"

#include "calligraplansettings.h"

#include <QFileDialog>
#include <QDebug>

using namespace KPlato;

ConfigProjectTemplatesPanel::ConfigProjectTemplatesPanel(QWidget *parent)
{
    setObjectName("ConfigWidget");
    ui.setupUi(this);
    model.setStringList(KPlatoSettings::projectTemplatePaths());
    ui.projectTemplatesView->setModel(&model);

    connect(ui.insertTemplatePath, &QToolButton::clicked, this, &ConfigProjectTemplatesPanel::slotInsertClicked);
    connect(ui.removeTemplatePath, &QToolButton::clicked, this, &ConfigProjectTemplatesPanel::slotRemoveClicked);

    connect(&model, &QStringListModel::dataChanged, this, &ConfigProjectTemplatesPanel::settingsChanged);
}

bool ConfigProjectTemplatesPanel::hasChanged() const
{
    bool changed = KPlatoSettings::projectTemplatePaths() != model.stringList();
    return changed;
}

void ConfigProjectTemplatesPanel::updateSettings()
{
    KPlatoSettings::setProjectTemplatePaths(model.stringList());
}

void ConfigProjectTemplatesPanel::updateWidgets()
{
    model.setStringList(KPlatoSettings::projectTemplatePaths());
}

void ConfigProjectTemplatesPanel::slotInsertClicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, i18n("Project Templates Path"));
    if (!dirName.isEmpty()) {
        model.setStringList(model.stringList() << dirName);
    }
    Q_EMIT settingsChanged();
}

void ConfigProjectTemplatesPanel::slotRemoveClicked()
{
    const QList<QModelIndex> lst = ui.projectTemplatesView->selectionModel()->selectedRows();
    for (const QModelIndex &idx : lst) {
        model.removeRow(idx.row(), idx.parent());
    }
    Q_EMIT settingsChanged();
}

