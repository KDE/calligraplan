/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004, 2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTCONFIG_H
#define KPTCONFIG_H

#include "kptconfigbase.h"


namespace KPlato
{

class Config : public ConfigBase
{
    Q_OBJECT
public:
    Config();
    ~Config() override;

    void readConfig();
    void saveSettings();

    bool isWorkingday(int day) const;
    QTime dayStartTime(int day) const;
    int dayLength(int day) const;

    void setDefaultValues(Project &project) const override;

    void setDefaultValues(Task &task) override;

    int minimumDurationUnit() const override;
    int maximumDurationUnit() const override;

    bool summaryTaskLevelColorsEnabled() const override;
    QBrush summaryTaskDefaultColor() const override;
    QBrush summaryTaskLevelColor_1() const override;
    QBrush summaryTaskLevelColor_2() const override;
    QBrush summaryTaskLevelColor_3() const override;
    QBrush summaryTaskLevelColor_4() const override;

    QBrush taskNormalColor() const override;
    QBrush taskErrorColor() const override;
    QBrush taskCriticalColor() const override;
    QBrush taskFinishedColor() const override;

    QBrush milestoneNormalColor() const override;
    QBrush milestoneErrorColor() const override;
    QBrush milestoneCriticalColor() const override;
    QBrush milestoneFinishedColor() const override;

    QString documentationPath() const override;
    QString contextPath() const override;
    QString contextLanguage() const override;

    bool useLocalTaskModules() const override;
    QStringList taskModulePaths() const override;

    QStringList projectTemplatePaths() const override;
};

}  //KPlato namespace

#endif // CONFIG_H
