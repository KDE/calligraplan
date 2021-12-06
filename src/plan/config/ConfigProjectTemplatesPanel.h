/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef CONFIGPROJECTTEMPLATESPANEL_H
#define CONFIGPROJECTTEMPLATESPANEL_H

#include "plan_export.h"

#include "ui_ConfigProjectTemplatesPanel.h"

#include <QWidget>
#include <QStringListModel>

namespace KPlato
{


class PLAN_EXPORT ConfigProjectTemplatesPanel : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool hasChanged READ hasChanged) // clazy:exclude=qproperty-without-notify

public:
    explicit ConfigProjectTemplatesPanel(QWidget *parent=nullptr);

    QStringListModel model;
    Ui::ConfigProjectTemplatesPanel ui;

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

#endif
