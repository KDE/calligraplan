/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KPlatoXmlLoaderBase.h"

#include "kptlocale.h"
#include "kptxmlloaderobject.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptcalendar.h"
#include "kptschedule.h"
#include "kptrelation.h"
#include "kptresource.h"
#include "kptaccount.h"
#include "kptappointment.h"

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QTimeZone>


using namespace KPlato;

KPlatoXmlLoaderBase::KPlatoXmlLoaderBase()
{
}

bool KPlatoXmlLoaderBase::load(Project *project, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"project";
    // load locale first
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("locale")) {
            Locale *l = project->locale();
            l->setCurrencySymbol(e.attribute(QStringLiteral("currency-symbol"), l->currencySymbol()));
            //NOTE: decimalplaces missing
        }
    }
    QList<Calendar*> cals;
    QString s;
    bool ok = false;
    project->setName(element.attribute(QStringLiteral("name")));
    project->removeId(project->id());
    project->setId(element.attribute(QStringLiteral("id")));
    project->registerNodeId(project);
    project->setLeader(element.attribute(QStringLiteral("leader")));
    project->setDescription(element.attribute(QStringLiteral("description")));
    QTimeZone tz(element.attribute(QStringLiteral("timezone")).toLatin1());

    if (tz.isValid()) {
        project->setTimeZone(tz);
    } else warnPlanXml<<"No timezone specified, using default (local)";
    status.setProjectTimeZone(project->timeZone());

    // Allow for both numeric and text
    s = element.attribute(QStringLiteral("scheduling"), QString::number(0));
    project->setConstraint((Node::ConstraintType) (s.toInt(&ok)));
    if (! ok)
        project->setConstraint(s);
    if (project->constraint() != Node::MustStartOn && project->constraint() != Node::MustFinishOn) {
        errorPlanXml << "Illegal constraint: " << project->constraintToString();
        project->setConstraint(Node::MustStartOn);
    }
    s = element.attribute(QStringLiteral("start-time"));
    if (! s.isEmpty()) {
        project->setConstraintStartTime(DateTime::fromString(s, project->timeZone()));
    }
    s = element.attribute(QStringLiteral("end-time"));
    if (! s.isEmpty()) {
        project->setConstraintEndTime(DateTime::fromString(s, project->timeZone()));
    }
    // Load the project children
    // Do calendars first, they only reference other calendars
    //debugPlanXml<<"Calendars--->";
    n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("calendar")) {
            // Load the calendar.
            // Referenced by resources
            Calendar * child = new Calendar();
            child->setProject(project);
            if (load(child, e, status)) {
                cals.append(child); // temporary, reorder later
            } else {
                // TODO: Complain about this
                errorPlanXml << "Failed to load calendar";
                delete child;
            }
        } else if (e.tagName() == QStringLiteral("standard-worktime")) {
            // Load standard worktime
            StandardWorktime * child = new StandardWorktime();
            if (load(child, e, status)) {
                project->setStandardWorktime(child);
            } else {
                errorPlanXml << "Failed to load standard worktime";
                delete child;
            }
        }
    }
    // calendars references calendars in arbitrary saved order
    bool added = false;
    do {
        added = false;
        QList<Calendar*> lst;
        while (!cals.isEmpty()) {
            Calendar *c = cals.takeFirst();
            if (c->parentId().isEmpty()) {
                project->addCalendar(c, status.baseCalendar()); // handle pre 0.6 version
                added = true;
                //debugPlanXml<<"added to project:"<<c->name();
            } else {
                Calendar *par = project->calendar(c->parentId());
                if (par) {
                    project->addCalendar(c, par);
                    added = true;
                    //debugPlanXml<<"added:"<<c->name()<<" to parent:"<<par->name();
                } else {
                    lst.append(c); // treat later
                    //debugPlanXml<<"treat later:"<<c->name();
                }
            }
        }
        cals = lst;
    } while (added);
    if (! cals.isEmpty()) {
        errorPlanXml<<"All calendars not saved!";
    }
    //debugPlanXml<<"Calendars<---";
    // Resource groups and resources, can reference calendars
    n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("resource-group")) {
            // Load the resources
            // References calendars
            ResourceGroup * child = new ResourceGroup();
            if (load(child, e, status)) {
                project->addResourceGroup(child);
            } else {
                // TODO: Complain about this
                delete child;
            }
        }
    }
    // The main stuff
    n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("project")) {
            //debugPlanXml<<"Sub project--->";
/*                // Load the subproject
            Project * child = new Project(this);
            if (child->load(e)) {
                if (!addTask(child, this)) {
                    delete child; // TODO: Complain about this
                }
            } else {
                // TODO: Complain about this
                delete child;
            }*/
        } else if (e.tagName() == QStringLiteral("task")) {
            //debugPlanXml<<"Task--->";
            // Load the task (and resourcerequests).
            // Depends on resources already loaded
            Task *child = new Task(project);
            if (load(child, e, status)) {
                if (! project->addTask(child, project)) {
                    delete child; // TODO: Complain about this
                }
            } else {
                // TODO: Complain about this
                delete child;
            }
        }
    }
    // These go last
    n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        debugPlanXml<<n.isElement();
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("accounts")) {
            //debugPlanXml<<"Accounts--->";
            // Load accounts
            // References tasks
            if (! load(project->accounts(), e, status)) {
                errorPlanXml << "Failed to load accounts";
            }
        } else if (e.tagName() == QStringLiteral("relation")) {
            //debugPlanXml<<"Relation--->";
            // Load the relation
            // References tasks
            Relation * child = new Relation();
            if (! load(child, e, status)) {
                // TODO: Complain about this
                errorPlanXml << "Failed to load relation";
                delete child;
            }
            //debugPlanXml<<"Relation<---";
        } else if (e.tagName() == QStringLiteral("schedules")) {
            debugPlanXml<<"Project schedules & task appointments--->";
            // References tasks and resources
            KoXmlNode sn = e.firstChild();
            for (; ! sn.isNull(); sn = sn.nextSibling()) {
                if (! sn.isElement()) {
                    continue;
                }
                KoXmlElement el = sn.toElement();
                //debugPlanXml<<el.tagName()<<" Version="<<status.version();
                ScheduleManager *sm = nullptr;
                bool add = false;
                if (status.version() <= QStringLiteral("0.5")) {
                    if (el.tagName() == QStringLiteral("schedule")) {
                        sm = project->findScheduleManagerByName(el.attribute(QStringLiteral("name")));
                        if (sm == nullptr) {
                            sm = new ScheduleManager(*project, el.attribute(QStringLiteral("name")));
                            add = true;
                        }
                    }
                } else if (el.tagName() == QStringLiteral("plan")) {
                    sm = new ScheduleManager(*project);
                    add = true;
                }
                if (sm) {
                    if (load(sm, el, status)) {
                        if (add)
                            project->addScheduleManager(sm);
                    } else {
                        errorPlanXml << "Failed to load schedule manager";
                        delete sm;
                    }
                } else {
                    debugPlanXml<<"No schedule manager ?!";
                }
            }
            debugPlanXml<<"Project schedules & task appointments<---";
        } else if (e.tagName() == QStringLiteral("resource-teams")) {
            //debugPlanXml<<"Resource teams--->";
            // References other resources
            KoXmlNode tn = e.firstChild();
            for (; ! tn.isNull(); tn = tn.nextSibling()) {
                if (! tn.isElement()) {
                    continue;
                }
                KoXmlElement el = tn.toElement();
                if (el.tagName() == QStringLiteral("team")) {
                    Resource *r = project->findResource(el.attribute(QStringLiteral("team-id")));
                    Resource *tm = project->findResource(el.attribute(QStringLiteral("member-id")));
                    if (r == nullptr || tm == nullptr) {
                        errorPlanXml<<"resource-teams: cannot find resources";
                        continue;
                    }
                    if (r == tm) {
                        errorPlanXml<<"resource-teams: a team cannot be a member of itself";
                        continue;
                    }
                    r->addTeamMemberId(tm->id());
                } else {
                    errorPlanXml<<"resource-teams: unhandled tag"<<el.tagName();
                }
            }
            //debugPlanXml<<"Resource teams<---";
        } else if (e.tagName() == QStringLiteral("wbs-definition")) {
            load(project->wbsDefinition(), e, status);
        } else if (e.tagName() == QStringLiteral("locale")) {
            // handled earlier
        } else if (e.tagName() == QStringLiteral("resource-group")) {
            // handled earlier
        } else if (e.tagName() == QStringLiteral("calendar")) {
            // handled earlier
        } else if (e.tagName() == QStringLiteral("standard-worktime")) {
            // handled earlier
        } else if (e.tagName() == QStringLiteral("project")) {
            // handled earlier
        } else if (e.tagName() == QStringLiteral("task")) {
            // handled earlier
        } else {
            warnPlanXml<<"Unhandled tag:"<<e.tagName();
        }
    }
    // set schedule parent
    const auto schedules = project->schedules();
    for (Schedule *s : schedules) {
        project->setParentSchedule(s);
    }

    debugPlanXml<<"Project loaded:"<<project<<project->name()<<project->allNodes();
    return true;
}

