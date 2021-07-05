/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTTASKDEFAULTPANEL_H
#define KPTTASKDEFAULTPANEL_H

#include "plan_export.h"

#include "ui_kptconfigtaskpanelbase.h"

#include <QWidget>

namespace KPlato
{

class DateTime;
class Task;


class ConfigTaskPanelImpl : public QWidget, public Ui_ConfigTaskPanelBase
{
    Q_OBJECT
public:
    explicit ConfigTaskPanelImpl(QWidget *parent);

    void initDescription();

public Q_SLOTS:
    virtual void changeLeader();
    void startDateTimeChanged(const QDateTime&);
    void endDateTimeChanged(const QDateTime&);
    void unitChanged(int unit);
    void currentUnitChanged(int);
};

class PLAN_EXPORT TaskDefaultPanel : public ConfigTaskPanelImpl
{
    Q_OBJECT
public:
    explicit TaskDefaultPanel(QWidget *parent=nullptr);
    
};

} //KPlato namespace

#endif // TASKDEFAULTPANEL_H
