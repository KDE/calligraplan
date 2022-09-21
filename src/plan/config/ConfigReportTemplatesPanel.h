/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef CONFIGREPORTTEMPLATESPANEL_H
#define CONFIGREPORTTEMPLATESPANEL_H

#include "plan_export.h"

#include "ui_ConfigReportTemplatesPanel.h"

#include <QWidget>
#include <QStringListModel>

namespace KPlato
{


class PLAN_EXPORT ConfigReportTemplatesPanel : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool hasChanged READ hasChanged) // clazy:exclude=qproperty-without-notify

public:
    explicit ConfigReportTemplatesPanel(QWidget *parent=nullptr);

    QStringListModel model;
    Ui::ConfigReportTemplatesPanel ui;

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

#endif // CONFIGREPORTTEMPLATESPANEL_H
