/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CONFIGPROJECTPANEL_H
#define CONFIGPROJECTPANEL_H

#include "plan_export.h"

#include "ui_ConfigProjectPanel.h"

#include <QWidget>

namespace KPlato
{


class ConfigProjectPanelImpl : public QWidget, public Ui::ConfigProjectPanel
{
    Q_OBJECT
public:
    explicit ConfigProjectPanelImpl(QWidget *parent);

    void initDescription();

public Q_SLOTS:
    void resourceFileBrowseBtnClicked();
};

class PLAN_EXPORT ConfigProjectPanel : public ConfigProjectPanelImpl
{
    Q_OBJECT
public:
    explicit ConfigProjectPanel(QWidget *parent=nullptr);
    
};

} //KPlato namespace

#endif // CONFIGPROJECTPANEL_H
