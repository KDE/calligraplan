/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ScheduleManagerInfoDialog.h"

#include <kptproject.h>
#include <kptschedule.h>
#include <KoDocument.h>
#include <ExtraProperties.h>

#include <QStandardItemModel>
#include <QStandardItem>

ScheduleManagerInfoDialog::ScheduleManagerInfoDialog(const QList<KoDocument*> &documents)
{
    ui.setupUi(this);

    auto m = new QStandardItemModel(this);
    QStringList headers = {i18nc("@title:column", "Project"), i18nc("@title:column", "Schedule"), i18nc("@title:column", "Action"), i18nc("@title:column", "State")};
    m->setHorizontalHeaderLabels(headers);
    createScheduleManagerInfoList(documents);
    for (int c = m_scheduleManagerInfoList.count(); --c >= 0;) {
        auto i = m_scheduleManagerInfoList.at(c);
        QList<QStandardItem*> items;
        items << new QStandardItem(i.projectName);
        items << new QStandardItem(i.name);
        QString action;
        if (i.newTopLevelManager) {
            action = i18n("New schedule");
        } else if (i.newSubSchedule) {
            action = i18n("New sub-schedule");
        } else if (i.schedule) {
            action = i18n("Schedule");
        } else {
            action = i18n("New schedule");
        }
        items << new QStandardItem(action);
        items << new QStandardItem(i.state);
        m->appendRow(items);
    }
    ui.treeView->setModel(m);
    ui.treeView->resizeColumnToContents(0);
    ui.treeView->resizeColumnToContents(1);
    ui.treeView->resizeColumnToContents(2);
}

const QList<ScheduleManagerInfo> &ScheduleManagerInfoDialog::scheduleManagerInfoList() const
{
    return m_scheduleManagerInfoList;
}

void ScheduleManagerInfoDialog::createScheduleManagerInfoList(const QList<KoDocument*> &documents)
{
    for (const auto document : documents) {
        ScheduleManagerInfo info;
        info.document = document;
        const auto project = document->project();
        info.project = project;
        info.projectName = project->name();
        auto sm = project->findScheduleManagerByName(document->property(SCHEDULEMANAGERNAME).toString());
        if (!sm) {
            sm = project->currentScheduleManager();
        }
        // sm should now be the schedule user has selected (if there is a schedule at all).
        // The general idea is to not touch the schedule if we do not own it.
        // Plan should claim the schedule if it wants to block us out.
        if (!sm) {
            // No schedule, just create a new one
            info.newTopLevelManager = true;
            info.manager = project->createScheduleManager(info.parentManager);
            info.manager->setOwner(KPlato::ScheduleManager::OwnerPortfolio);
            info.name = info.manager->name();
            info.state = i18n("New schedule '%1' will be created.", info.name);
        } else if (sm->owner() == KPlato::ScheduleManager::OwnerPortfolio) {
            // We own this schedule so we can schedule it
            info.schedule = true;
            info.unscheduled = !sm->isScheduled();
            info.manager = project->createScheduleManager(info.parentManager);
            info.manager->setOwner(KPlato::ScheduleManager::OwnerPortfolio);
            info.name = info.manager->name();
            info.state = i18n("Existing schedule '%1' is owned by Portfolio.\n'%1' will be scheduled.", info.name);
        } else if (!project->isStarted()) {
            // create a new schedule on the same level as sm
            info.parentManager = sm->parentManager();
            info.newSubSchedule = info.parentManager != nullptr;;
            info.newTopLevelManager = !info.newSubSchedule;
            info.manager = project->createScheduleManager(info.parentManager);
            info.manager->setOwner(KPlato::ScheduleManager::OwnerPortfolio);
            info.name = info.manager->name();
            info.state = i18n("Existing schedule '%1' is owned by Plan.\nNew shcedule '%2' will be created.", sm->name(), info.name);
        } else if (project->isStarted() && !sm->parentManager()) {
            // project is started and sm is a top-level schedule so create new sub-schedule to sm
            info.newSubSchedule = true;
            info.parentManager = sm;
            info.manager = project->createScheduleManager(info.parentManager);
            info.manager->setOwner(KPlato::ScheduleManager::OwnerPortfolio);
            info.name = info.manager->name();
            info.state = i18n("Project is started.\n'%1' will be created as sub-schedule to '%2'.", info.name, sm->name());
        } else if (sm->parentManager()) {
            Q_ASSERT(sm->parentManager()->isScheduled());
            // project is started and sm is not a top-level schedule so create new sub-schedule to sm->parentManager()
            info.newSubSchedule = true;
            info.parentManager = sm->parentManager();
            info.manager = project->createScheduleManager(info.parentManager);
            info.manager->setOwner(KPlato::ScheduleManager::OwnerPortfolio);
            info.name = info.manager->name();
            info.state = i18n("Project is started.\n'%1' will be created as sub-schedule to '%2'.", info.name, sm->name());
        } else {
            Q_ASSERT(false); // should not get here
        }

        m_scheduleManagerInfoList << info;
    }
}
