/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CONFIGWORKVACATIONPANEL_H
#define CONFIGWORKVACATIONPANEL_H

#include "plan_export.h"

#include "ui_ConfigWorkVacationPanel.h"

#include <QWidget>

namespace KPlato
{


class ConfigWorkVacationPanelImpl : public QWidget, public Ui::ConfigWorkVacationPanel
{
    Q_OBJECT
public:
    explicit ConfigWorkVacationPanelImpl(QWidget *parent);

private Q_SLOTS:
#ifdef HAVE_KHOLIDAYS
    void slotRegionChanged(int idx);
    void slotRegionCodeChanged(const QString &code);
#endif
};

class PLAN_EXPORT ConfigWorkVacationPanel : public ConfigWorkVacationPanelImpl
{
    Q_OBJECT
public:
    explicit ConfigWorkVacationPanel(QWidget *parent=nullptr);
    
};

} //KPlato namespace

#endif // CONFIGWORKVACATIONPANEL_H
