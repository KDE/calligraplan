/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLANPORTFOLIO_GANTTMODEL_H
#define PLANPORTFOLIO_GANTTMODEL_H

#include "ProjectsModel.h"

namespace {
    class Project;
    class ScheduleManager;
}

class GanttModel : public ProjectsFilterModel
{
    Q_OBJECT

public:
    explicit GanttModel(QObject *parent = nullptr);
    ~GanttModel();

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    QDateTime projectRestartTime(const KPlato::Project *project, KPlato::ScheduleManager *sm) const;
};

#endif
