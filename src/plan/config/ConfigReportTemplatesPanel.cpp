/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ConfigReportTemplatesPanel.h"

#include "calligraplansettings.h"

#include <QFileDialog>

using namespace KPlato;

ConfigReportTemplatesPanel::ConfigReportTemplatesPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ConfigWidget"));
    ui.setupUi(this);
    model.setStringList(KPlatoSettings::reportTemplatePaths());
    ui.reportTemplatesView->setModel(&model);

    connect(ui.insertModule, &QToolButton::clicked, this, &ConfigReportTemplatesPanel::slotInsertClicked);
    connect(ui.removeModule, &QToolButton::clicked, this, &ConfigReportTemplatesPanel::slotRemoveClicked);

    connect(&model, &QStringListModel::dataChanged, this, &ConfigReportTemplatesPanel::settingsChanged);
}

bool ConfigReportTemplatesPanel::hasChanged() const
{
    bool changed = KPlatoSettings::reportTemplatePaths() != model.stringList();
    return changed;
}

void ConfigReportTemplatesPanel::updateSettings()
{
    KPlatoSettings::setReportTemplatePaths(model.stringList());
}

void ConfigReportTemplatesPanel::updateWidgets()
{
    model.setStringList(KPlatoSettings::reportTemplatePaths());
}

void ConfigReportTemplatesPanel::slotInsertClicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, i18n("Task Modules Path"));
    if (!dirName.isEmpty()) {
        model.setStringList(model.stringList() << dirName);
    }
    Q_EMIT settingsChanged();
}

void ConfigReportTemplatesPanel::slotRemoveClicked()
{
    const QList<QModelIndex> lst = ui.reportTemplatesView->selectionModel()->selectedRows();
    for (const QModelIndex &idx : lst) {
        model.removeRow(idx.row(), idx.parent());
    }
    Q_EMIT settingsChanged();
}

