/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SCHEDULEMANAGERINFODIALOG_H
#define SCHEDULEMANAGERINFODIALOG_H

#include <QDialog>
#include <ui_ScheduleManagerInfoDialog.h>

#include <QList>

class KoDocument;

namespace KPlato {
class Project;
class ScheduleManager;
}

struct ScheduleManagerInfo
{
    QString projectName;
    QString name;
    bool newTopLevelManager = false;
    bool newSubSchedule = false;
    bool schedule = false;
    bool unscheduled = false;
    QString state;
    KoDocument *document = nullptr;
    KPlato::Project *project = nullptr;
    KPlato::ScheduleManager *manager = nullptr;
    KPlato::ScheduleManager *parentManager = nullptr;
};

class ScheduleManagerInfoDialog : public QDialog
{
    Q_OBJECT
public:
    ScheduleManagerInfoDialog(const QList<KoDocument*> &documents);

    const QList<ScheduleManagerInfo> &scheduleManagerInfoList() const;

private:
    void createScheduleManagerInfoList(const QList<KoDocument*> &documents);
    QList<ScheduleManagerInfo> m_scheduleManagerInfoList;
    Ui::ScheduleManagerInfoDialog ui;
};

#endif // SCHEDULEMANAGERINFODIALOG_H
