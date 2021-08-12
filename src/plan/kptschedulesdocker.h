/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTSCHEDULESDOCKER_H
#define KPTSCHEDULESDOCKER_H

#include <QDockWidget>
#include <KoDockFactoryBase.h>

#include "kptschedulemodel.h"

#include <QSortFilterProxyModel>

class QTreeView;

namespace KPlato
{

class Project;
class ScheduleManager;

class SchedulesDocker : public QDockWidget
{
    Q_OBJECT
public:
    explicit SchedulesDocker();
    ~SchedulesDocker() override;

    ScheduleManager *selectedSchedule() const;

Q_SIGNALS:
    void selectionChanged(KPlato::ScheduleManager *sm);

public Q_SLOTS:
    void setProject(KPlato::Project *project);
    void setSelectedSchedule(KPlato::ScheduleManager *sm);

protected Q_SLOTS:
    void slotSelectionChanged();

private:
    QTreeView *m_view;
    QSortFilterProxyModel m_sfModel;
    ScheduleItemModel m_model;
};

class SchedulesDockerFactory : public KoDockFactoryBase
{
public:
    SchedulesDockerFactory();

    QString id() const override;
    QDockWidget* createDockWidget() override;
    /// @return the dock widget area the widget should appear in by default
    KoDockFactoryBase::DockPosition defaultDockPosition() const override { return DockLeft; }

};

} //namespace KPlato

#endif
