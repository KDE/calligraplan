/* This file is part of the KDE project
   Copyright (C) 2004, 2007 Dag Andersen <dag.andersen@kdemail.net>
   Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
