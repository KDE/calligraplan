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
    QList<QModelIndex> lst = ui.projectTemplatesView->selectionModel()->selectedRows();
    for (const QModelIndex &idx : lst) {
        model.removeRow(idx.row(), idx.parent());
    }
    Q_EMIT settingsChanged();
}

