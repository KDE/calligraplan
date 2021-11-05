/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PROJECTLOADER_V0_H
#define PROJECTLOADER_V0_H

#include "plankernel_export.h"

#include "ProjectLoaderBase.h"

#include "kptaccount.h"
#include "kpttask.h"


namespace KPlato
{
    class XMLLoaderObject;
    class Project;
    class Task;
    class Calendar;
    class CalendarDay;
    class CalendarWeekdays;
    class StandardWorktime;
    class Relation;
    class ResourceGroup;
    class Resource;
    class Accounts;
    class Account;
    class ScheduleManager;
    class Schedule;
    class MainSchedule;
    class NodeSchedule;
    class WBSDefinition;
    class WorkPackage;
    class Documents;
    class Document;
    class Estimate;
//     class ResourceGroupRequest;
    class ResourceRequest;
    class Appointment;
    class AppointmentIntervalList;
    class AppointmentInterval;

class PLANKERNEL_EXPORT ProjectLoader_v0 : public ProjectLoaderBase
{
    Q_OBJECT
public:
    ProjectLoader_v0();

    bool load(XMLLoaderObject &context, const KoXmlDocument &document = KoXmlDocument()) override;

    bool load(Project *project, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool loadSettings(const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Task *task, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Calendar *calendar, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(CalendarDay *day, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(CalendarWeekdays *weekdays, const KoXmlElement& element, XMLLoaderObject &status) override;
    bool load(StandardWorktime *swt, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Relation *relation, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(ResourceGroup *rg, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Resource *resource, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Accounts &accounts, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Account *account, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Account::CostPlace *cp, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(ScheduleManager *manager, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Schedule *schedule, const KoXmlElement &element, XMLLoaderObject &status) override;
    MainSchedule *loadMainSchedule(ScheduleManager* manager, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool loadMainSchedule(MainSchedule* ms, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool loadNodeSchedule(NodeSchedule* sch, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(WBSDefinition &def, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Documents &documents, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Document *document, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Estimate *estimate, const KoXmlElement &element, XMLLoaderObject &status) override;
//     bool load(ResourceGroupRequest *gr, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(ResourceRequest *rr, const KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(WorkPackage& wp, const KoXmlElement& element, XMLLoaderObject &status) override;
    bool loadWpLog(WorkPackage* wp, KoXmlElement &element, XMLLoaderObject &status) override;
    bool load(Completion& completion, const KoXmlElement& element, XMLLoaderObject &status) override;
    bool load(Completion::UsedEffort *ue, const KoXmlElement& element, XMLLoaderObject &status) override;
    bool load(Appointment *appointment, const KoXmlElement& element, XMLLoaderObject& status, Schedule &sch) override;
    bool load(AppointmentIntervalList &lst, const KoXmlElement& element, XMLLoaderObject &status) override;
    bool load(AppointmentInterval &interval, const KoXmlElement& element, XMLLoaderObject &status) override;
};

} // namespace KPlato

#endif
