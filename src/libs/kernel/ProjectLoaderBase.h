/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PROJECTLOADERBASE_H
#define PROJECTLOADERBASE_H

#include "plankernel_export.h"

#include "kptaccount.h"
#include "kpttask.h"

#include <KoXmlReader.h>

#include <QObject>


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
    class ResourceGroupRequest;
    class ResourceRequest;
    class Appointment;
    class AppointmentIntervalList;
    class AppointmentInterval;

class PLANKERNEL_EXPORT ProjectLoaderBase : public QObject
{
public:
    ProjectLoaderBase() {};

    virtual bool load(XMLLoaderObject &context, const KoXmlDocument &document = KoXmlDocument()) {
        Q_UNUSED(context)
        Q_UNUSED(document)
        return false;
    }

    virtual bool load(Project *project, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(project) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool loadSettings(const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Task *task, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(task) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Calendar *calendar, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(calendar) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(CalendarDay *day, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(day) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(CalendarWeekdays *weekdays, const KoXmlElement& element, XMLLoaderObject& status) { Q_UNUSED(weekdays) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(StandardWorktime *swt, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(swt) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Relation *relation, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(relation) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(ResourceGroup *rg, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(rg) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Resource *resource, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(resource) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Accounts &accounts, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(accounts) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Account *account, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(account) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Account::CostPlace *cp, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(cp) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(ScheduleManager *manager, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(manager) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Schedule *schedule, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(schedule) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual MainSchedule *loadMainSchedule(ScheduleManager* manager, const KoXmlElement &element, XMLLoaderObject& status) { Q_UNUSED(manager) Q_UNUSED(element) Q_UNUSED(status) return nullptr; }
    virtual bool loadMainSchedule(MainSchedule* ms, const KoXmlElement &element, XMLLoaderObject& status) { Q_UNUSED(ms) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool loadNodeSchedule(NodeSchedule* sch, const KoXmlElement &element, XMLLoaderObject& status) { Q_UNUSED(sch) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(WBSDefinition &def, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(def) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Documents &documents, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(documents) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Document *document, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(document) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Estimate *estimate, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(estimate) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(ResourceRequest *rr, const KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(rr) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(WorkPackage& wp, const KoXmlElement& element, XMLLoaderObject& status) { Q_UNUSED(wp) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool loadWpLog(WorkPackage* wp, KoXmlElement &element, XMLLoaderObject &status) { Q_UNUSED(wp) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Completion& completion, const KoXmlElement& element, XMLLoaderObject& status) { Q_UNUSED(completion) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Completion::UsedEffort *ue, const KoXmlElement& element, XMLLoaderObject& status) { Q_UNUSED(ue) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(Appointment *appointment, const KoXmlElement& element, XMLLoaderObject& status, Schedule &sch) { Q_UNUSED(appointment) Q_UNUSED(element) Q_UNUSED(status) Q_UNUSED(sch) return false; }
    virtual bool load(AppointmentIntervalList &lst, const KoXmlElement& element, XMLLoaderObject& status) { Q_UNUSED(lst) Q_UNUSED(element) Q_UNUSED(status) return false; }
    virtual bool load(AppointmentInterval &interval, const KoXmlElement& element, XMLLoaderObject& status) { Q_UNUSED(interval) Q_UNUSED(element) Q_UNUSED(status) return false; }
};

} // namespace KPlato

#endif
