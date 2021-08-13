/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef CONFIGTASKMODULESPANEL_H
#define CONFIGTASKMODULESPANEL_H

#include "plan_export.h"

#include "ui_ConfigTaskModulesPanel.h"

#include <QWidget>
#include <QStringListModel>

namespace KPlato
{


class PLAN_EXPORT ConfigTaskModulesPanel : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool hasChanged READ hasChanged)

public:
    explicit ConfigTaskModulesPanel(QWidget *parent=nullptr);

    QStringListModel model;
    Ui::ConfigTaskModulesPanel ui;

    bool hasChanged() const;

public Q_SLOTS:
    void updateSettings();
    void updateWidgets();

Q_SIGNALS:
    void settingsChanged();

private Q_SLOTS:
    void slotInsertClicked();
    void slotRemoveClicked();
};

} //KPlato namespace

#endif // CONFIGTASKMODULESPANEL_H
