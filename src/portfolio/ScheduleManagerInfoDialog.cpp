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
        if (i.newManager) {
            action = i18n("New schedule");
        } else if (i.subschedule) {
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
        if (!sm) {
            info.newManager = true;
            info.manager = project->createScheduleManager();
            info.name = info.manager->name();
            info.state = i18n("New schedule '%1' will be created.", info.name);

        } else if (!sm->isScheduled()) {
            info.schedule = true;
            info.unscheduled = true;
            info.name = sm->name();
            info.state = i18n("Existing schedule '%1' is not scheduled.\n'%1' will be scheduled.", info.name);
        } else if (sm->isBaselined()) {
            // create a subschedule
            info.subschedule = true;
            info.manager = project->createScheduleManager(sm);
            info.parentManager = sm;
            info.name = info.manager->name();
            info.state = i18n("Project is baselined.\n'%1' will be created as sub-schedule to '%2'.", info.name, sm->name());
        } else if (project->isStarted() && !sm->parentManager()) {
            if (sm->property(ORIGINALSCHEDULEMANAGER).toBool()) {
                info.subschedule = true;
                info.manager = project->createScheduleManager(sm);
                info.parentManager = sm;
                info.name = info.manager->name();
                info.state = i18n("Project is started.\n'%1' will be created as sub-schedule to '%2'.", info.name, sm->name());
            } else {
                info.schedule = true;
                info.name = sm->name();
                info.state = i18n("Project is started.\nSchedule '%1' will be re-scheduled.", info.name);
            }
        } else if (project->isStarted() && sm->parentManager()) {
            if (sm->property(ORIGINALSCHEDULEMANAGER).toBool()) {
                info.subschedule = true;
                info.manager = project->createScheduleManager(sm);
                info.parentManager = sm;
                info.name = info.manager->name();
                info.state = i18n("Project is started.\n'%1' will be created as sub-schedule to '%2'.", info.name, sm->name());
            } else {
                info.schedule = true;
                info.name = sm->name();
                info.state = i18n("Project is started.\nExisting schedule '%1' will be re-scheduled.", info.name);
            }
        } else {
            // scheduled, not started, not baselined
            if (sm->property(ORIGINALSCHEDULEMANAGER).toBool()) {
                info.manager = project->createScheduleManager(sm->parentManager());
                info.parentManager = sm->parentManager();
                info.name = info.manager->name();
                if (sm->parentManager()) {
                    info.state = i18n("Project is not started.\nNew sub-schedule '%1'.\n'%1' will be created as sub-schedule to '%2'.", info.name, sm->parentManager()->name());
                } else {
                    info.state = i18n("Project is not started.\nNew schedule '%1' will be created.", info.name);
                }
            } else {
                info.schedule = true;
                info.state = i18n("Project is not started.\nExisting schedule '%1' will be re-scheduled.", info.name);
            }
        }
        m_scheduleManagerInfoList << info;
    }
}