bool KPlatoXmlLoaderBase::load(Task *task, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"task";
    QString s;
    bool ok = false;
    task->setId(element.attribute(QStringLiteral("id")));

    task->setName(element.attribute(QStringLiteral("name")));
    task->setLeader(element.attribute(QStringLiteral("leader")));
    task->setDescription(element.attribute(QStringLiteral("description")));
    //debugPlanXml<<m_name<<": id="<<m_id;

    // Allow for both numeric and text
    QString constraint = element.attribute(QStringLiteral("scheduling"), QString::number(0));
    task->setConstraint((Node::ConstraintType)constraint.toInt(&ok));
    if (! ok) {
        task->setConstraint(constraint);
    }
    s = element.attribute(QStringLiteral("constraint-starttime"));
    if (! s.isEmpty()) {
        task->setConstraintStartTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    s = element.attribute(QStringLiteral("constraint-endtime"));
    if (! s.isEmpty()) {
        task->setConstraintEndTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    task->setStartupCost(element.attribute(QStringLiteral("startup-cost"), QString::number(0.0)).toDouble());
    task->setShutdownCost(element.attribute(QStringLiteral("shutdown-cost"), QString::number(0.0)).toDouble());

    // Load the task children
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("project")) {
            // Load the subproject
/*                Project *child = new Project(this, status);
            if (child->load(e)) {
                if (!project.addSubTask(child, this)) {
                    delete child;  // TODO: Complain about this
                }
            } else {
                // TODO: Complain about this
                delete child;
            }*/
        } else if (e.tagName() == QStringLiteral("task")) {
            // Load the task
            Task *child = new Task(task);
            if (load(child, e, status)) {
                if (! status.project().addSubTask(child, task)) {
                    delete child;  // TODO: Complain about this
                }
            } else {
                // TODO: Complain about this
                delete child;
            }
        } else if (e.tagName() == QStringLiteral("resource")) {
            // tasks don't have resources
        } else if (e.tagName() == QStringLiteral("estimate") || (/*status.version() < QStringLiteral("0.6") &&*/ e.tagName() == QStringLiteral("effort"))) {
            //  Load the estimate
            load(task->estimate(), e, status);
        } else if (e.tagName() == QStringLiteral("resourcegroup-request")) {
            warnPlan<<"KPlatoXmlLoader: requests not handled";
        } else if (e.tagName() == QStringLiteral("workpackage")) {
            load(task->workPackage(), e, status);
        } else if (e.tagName() == QStringLiteral("progress")) {
            load(task->completion(), e, status);
        } else if (e.tagName() == QStringLiteral("schedules")) {
            KoXmlNode n = e.firstChild();
            for (; ! n.isNull(); n = n.nextSibling()) {
                if (! n.isElement()) {
                    continue;
                }
                KoXmlElement el = n.toElement();
                if (el.tagName() == QStringLiteral("schedule")) {
                    NodeSchedule *sch = new NodeSchedule();
                    if (loadNodeSchedule(sch, el, status)) {
                        sch->setNode(task);
                        task->addSchedule(sch);
                    } else {
                        errorPlanXml<<"Failed to load schedule";
                        delete sch;
                    }
                }
            }
        } else if (e.tagName() == QStringLiteral("documents")) {
            load(task->documents(), e, status);
        } else if (e.tagName() == QStringLiteral("workpackage-log")) {
            KoXmlNode n = e.firstChild();
            for (; ! n.isNull(); n = n.nextSibling()) {
                if (! n.isElement()) {
                    continue;
                }
                KoXmlElement el = n.toElement();
                if (el.tagName() == QStringLiteral("workpackage")) {
                    WorkPackage *wp = new WorkPackage(task);
                    if (loadWpLog(wp, el, status)) {
                        task->addWorkPackage(wp);
                    } else {
                        errorPlanXml<<"Failed to load logged workpackage";
                        delete wp;
                    }
                }
            }
        }
    }
    //debugPlanXml<<m_name<<" loaded";
    return true;
}

bool KPlatoXmlLoaderBase::load(Calendar *calendar, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"calendar"<<element.text();
    //bool ok;
    calendar->setId(element.attribute(QStringLiteral("id")));
    calendar->setParentId(element.attribute(QStringLiteral("parent")));
    calendar->setName(element.attribute(QStringLiteral("name")));
    QTimeZone tz(element.attribute(QStringLiteral("timezone")).toLatin1());
    if (tz.isValid()) {
        calendar->setTimeZone(tz);
    } else warnPlanXml<<"No timezone specified, use default (local)";
    bool dc = (bool)element.attribute(QStringLiteral("default"), QString::number(0)).toInt();
    if (dc) {
        status.project().setDefaultCalendar(calendar);
    }
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("weekday")) {
            if (! load(calendar->weekdays(), e, status)) {
                return false;
            }
        }
        if (e.tagName() == QStringLiteral("day")) {
            CalendarDay *day = new CalendarDay();
            if (load(day, e, status)) {
                if (! day->date().isValid()) {
                    delete day;
                    errorPlanXml<<calendar->name()<<": Failed to load calendarDay - Invalid date";
                } else {
                    CalendarDay *d = calendar->findDay(day->date());
                    if (d) {
                        // already exists, keep the new
                        delete calendar->takeDay(d);
                        warnPlanXml<<calendar->name()<<" Load calendarDay - Date already exists";
                    }
                    calendar->addDay(day);
                }
            } else {
                delete day;
                errorPlanXml<<"Failed to load calendarDay";
                return true; // don't throw away the whole calendar
            }
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(CalendarDay *day, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"day";
    bool ok=false;
    day->setState(QString(element.attribute(QStringLiteral("state"), QString::number(-1))).toInt(&ok));
    if (day->state() < 0) {
        return false;
    }
    //debugPlanXml<<" state="<<m_state;
    QString s = element.attribute(QStringLiteral("date"));
    if (! s.isEmpty()) {
        day->setDate(QDate::fromString(s, Qt::ISODate));
        if (! day->date().isValid()) {
            day->setDate(QDate::fromString(s));
        }
    }
    day->clearIntervals();
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("interval")) {
            //debugPlanXml<<"Interval start="<<e.attribute(QStringLiteral("start")<<" end="<<e.attribute(QStringLiteral("end");
            QString st = e.attribute(QStringLiteral("start"));
            if (st.isEmpty()) {
                errorPlanXml<<"Empty interval";
                continue;
            }
            QTime start = QTime::fromString(st);
            int length = 0;
            if (status.version() <= QStringLiteral("0.6.1")) {
                QString en = e.attribute(QStringLiteral("end"));
                if (en.isEmpty()) {
                    errorPlanXml<<"Invalid interval end";
                    continue;
                }
                QTime end = QTime::fromString(en);
                length = start.msecsTo(end);
            } else {
                length = e.attribute(QStringLiteral("length"), QString::number(0)).toInt();
            }
            if (length <= 0) {
                errorPlanXml<<"Invalid interval length";
                continue;
            }
            day->addInterval(new TimeInterval(start, length));
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(CalendarWeekdays *weekdays, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"weekdays";
    bool ok;
    int dayNo = QString(element.attribute(QStringLiteral("day"),QString::number(-1))).toInt(&ok);
    if (dayNo < 0 || dayNo > 6) {
        errorPlanXml<<"Illegal weekday: "<<dayNo;
        return true; // we continue anyway
    }
    CalendarDay *day = weekdays->weekday(dayNo + 1);
    if (day == nullptr) {
        errorPlanXml<<"No weekday: "<<dayNo;
        return false;
    }
    if (! load(day, element, status)) {
        day->setState(CalendarDay::None);
    }
    return true;

}

bool KPlatoXmlLoaderBase::load(StandardWorktime *swt, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"swt";
    swt->setYear(Duration::fromString(element.attribute(QStringLiteral("year")), Duration::Format_Hour));
    swt->setMonth(Duration::fromString(element.attribute(QStringLiteral("month")), Duration::Format_Hour));
    swt->setWeek(Duration::fromString(element.attribute(QStringLiteral("week")), Duration::Format_Hour));
    swt->setDay(Duration::fromString(element.attribute(QStringLiteral("day")), Duration::Format_Hour));

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("calendar")) {
            // pre 0.6 version stored base calendar in standard worktime
            if (status.version() >= QStringLiteral("0.6")) {
                warnPlanXml<<"Old format, calendar in standard worktime";
                warnPlanXml<<"Tries to load anyway";
            }
            // try to load anyway
            Calendar *calendar = new Calendar;
            if (load(calendar, e, status)) {
                status.project().addCalendar(calendar);
                calendar->setDefault(true);
                status.project().setDefaultCalendar(calendar); // hmmm
                status.setBaseCalendar(calendar);
            } else {
                delete calendar;
                errorPlanXml<<"Failed to load calendar";
            }
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(Relation *relation, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"relation";
    relation->setParent(status.project().findNode(element.attribute(QStringLiteral("parent-id"))));
    if (relation->parent() == nullptr) {
        warnPlanXml<<"Parent node == 0, cannot find id:"<<element.attribute(QStringLiteral("parent-id"));
        return false;
    }
    relation->setChild(status.project().findNode(element.attribute(QStringLiteral("child-id"))));
    if (relation->child() == nullptr) {
        warnPlanXml<<"Child node == 0, cannot find id:"<<element.attribute(QStringLiteral("child-id"));
        return false;
    }
    if (relation->child() == relation->parent()) {
        warnPlanXml<<"Parent node == child node";
        return false;
    }
    if (! relation->parent()->legalToLink(relation->child())) {
        warnPlanXml<<"Relation is not legal:"<<relation->parent()->name()<<"->"<<relation->child()->name();
        return false;
    }
    relation->setType(element.attribute(QStringLiteral("type")));

    relation->setLag(Duration::fromString(element.attribute(QStringLiteral("lag"))));

    if (! relation->parent()->addDependChildNode(relation)) {
        errorPlanXml<<"Failed to add relation: Child="<<relation->child()->name()<<" parent="<<relation->parent()->name();
        return false;
    }
    if (! relation->child()->addDependParentNode(relation)) {
        relation->parent()->takeDependChildNode(relation);
        errorPlanXml<<"Failed to add relation: Child="<<relation->child()->name()<<" parent="<<relation->parent()->name();
        return false;
    }
    //debugPlanXml<<"Added relation: Child="<<relation->child()->name()<<" parent="<<relation->parent()->name();
    return true;
}

bool KPlatoXmlLoaderBase::load(ResourceGroup *rg, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"resource-group";
    rg->setId(element.attribute(QStringLiteral("id")));
    rg->setName(element.attribute(QStringLiteral("name")));
    rg->setType(element.attribute(QStringLiteral("type")));

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("resource")) {
            // Load the resource
            Resource *child = new Resource();
            if (load(child, e, status)) {
                child->addParentGroup(rg);
                status.project().addResource(child);
            } else {
                // TODO: Complain about this
                delete child;
            }
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(Resource *resource, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"resource";
    const Locale *locale = status.project().locale();
    QString s;
    resource->setId(element.attribute(QStringLiteral("id")));
    resource->setName(element.attribute(QStringLiteral("name")));
    resource->setInitials(element.attribute(QStringLiteral("initials")));
    resource->setEmail(element.attribute(QStringLiteral("email")));
    resource->setType(element.attribute(QStringLiteral("type")));
    resource->setCalendar(status.project().findCalendar(element.attribute(QStringLiteral("calendar-id"))));
    resource->setUnits(element.attribute(QStringLiteral("units"), QString::number(100)).toInt());
    s = element.attribute(QStringLiteral("available-from"));
    if (! s.isEmpty()) {
        resource->setAvailableFrom(DateTime::fromString(s, status.projectTimeZone()));
    }
    s = element.attribute(QStringLiteral("available-until"));
    if (! s.isEmpty()) {
        resource->setAvailableUntil(DateTime::fromString(s, status.projectTimeZone()));
    }
    resource->setNormalRate(locale->readMoney(element.attribute(QStringLiteral("normal-rate"))));
    resource->setOvertimeRate(locale->readMoney(element.attribute(QStringLiteral("overtime-rate"))));
    resource->setAccount(status.project().accounts().findAccount(element.attribute(QStringLiteral("account"))));

    KoXmlElement e;
    KoXmlElement parent = element.namedItem(QStringLiteral("required-resources")).toElement();
    forEachElement(e, parent) {
        if (e.nodeName() == QStringLiteral("resource")) {
            QString id = e.attribute(QStringLiteral("id"));
            if (id.isEmpty()) {
                warnPlanXml<<"Missing resource id";
                continue;
            }
            resource->addRequiredId(id);
        }
    }
    parent = element.namedItem(QStringLiteral("external-appointments")).toElement();
    forEachElement(e, parent) {
        if (e.nodeName() == QStringLiteral("project")) {
            QString id = e.attribute(QStringLiteral("id"));
            if (id.isEmpty()) {
                errorPlanXml<<"Missing project id";
                continue;
            }
            resource->clearExternalAppointments(id); // in case...
            AppointmentIntervalList lst;
            load(lst, e, status);
            Appointment *a = new Appointment();
            a->setIntervals(lst);
            a->setAuxcilliaryInfo(e.attribute(QStringLiteral("name"), QStringLiteral("Unknown")));
            resource->addExternalAppointment(id, a);
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(Accounts &accounts, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"accounts";
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("account")) {
            Account *child = new Account();
            if (load(child, e, status)) {
                accounts.insert(child);
            } else {
                // TODO: Complain about this
                warnPlanXml<<"Loading failed";
                delete child;
            }
        }
    }
    if (element.hasAttribute(QStringLiteral("default-account"))) {
        accounts.setDefaultAccount(accounts.findAccount(element.attribute(QStringLiteral("default-account"))));
        if (accounts.defaultAccount() == nullptr) {
            warnPlanXml<<"Could not find default account.";
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(Account* account, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"account";
    account->setName(element.attribute(QStringLiteral("name")));
    account->setDescription(element.attribute(QStringLiteral("description")));
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("costplace")) {
            Account::CostPlace *child = new Account::CostPlace(account);
            if (load(child, e, status)) {
                account->append(child);
            } else {
                delete child;
            }
        } else if (e.tagName() == QStringLiteral("account")) {
            Account *child = new Account();
            if (load(child, e, status)) {
                account->insert(child);
            } else {
                // TODO: Complain about this
                warnPlanXml<<"Loading failed";
                delete child;
            }
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(Account::CostPlace* cp, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"cost-place";
    cp->setObjectId(element.attribute(QStringLiteral("object-id")));
    if (cp->objectId().isEmpty()) {
        // check old format
        cp->setObjectId(element.attribute(QStringLiteral("node-id")));
        if (cp->objectId().isEmpty()) {
            errorPlanXml<<"No object id";
            return false;
        }
    }
    cp->setNode(status.project().findNode(cp->objectId()));
    if (cp->node() == nullptr) {
        cp->setResource(status.project().findResource(cp->objectId()));
        if (cp->resource() == nullptr) {
            errorPlanXml<<"Cannot find object with id: "<<cp->objectId();
            return false;
        }
    }
    bool on = (bool)(element.attribute(QStringLiteral("running-cost")).toInt());
    if (on) cp->setRunning(on);
    on = (bool)(element.attribute(QStringLiteral("startup-cost")).toInt());
    if (on) cp->setStartup(on);
    on = (bool)(element.attribute(QStringLiteral("shutdown-cost")).toInt());
    if (on) cp->setShutdown(on);
    return true;
}

bool KPlatoXmlLoaderBase::load(ScheduleManager *manager, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"schedule-manager";
    MainSchedule *sch = nullptr;
    if (status.version() <= QStringLiteral("0.5")) {
        manager->setUsePert(false);
        MainSchedule *sch = loadMainSchedule(manager, element, status);
        if (sch && sch->type() == Schedule::Expected) {
            sch->setManager(manager);
            manager->setExpected(sch);
        } else {
            delete sch;
        }
        return true;
    }
    manager->setName(element.attribute(QStringLiteral("name")));
    manager->setManagerId(element.attribute(QStringLiteral("id")));
    manager->setUsePert(element.attribute(QStringLiteral("distribution")).toInt() == 1);
    manager->setAllowOverbooking((bool)(element.attribute(QStringLiteral("overbooking")).toInt()));
    manager->setCheckExternalAppointments((bool)(element.attribute(QStringLiteral("check-external-appointments")).toInt()));
    manager->setSchedulingDirection((bool)(element.attribute(QStringLiteral("scheduling-direction")).toInt()));
    manager->setBaselined((bool)(element.attribute(QStringLiteral("baselined")).toInt()));
    manager->setSchedulerPluginId(element.attribute(QStringLiteral("scheduler-plugin-id")));
    manager->setRecalculate((bool)(element.attribute(QStringLiteral("recalculate")).toInt()));
    manager->setRecalculateFrom(DateTime::fromString(element.attribute(QStringLiteral("recalculate-from")), status.projectTimeZone()));
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        //debugPlanXml<<e.tagName();
        if (e.tagName() == QStringLiteral("schedule")) {
            sch = loadMainSchedule(manager, e, status);
            if (sch && sch->type() == Schedule::Expected) {
                sch->setManager(manager);
                manager->setExpected(sch); break;
            } else {
                delete sch;
            }
        } else if (e.tagName() == QStringLiteral("plan")) {
            ScheduleManager *sm = new ScheduleManager(status.project());
            if (load(sm, e, status)) {
                status.project().addScheduleManager(sm, manager);
            } else {
                errorPlanXml<<"Failed to load schedule manager";
                delete sm;
            }
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(Schedule *schedule, const KoXmlElement& element, XMLLoaderObject& /*status*/)
{
    debugPlanXml<<"schedule";
    schedule->setName(element.attribute(QStringLiteral("name")));
    schedule->setType(element.attribute(QStringLiteral("type")));
    schedule->setId(element.attribute(QStringLiteral("id")).toLong());

    return true;
}

MainSchedule* KPlatoXmlLoaderBase::loadMainSchedule(ScheduleManager* /*manager*/, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"main-schedule";
    MainSchedule *sch = new MainSchedule();
    if (loadMainSchedule(sch, element, status)) {
        status.project().addSchedule(sch);
        sch->setNode(&(status.project()));
        status.project().setParentSchedule(sch);
        // If it's here, it's scheduled!
        sch->setScheduled(true);
    } else {
        errorPlanXml << "Failed to load schedule";
        delete sch;
        sch = nullptr;
    }
    return sch;
}

bool KPlatoXmlLoaderBase::loadMainSchedule(MainSchedule *ms, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml;
    QString s;
    load(ms, element, status);

    s = element.attribute(QStringLiteral("start"));
    if (!s.isEmpty()) {
        ms->startTime = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute(QStringLiteral("end"));
    if (!s.isEmpty()) {
        ms->endTime = DateTime::fromString(s, status.projectTimeZone());
    }
    ms->duration = Duration::fromString(element.attribute(QStringLiteral("duration")));
    ms->constraintError = element.attribute(QStringLiteral("scheduling-conflict"), QString::number(0)).toInt();

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement el = n.toElement();
        if (el.tagName() == QStringLiteral("appointment")) {
            // Load the appointments.
            // Resources and tasks must already be loaded
            Appointment * child = new Appointment();
            if (! load(child, el, status, *ms)) {
                // TODO: Complain about this
                errorPlanXml << "Failed to load appointment" << '\n';
                delete child;
            }
        } else if (el.tagName() == QStringLiteral("criticalpath-list")) {
            // Tasks must already be loaded
            for (KoXmlNode n1 = el.firstChild(); ! n1.isNull(); n1 = n1.nextSibling()) {
                if (! n1.isElement()) {
                    continue;
                }
                KoXmlElement e1 = n1.toElement();
                if (e1.tagName() != QStringLiteral("criticalpath")) {
                    continue;
                }
                QList<Node*> lst;
                for (KoXmlNode n2 = e1.firstChild(); ! n2.isNull(); n2 = n2.nextSibling()) {
                    if (! n2.isElement()) {
                        continue;
                    }
                    KoXmlElement e2 = n2.toElement();
                    if (e2.tagName() != QStringLiteral("node")) {
                        continue;
                    }
                    QString s = e2.attribute(QStringLiteral("id"));
                    Node *node = status.project().findNode(s);
                    if (node) {
                        lst.append(node);
                    } else {
                        errorPlanXml<<"Failed to find node id="<<s;
                    }
                }
                ms->m_pathlists.append(lst);
            }
            ms->criticalPathListCached = true;
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::loadNodeSchedule(NodeSchedule* schedule, const KoXmlElement &element, XMLLoaderObject& status)
{
    debugPlanXml<<"node-schedule";
    QString s;
    load(schedule, element, status);
    s = element.attribute(QStringLiteral("earlystart"));
    if (s.isEmpty()) { // try version < 0.6
        s = element.attribute(QStringLiteral("earlieststart"));
    }
    if (! s.isEmpty()) {
        schedule->earlyStart = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute(QStringLiteral("latefinish"));
    if (s.isEmpty()) { // try version < 0.6
        s = element.attribute(QStringLiteral("latestfinish"));
    }
    if (! s.isEmpty()) {
        schedule->lateFinish = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute(QStringLiteral("latestart"));
    if (! s.isEmpty()) {
        schedule->lateStart = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute(QStringLiteral("earlyfinish"));
    if (! s.isEmpty()) {
        schedule->earlyFinish = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute(QStringLiteral("start"));
    if (! s.isEmpty())
        schedule->startTime = DateTime::fromString(s, status.projectTimeZone());
    s = element.attribute(QStringLiteral("end"));
    if (!s.isEmpty())
        schedule->endTime = DateTime::fromString(s, status.projectTimeZone());
    s = element.attribute(QStringLiteral("start-work"));
    if (!s.isEmpty())
        schedule->workStartTime = DateTime::fromString(s, status.projectTimeZone());
    s = element.attribute(QStringLiteral("end-work"));
    if (!s.isEmpty())
        schedule->workEndTime = DateTime::fromString(s, status.projectTimeZone());
    schedule->duration = Duration::fromString(element.attribute(QStringLiteral("duration")));

    schedule->inCriticalPath = element.attribute(QStringLiteral("in-critical-path"), QString::number(0)).toInt();
    schedule->resourceError = element.attribute(QStringLiteral("resource-error"), QString::number(0)).toInt();
    schedule->resourceOverbooked = element.attribute(QStringLiteral("resource-overbooked"), QString::number(0)).toInt();
    schedule->resourceNotAvailable = element.attribute(QStringLiteral("resource-not-available"), QString::number(0)).toInt();
    schedule->constraintError = element.attribute(QStringLiteral("scheduling-conflict"), QString::number(0)).toInt();
    schedule->notScheduled = element.attribute(QStringLiteral("not-scheduled"), QString::number(1)).toInt();

    schedule->positiveFloat = Duration::fromString(element.attribute(QStringLiteral("positive-float")));
    schedule->negativeFloat = Duration::fromString(element.attribute(QStringLiteral("negative-float")));
    schedule->freeFloat = Duration::fromString(element.attribute(QStringLiteral("free-float")));

    return true;
}

bool KPlatoXmlLoaderBase::load(WBSDefinition &def, const KoXmlElement &element, XMLLoaderObject &/*status*/)
{
    debugPlanXml<<"wbs-def";
    def.setProjectCode(element.attribute(QStringLiteral("project-code")));
    def.setProjectSeparator(element.attribute(QStringLiteral("project-separator")));
    def.setLevelsDefEnabled((bool)element.attribute(QStringLiteral("levels-enabled"), QString::number(0)).toInt());
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("default")) {
            def.defaultDef().code = e.attribute(QStringLiteral("code"), QStringLiteral("Number"));
            def.defaultDef().separator = e.attribute(QStringLiteral("separator"), QStringLiteral("."));
        } else if (e.tagName() == QStringLiteral("levels")) {
            KoXmlNode n = e.firstChild();
            for (; ! n.isNull(); n = n.nextSibling()) {
                if (! n.isElement()) {
                    continue;
                }
                KoXmlElement el = n.toElement();
                WBSDefinition::CodeDef d;
                d.code = el.attribute(QStringLiteral("code"));
                d.separator = el.attribute(QStringLiteral("separator"));
                int lvl = el.attribute(QStringLiteral("level"), QString::number(-1)).toInt();
                if (lvl >= 0) {
                    def.setLevelsDef(lvl, d);
                } else errorPlanXml<<"Invalid levels definition";
            }
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(Documents &documents, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"documents";
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("document")) {
            Document *doc = new Document();
            if (! load(doc, e, status)) {
                warnPlanXml<<"Failed to load document";
                status.addMsg(XMLLoaderObject::Errors, QStringLiteral("Failed to load document"));
                delete doc;
            } else {
                documents.addDocument(doc);
                status.addMsg(i18n("Document loaded, URL=%1",  doc->url().url()));
            }
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(Document *document, const KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlanXml<<"document";
    Q_UNUSED(status);
    document->setUrl(QUrl(element.attribute(QStringLiteral("url"))));
    document->setType((Document::Type)(element.attribute(QStringLiteral("type")).toInt()));
    document->setStatus(element.attribute(QStringLiteral("status")));
    document->setSendAs((Document::SendAs)(element.attribute(QStringLiteral("sendas")).toInt()));
    return true;
}

bool KPlatoXmlLoaderBase::load(Estimate* estimate, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"estimate";
    estimate->setType(element.attribute(QStringLiteral("type")));
    estimate->setRisktype(element.attribute(QStringLiteral("risk")));
    if (status.version() <= QStringLiteral("0.6")) {
        estimate->setUnit((Duration::Unit)(element.attribute(QStringLiteral("display-unit"), QString().number(Duration::Unit_h)).toInt()));
        QList<qint64> s = status.project().standardWorktime()->scales();
        estimate->setExpectedEstimate(Estimate::scale(Duration::fromString(element.attribute(QStringLiteral("expected"))), estimate->unit(), s));
        estimate->setOptimisticEstimate(Estimate::scale(Duration::fromString(element.attribute(QStringLiteral("optimistic"))), estimate->unit(), s));
        estimate->setPessimisticEstimate(Estimate::scale(Duration::fromString(element.attribute(QStringLiteral("pessimistic"))), estimate->unit(), s));
    } else {
        if (status.version() <= QStringLiteral("0.6.2")) {
            // 0 pos in unit is now Unit_Y, so add 3 to get the correct new unit
            estimate->setUnit((Duration::Unit)(element.attribute(QStringLiteral("unit"), QString().number(Duration::Unit_ms - 3)).toInt() + 3));
        } else {
            estimate->setUnit(Duration::unitFromString(element.attribute(QStringLiteral("unit"))));
        }
        estimate->setExpectedEstimate(element.attribute(QStringLiteral("expected"), QString::number(0.0)).toDouble());
        estimate->setOptimisticEstimate(element.attribute(QStringLiteral("optimistic"), QString::number(0.0)).toDouble());
        estimate->setPessimisticEstimate(element.attribute(QStringLiteral("pessimistic"), QString::number(0.0)).toDouble());

        estimate->setCalendar(status.project().findCalendar(element.attribute(QStringLiteral("calendar-id"))));
    }
    return true;
}

#if 0
bool KPlatoXmlLoaderBase::load(ResourceGroupRequest* gr, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"resourcegroup-request";
    gr->setGroup(status.project().findResourceGroup(element.attribute(QStringLiteral("group-id")));
    if (gr->group() == 0) {
        errorPlanXml<<"The referenced resource group does not exist: group id="<<element.attribute(QStringLiteral("group-id");
        return false;
    }
    gr->group()->registerRequest(gr);
    Q_ASSERT(gr->parent());

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("resource-request") {
            ResourceRequest *r = new ResourceRequest();
            if (load(r, e, status)) {
                gr->parent()->addResourceRequest(r, gr);
            } else {
                errorPlanXml<<"Failed to load resource request";
                delete r;
            }
        }
    }
    // meaning of m_units changed
    int x = element.attribute(QStringLiteral("units").toInt() -gr->count();
    gr->setUnits(x > 0 ? x : 0);

    return true;
}
#endif
bool KPlatoXmlLoaderBase::load(ResourceRequest *rr, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"resource-request";
    rr->setResource(status.project().resource(element.attribute(QStringLiteral("resource-id"))));
    if (rr->resource() == nullptr) {
        warnPlanXml<<"The referenced resource does not exist: resource id="<<element.attribute(QStringLiteral("resource-id"));
        return false;
    }
    rr->setUnits(element.attribute(QStringLiteral("units")).toInt());

    KoXmlElement parent = element.namedItem("required-resources").toElement();
    KoXmlElement e;
    QList<Resource*> required;
    forEachElement(e, parent) {
        if (e.nodeName() == QStringLiteral("resource")) {
            QString id = e.attribute(QStringLiteral("id"));
            if (id.isEmpty()) {
                errorPlanXml<<"Missing project id";
                continue;
            }
            Resource *r = status.project().resource(id);
            if (r == nullptr) {
                warnPlanXml<<"The referenced resource does not exist: resource id="<<element.attribute(QStringLiteral("resource-id"));
            } else {
                if (r != rr->resource()) {
                    required << r;
                }
            }
        }
    }
    rr->setRequiredResources(required);
    return true;

}

bool KPlatoXmlLoaderBase::load(WorkPackage &wp, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"workpackage";
    Q_UNUSED(status);
    wp.setOwnerName(element.attribute(QStringLiteral("owner")));
    wp.setOwnerId(element.attribute(QStringLiteral("owner-id")));
    return true;
}

bool KPlatoXmlLoaderBase::loadWpLog(WorkPackage *wp, KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"wplog";
    wp->setOwnerName(element.attribute(QStringLiteral("owner")));
    wp->setOwnerId(element.attribute(QStringLiteral("owner-id")));
    wp->setTransmitionStatus(wp->transmitionStatusFromString(element.attribute(QStringLiteral("status"))));
    wp->setTransmitionTime(DateTime(QDateTime::fromString(element.attribute(QStringLiteral("time")), Qt::ISODate)));
    return load(wp->completion(), element, status);
}

bool KPlatoXmlLoaderBase::load(Completion &completion, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"completion";
    QString s;
    completion.setStarted((bool)element.attribute(QStringLiteral("started"), QString::number(0)).toInt());
    completion.setFinished((bool)element.attribute(QStringLiteral("finished"), QString::number(0)).toInt());
    s = element.attribute(QStringLiteral("startTime"));
    if (!s.isEmpty()) {
        completion.setStartTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    s = element.attribute(QStringLiteral("finishTime"));
    if (!s.isEmpty()) {
        completion.setFinishTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    completion.setEntrymode(element.attribute(QStringLiteral("entrymode")));
    if (status.version() < QStringLiteral("0.6")) {
        if (completion.isStarted()) {
            Completion::Entry *entry = new Completion::Entry(element.attribute(QStringLiteral("percent-finished"), QString::number(0)).toInt(), Duration::fromString(element.attribute(QStringLiteral("remaining-effort"))),  Duration::fromString(element.attribute(QStringLiteral("performed-effort"))));
            entry->note = element.attribute(QStringLiteral("note"));
            QDate date = completion.startTime().date();
            if (completion.isFinished()) {
                date = completion.finishTime().date();
            }
            // almost the best we can do ;)
            completion.addEntry(date, entry);
        }
    } else {
        KoXmlElement e;
        forEachElement(e, element) {
                if (e.tagName() == QStringLiteral("completion-entry")) {
                    QDate date;
                    s = e.attribute(QStringLiteral("date"));
                    if (!s.isEmpty()) {
                        date = QDate::fromString(s, Qt::ISODate);
                    }
                    if (!date.isValid()) {
                        warnPlanXml<<"Invalid date: "<<date<<s;
                        continue;
                    }
                    Completion::Entry *entry = new Completion::Entry(e.attribute(QStringLiteral("percent-finished"), QString::number(0)).toInt(), Duration::fromString(e.attribute(QStringLiteral("remaining-effort"))),  Duration::fromString(e.attribute(QStringLiteral("performed-effort"))));
                    completion.addEntry(date, entry);
                } else if (e.tagName() == QStringLiteral("used-effort")) {
                    KoXmlElement el;
                    forEachElement(el, e) {
                            if (el.tagName() == QStringLiteral("resource")) {
                                QString id = el.attribute(QStringLiteral("id"));
                                Resource *r = status.project().resource(id);
                                if (r == nullptr) {
                                    warnPlanXml<<"Cannot find resource, id="<<id;
                                    continue;
                                }
                                Completion::UsedEffort *ue = new Completion::UsedEffort();
                                completion.addUsedEffort(r, ue);
                                load(ue, el, status);
                            }
                    }
                }
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(Completion::UsedEffort* ue, const KoXmlElement& element, XMLLoaderObject& /*status*/)
{
    debugPlanXml<<"used-effort";
    KoXmlElement e;
    forEachElement(e, element) {
        if (e.tagName() == QStringLiteral("actual-effort")) {
            QDate date = QDate::fromString(e.attribute(QStringLiteral("date")), Qt::ISODate);
            if (date.isValid()) {
                Completion::UsedEffort::ActualEffort a;
                a.setNormalEffort(Duration::fromString(e.attribute(QStringLiteral("normal-effort"))));
                a.setOvertimeEffort(Duration::fromString(e.attribute(QStringLiteral("overtime-effort"))));
                ue->setEffort(date, a);
            }
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(Appointment *appointment, const KoXmlElement& element, XMLLoaderObject& status, Schedule &sch)
{
    debugPlanXml<<"appointment";
    Node *node = status.project().findNode(element.attribute(QStringLiteral("task-id")));
    if (node == nullptr) {
        errorPlanXml<<"The referenced task does not exists: "<<element.attribute(QStringLiteral("task-id"));
        return false;
    }
    Resource *res = status.project().resource(element.attribute(QStringLiteral("resource-id")));
    if (res == nullptr) {
        errorPlanXml<<"The referenced resource does not exists: resource id="<<element.attribute(QStringLiteral("resource-id"));
        return false;
    }
    if (!res->addAppointment(appointment, sch)) {
        errorPlanXml<<"Failed to add appointment to resource: "<<res->name();
        return false;
    }
    if (! node->addAppointment(appointment, sch)) {
        errorPlanXml<<"Failed to add appointment to node: "<<node->name();
        appointment->resource()->takeAppointment(appointment);
        return false;
    }
    //debugPlanXml<<"res="<<m_resource<<" node="<<m_node;
    AppointmentIntervalList lst = appointment->intervals();
    load(lst, element, status);
    if (lst.isEmpty()) {
        errorPlanXml<<"Appointment interval list is empty (added anyway): "<<node->name()<<res->name();
        return false;
    }
    appointment->setIntervals(lst);
    return true;
}

bool KPlatoXmlLoaderBase::load(AppointmentIntervalList& lst, const KoXmlElement& element, XMLLoaderObject& status)
{
    debugPlanXml<<"appointment-interval-list";
    KoXmlElement e;
    forEachElement(e, element) {
        if (e.tagName() == QStringLiteral("interval")) {
            AppointmentInterval a;
            if (load(a, e, status)) {
                lst.add(a);
            } else {
                errorPlanXml<<"Could not load interval";
            }
        }
    }
    return true;
}

bool KPlatoXmlLoaderBase::load(AppointmentInterval& interval, const KoXmlElement& element, XMLLoaderObject& status)
{
    bool ok;
    QString s = element.attribute(QStringLiteral("start"));
    if (!s.isEmpty()) {
        interval.setStartTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    s = element.attribute(QStringLiteral("end"));
    if (!s.isEmpty()) {
        interval.setEndTime(DateTime::fromString(s, status.projectTimeZone()));
    }
    double l = element.attribute(QStringLiteral("load"), QString::number(100)).toDouble(&ok);
    if (ok) {
        interval.setLoad(l);
    }
    debugPlanXml<<"interval:"<<interval;
    return interval.isValid();
}
