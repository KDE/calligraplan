/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "plannerimport.h"

#include "kptproject.h"
#include <kpttask.h>
#include <kptnode.h>
#include <kptresource.h>
#include <kptdocuments.h>
#include <kptrelation.h>
#include <kptduration.h>
#include <kptcalendar.h>
#include <kptappointment.h>

#include <QByteArray>
#include <QString>
#include <QTextStream>
#include <QFile>
#include <QDomElement>
#include <QDebug>

#include <KPluginFactory>

#include <KoFilterChain.h>
#include <KoFilterManager.h>
#include <KoDocument.h>


using namespace KPlato;

#define PLANNERIMPORT_LOG "calligra.plan.filter.planner.import"
#define debugPlannerImport qCDebug(QLoggingCategory(PLANNERIMPORT_LOG))<<Q_FUNC_INFO
#define warnPlannerImport qCWarning(QLoggingCategory(PLANNERIMPORT_LOG))
#define errorPlannerImport qCCritical(QLoggingCategory(PLANNERIMPORT_LOG))

#define forEachChildElementWithTag(elem, parent, tag) \
for (QDomNode _node = parent.firstChild(); !_node.isNull(); _node = _node.nextSibling()) \
    if ((elem = _node.toElement()).isNull() || elem.tagName() != tag) {} else

#define forEachChildElement(elem, parent) \
for (QDomNode _node = parent.firstChild(); !_node.isNull(); _node = _node.nextSibling()) \
    if ((elem = _node.toElement()).isNull()) {} else

#define forEachElementInList(elem, list) \
for (int i = 0; i < list.count(); ++i) \
    if ((elem = list.item(i).toElement()).isNull()) {} else
            
K_PLUGIN_FACTORY_WITH_JSON(KPlatoImportFactory, "plan_planner_import.json",
                           registerPlugin<PlannerImport>();)

PlannerImport::PlannerImport(QObject* parent, const QVariantList &)
    : KoFilter(parent)
{
}

KoFilter::ConversionStatus PlannerImport::convert(const QByteArray& from, const QByteArray& to)
{
    debugPlannerImport << from << to;
    if ((from != "application/x-planner") || (to != "application/x-vnd.kde.plan")) {
        return KoFilter::NotImplemented;
    }
    QFile in(m_chain->inputFile());
    if (!in.open(QIODevice::ReadOnly)) {
        errorPlannerImport << "Unable to open input file!";
        in.close();
        return KoFilter::FileNotFound;
    }
    QDomDocument inDoc;
    if (!inDoc.setContent(&in)) {
        errorPlannerImport << "Invalid format in input file!";
        in.close();
        return KoFilter::InvalidFormat;
    }

    KoDocument *part = nullptr;
    bool batch = false;
    if (m_chain->manager()) {
        batch = m_chain->manager()->getBatchMode();
    }
    if (batch) {
        //TODO
        debugPlannerImport << "batch";
    } else {
        //debugPlannerImport<<"online";
        part = m_chain->outputDocument();
    }
    if (part == nullptr || part->project() == nullptr) {
        errorPlannerImport << "Cannot open document";
        return KoFilter::InternalError;
    }
    if (!loadPlanner(inDoc, part)) {
        return KoFilter::ParsingError;
    }

    return KoFilter::OK;
}

DateTime toDateTime(const QString &dts)
{
    // NOTE: time ends in Z, so should be UTC, but it seems it is in local time anyway.
    //       Atm, just ignore timezone
    const QString format = QStringLiteral("yyyyMMddThhmmssZ");
    return DateTime(QDateTime::fromString(dts, format));
}

//<project name="Test Planner project" company="HEJ" manager="P.Manager" phase="" project-start="20190503T000000Z" mrproject-version="2" calendar="1">
bool loadProject(const QDomElement &el, Project &project)
{
    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Planner"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    sm->setAllowOverbooking(true);
    sm->expected()->setScheduled(true);
    project.setCurrentSchedule(sm->scheduleId());
    
    project.setName(el.attribute(QStringLiteral("name")));
    project.setLeader(el.attribute(QStringLiteral("manager")));
    DateTime dt = toDateTime(el.attribute(QStringLiteral("project-start")));
    if (dt.isValid()) {
        project.setConstraintStartTime(dt);
        project.setStartTime(dt);
    }
    if (el.hasAttribute(QStringLiteral("calendar"))) {
        Calendar *c = new Calendar();
        c->setId(el.attribute(QStringLiteral("calendar")));
        project.addCalendar(c);
        project.setDefaultCalendar(c);
        debugPlannerImport<<"Added default calendar:"<<c;
    }
    return true;
}

// <calendars>
//   <day-types>
//     <day-type id="0" name="Arbejdstid" description="En normal arbejdsdag"/>
//     <day-type id="1" name="Ikke-arbejdstid" description="En normal fridag"/>
//     <day-type id="2" name="Brug grundkalender" description="Brug dag fra basiskalender"/>
//     <day-type id="3" name="Tandl&#xE6;ge" description=""/>
//     <day-type id="4" name="Vacation" description=""/>
//   </day-types>
//   <calendar id="1" name="Top">
//     <default-week/>
//     <overridden-day-types>
//       <overridden-day-type id="0">
//         interval start="0900" end="1700"/>
//       /overridden-day-type>
//     /overridden-day-types>
//     days/>
//   /calendar>
//   calendar id="2" name="Standard">
//     default-week mon="0" tue="0" wed="0" thu="0" fri="0" sat="1" sun="1"/>
//     overridden-day-types>
//       overridden-day-type id="0">
//         interval start="0800" end="1200"/>
//         interval start="1300" end="1700"/>
//       /overridden-day-type>
//     /overridden-day-types>
//     days/>
//     calendar id="3" name="Min kalender">
//       <default-week mon="2" tue="2" wed="2" thu="2" fri="2" sat="2" sun="2"/>
//       overridden-day-types>
//         overridden-day-type id="3">
//           interval start="0800" end="1600"/>
//         /overridden-day-type>
//       /overridden-day-types>
//       days>
//         day date="20190506" type="day-type" id="3"/>
//       /days>
//     /calendar>
//   /calendar>
// </calendars>
CalendarDay::State toDayState(int type)
{
    
    QList<CalendarDay::State> state = QList<CalendarDay::State>() << CalendarDay::Working << CalendarDay::NonWorking;
    if (type < state.count()) {
        return state.value(type);
    }
    return CalendarDay::Undefined;
}
bool loadWeek(const QDomElement &el, Calendar *calendar)
{
    debugPlannerImport<<calendar->name();
    QList<int> defaultWeek = QList<int>() << 2 << 2 << 2 << 2 << 2 << 2 << 2;
    QDomElement wel;
    forEachChildElementWithTag(wel, el, QStringLiteral("default-week")) {
        defaultWeek[0] = wel.attribute(QStringLiteral("mon"), QString::number(2)).toInt();
        defaultWeek[1] = wel.attribute(QStringLiteral("tue"), QString::number(2)).toInt();
        defaultWeek[2] = wel.attribute(QStringLiteral("wed"), QString::number(2)).toInt();
        defaultWeek[3] = wel.attribute(QStringLiteral("thu"), QString::number(2)).toInt();
        defaultWeek[4] = wel.attribute(QStringLiteral("fri"), QString::number(2)).toInt();
        defaultWeek[5] = wel.attribute(QStringLiteral("sat"), QString::number(2)).toInt();
        defaultWeek[6] = wel.attribute(QStringLiteral("sun"), QString::number(2)).toInt();
    }
    debugPlannerImport<<defaultWeek;
    for (int i = 0; i < defaultWeek.count(); ++i) {
        CalendarDay *day = calendar->weekday(i+1);
        day->setState(toDayState(defaultWeek.at(i)));
    }
    forEachChildElementWithTag(wel, el, QStringLiteral("overridden-day-types")) {
        QDomElement oel;
        forEachChildElementWithTag(oel, wel, QStringLiteral("overridden-day-type")) {
            if (oel.hasAttribute(QStringLiteral("id"))) {
                int id = oel.attribute(QStringLiteral("id")).toInt();
                if (!defaultWeek.contains(id)) {
                    continue;
                }
                for (int i = 0; i < defaultWeek.count(); ++i) {
                    if (defaultWeek.at(i) != id) {
                        continue;
                    }
                    CalendarDay *day = calendar->weekday(i+1);
                    day->setState(CalendarDay::Working);
                    QDomElement iel;
                    forEachChildElementWithTag(iel, oel, QStringLiteral("interval")) {
                        QTime start = QTime::fromString(iel.attribute(QStringLiteral("start")), QStringLiteral("hhmm"));
                        QTime end = QTime::fromString(iel.attribute(QStringLiteral("end")), QStringLiteral("hhmm"));
                        day->addInterval(TimeInterval(start, start.msecsTo(end)));
                        debugPlannerImport<<"Overridden:"<<id<<"weekday="<<i+1<<iel.attribute(QStringLiteral("start"))<<"->"<<start<<':'<<iel.attribute(QStringLiteral("end"))<<end;
                    }
                }
            }
        }
    }
    debugPlannerImport<<calendar;
    return true;
}
bool loadDays(const QDomElement &el, Calendar *calendar)
{
    QDomNodeList lst = el.elementsByTagName(QStringLiteral("day"));
    QDomElement cel;
    forEachElementInList(cel, lst) {
        QDate date = QDate::fromString(cel.attribute(QStringLiteral("date")), QStringLiteral("yyyyMMdd"));
        if (!date.isValid()) {
            continue;
        }
        int type = cel.attribute(QStringLiteral("day-type"), QString::number(2)).toInt();
        CalendarDay *day = new CalendarDay(date, toDayState(type));
        QDomNodeList lst = cel.elementsByTagName(QStringLiteral("interval"));
        QDomElement iel;
        forEachElementInList(iel, lst) {
            QTime start = QTime::fromString(iel.attribute(QStringLiteral("start")), QStringLiteral("hh:mm"));
            QTime end = QTime::fromString(iel.attribute(QStringLiteral("end")), QStringLiteral("hh:mm"));
            day->addInterval(TimeInterval(start, start.msecsTo(end)));
        }
        calendar->addDay(day);
    }
    return true;
}
bool loadCalendars(const QDomElement &el, Project &project, Calendar *parent = nullptr)
{
    QDomElement cel;
    forEachChildElementWithTag(cel, el, QStringLiteral("calendar")) {
        QString id = cel.attribute(QStringLiteral("id"));
        Calendar *calendar = project.findCalendar(id);
        if (!calendar) {
            calendar = new Calendar();
            calendar->setId(cel.attribute(QStringLiteral("id")));
            project.addCalendar(calendar, parent);
            debugPlannerImport<<"Loading new calendar"<<calendar->id();
        } else debugPlannerImport<<"Loading default calendar"<<calendar->id();
        calendar->setName(cel.attribute(QStringLiteral("name")));
        loadWeek(cel, calendar);
        loadDays(cel, calendar);

        loadCalendars(cel, project, calendar);
    }
    return true;
}

// <resource-groups>
// <group id="1" name="G1" admin-name="leader" admin-phone="12345678" admin-email="leader@here"/>
// </resource-groups>
bool loadResourceGroups(const QDomElement &el, Project &project)
{
    QDomNodeList lst = el.elementsByTagName(QStringLiteral("group"));
    QDomElement gel;
    forEachElementInList(gel, lst) {
        ResourceGroup *g = new ResourceGroup();
        g->setId(gel.attribute(QStringLiteral("id")));
        g->setName(gel.attribute(QStringLiteral("name")));
        g->setCoordinator(gel.attribute(QStringLiteral("admin-name")));
        project.addResourceGroup(g);
    }
    return true;
}

// <resources>
// <resource group="1" id="1" name="R1" short-name="" type="1" units="0" email="r1@there" note="" std-rate="101"/>
// </resources>
Resource::Type toResourceType(const QString &type)
{
    QMap<QString, Resource::Type> types;
    types[QStringLiteral("0")] = Resource::Type_Material;
    types[QStringLiteral("1")] = Resource::Type_Work;
    return types.contains(type) ? types[type] : Resource::Type_Work;
}

bool loadResources(const QDomElement &el, Project &project)
{
    QDomNodeList lst = el.elementsByTagName(QStringLiteral("resource"));
    QDomElement rel;
    forEachElementInList(rel, lst) {
        Resource *r = new Resource();
        r->setId(rel.attribute(QStringLiteral("id")));
        r->setName(rel.attribute(QStringLiteral("name")));
        r->setInitials(rel.attribute(QStringLiteral("short-name")));
        r->setEmail(rel.attribute(QStringLiteral("email")));
        r->setType(toResourceType(rel.attribute(QStringLiteral("type"))));
        int units = rel.attribute(QStringLiteral("units"), QString::number(0)).toInt();
        if (units == 0) {
            // atm. planner saves 0 but assumes 100%
            units = 100;
        }
        r->setUnits(units);
        r->setNormalRate(rel.attribute(QStringLiteral("std-rate")).toDouble());
        r->setCalendar(project.findCalendar(rel.attribute(QStringLiteral("calendar"))));
        project.addResource(r);
        QString gid = rel.attribute(QStringLiteral("group"));
        ResourceGroup *g = project.group(gid);
        if (g) {
            g->addResource(-1, r, nullptr);
        }
    }
    return true;
}

Estimate::Type toEstimateType(const QString type)
{
    Estimate::Type res = Estimate::Type_Effort;
    if (type == QStringLiteral("fixed-work")) {
        res = Estimate::Type_Effort;
    } else if (type == QStringLiteral("fixed-duration")) {
        res = Estimate::Type_Duration;
    }
    return res;
}

Node::ConstraintType toConstraintType(const QString &type)
{
    Node::ConstraintType res = Node::ASAP;
    if (type == QStringLiteral("must-start-on")) {
        res = Node::MustStartOn;
    } else if (type == QStringLiteral("start-no-earlier-than")) {
        res = Node::StartNotEarlier;
    }
    return res;
}
bool loadConstraint(const QDomElement &el, Task *t)
{
    QDomElement cel;
    forEachChildElementWithTag(cel, el, QStringLiteral("constraint")) {
        t->setConstraint(toConstraintType(cel.attribute(QStringLiteral("type"))));
        t->setConstraintStartTime(toDateTime(cel.attribute(QStringLiteral("time"))));
    }
    return true;
}
// <task id="1" name="T1" note="" work="57600" start="20190503T000000Z" end="20190507T170000Z" work-start="20190503T080000Z" percent-complete="0" priority="0" type="normal" scheduling="fixed-work">
bool loadTasks(const QDomElement &el, Project &project, Node *parent = nullptr)
{
    QDomElement cel;
    forEachChildElementWithTag(cel, el, QStringLiteral("task")) {
        Task *t = project.createTask();
        t->setId(cel.attribute(QStringLiteral("id"), t->id()));
        t->setName(cel.attribute(QStringLiteral("name")));
        t->setDescription(cel.attribute(QStringLiteral("note")));
        loadConstraint(cel, t);
        t->estimate()->setType(toEstimateType(cel.attribute(QStringLiteral("scheduling"))));
        t->estimate()->setExpectedEstimate(Duration(cel.attribute(QStringLiteral("work"), QString::number(0)).toDouble(), Duration::Unit_s).toDouble());

        project.addSubTask(t, parent);
        long sid = project.scheduleManagers().constFirst()->scheduleId();
        NodeSchedule *sch = new NodeSchedule();
        sch->setId(sid);
        sch->setNode(t);
        t->addSchedule(sch);
        sch->setParent(t->parentNode()->currentSchedule());
        t->setCurrentSchedule(sid);

        const QString format = QStringLiteral("yyyyMMddThhmmssZ");
        QDateTime start;
        if (cel.hasAttribute(QStringLiteral("work-start"))) {
            start = QDateTime::fromString(cel.attribute(QStringLiteral("work-start")), format);
        } else {
            start = QDateTime::fromString(cel.attribute(QStringLiteral("start")), format);
        }
        QDateTime end = QDateTime::fromString(cel.attribute(QStringLiteral("end")), format);
        t->setStartTime(DateTime(start));
        t->setEndTime(DateTime(end));
        sch->setScheduled(true);

        debugPlannerImport<<"Loaded:"<<t<<"parent:"<<parent;

        loadTasks(cel, project, t);
    }
    return true;
}

Relation::Type toRelation(const QString &type)
{
    QMap<QString, Relation::Type> types;
    types[QStringLiteral("FS")] = Relation::FinishStart;
    types[QStringLiteral("FF")] = Relation::FinishFinish;
    types[QStringLiteral("SS")] = Relation::StartStart;
    types[QStringLiteral("SF")] = Relation::FinishStart; // not supported, use default

    return types.value(type);
}

// <predecessors>
// <predecessor id="1" predecessor-id="2" type="FS" lag="86400"/>
// </predecessors>
bool loadDependencies(const QDomElement &el, Project &project)
{
    QDomElement cel;
    forEachChildElementWithTag(cel, el, QStringLiteral("task")) {
        QString succid = cel.attribute(QStringLiteral("id"));
        Node *child = project.findNode(succid);
        if (!child) {
            warnPlannerImport<<"Task"<<succid<<"not found";
            continue;
        }
        QDomElement pels;
        forEachChildElementWithTag(pels, cel, QStringLiteral("predecessors")) {
            QDomElement pel;
            forEachChildElementWithTag(pel, pels, QStringLiteral("predecessor")) {
                QString predid = pel.attribute(QStringLiteral("predecessor-id"));
                Node *parent = project.findNode(predid);
                if (!parent) {
                    continue;
                }
                Duration lag(pel.attribute(QStringLiteral("lag"), QString::number(0)).toDouble(), Duration::Unit_s);
                Relation *rel = new Relation(parent, child, toRelation(pel.attribute(QStringLiteral("type"))), lag);
                if (!project.addRelation(rel)) {
                    warnPlannerImport<<"Could not add relation:"<<rel;
                    delete rel;
                } else debugPlannerImport<<"added:"<<rel;
            }
        }
        loadDependencies(cel, project);
    }
    return true;
}

//<allocation task-id="3" resource-id="1" units="100"/>
bool loadAllocations(const QDomElement &el, Project &project)
{
    QDomNodeList lst = el.elementsByTagName(QStringLiteral("allocation"));
    QDomElement pel;
    forEachElementInList(pel, lst) {
        Task *t = dynamic_cast<Task*>(project.findNode(pel.attribute(QStringLiteral("task-id"))));
        Resource *r = project.findResource(pel.attribute(QStringLiteral("resource-id")));
        if (!t || !r) {
            warnPlannerImport<<"Could not find task/resource:"<<t<<r;
            continue;
        }
        ResourceRequest *rr = new ResourceRequest(r);
        rr->setUnits(pel.attribute(QStringLiteral("units")).toInt());
        t->requests().addResourceRequest(rr);

        // do assignments
        Calendar *calendar = r->calendar();
        if (!calendar) {
            warnPlannerImport<<"No resource calendar:"<<r;
            continue;
        }
        Schedule *ts = t->currentSchedule();
        Schedule *rs = r->schedule(ts->id());
        if (!rs) {
            rs = r->createSchedule(t->name(), t->type(), ts->id());
        }
        r->setCurrentSchedulePtr(rs);
        AppointmentIntervalList apps = calendar->workIntervals(t->startTime(), t->endTime(), rr->units());
        const auto intervals = apps.map().values();
        for (const AppointmentInterval &a : intervals) {
            r->addAppointment(ts, a.startTime(), a.endTime(), a.load());
        }
        rs->setScheduled(true);
        debugPlannerImport<<"Assignments:"<<r<<':'<<r->appointmentIntervals().intervals();
    }
    return true;
}

bool PlannerImport::loadPlanner(const QDomDocument &in, KoDocument *doc) const
{
    QDomElement pel = in.documentElement();
    if (pel.tagName() != QStringLiteral("project")) {
        errorPlannerImport << "Missing project element";
        return false;
    }
    Project &project = *doc->project();
    if (!loadProject(pel, project)) {
        return false;
    }
    QDomElement el = pel.elementsByTagName(QStringLiteral("calendars")).item(0).toElement();
    if (el.isNull()) {
        debugPlannerImport << "No calendars element";
    }
    loadCalendars(el, project);

    el = pel.elementsByTagName(QStringLiteral("resource-groups")).item(0).toElement();
    if (el.isNull()) {
        debugPlannerImport << "No resource-groups element";
    }
    loadResourceGroups(el, project);

    el = pel.elementsByTagName(QStringLiteral("resources")).item(0).toElement();
    if (el.isNull()) {
        debugPlannerImport << "No resources element";
    }
    loadResources(el, project);

    el = pel.elementsByTagName(QStringLiteral("tasks")).item(0).toElement();
    if (el.isNull()) {
        debugPlannerImport << "No tasks element";
    } else {
        loadTasks(el, project);
        loadDependencies(el, project);
    }
    loadAllocations(pel, project);

    const auto nodes = project.allNodes();
    for (const Node *n : nodes) {
        if (n->endTime() > project.endTime()) {
            project.setEndTime(n->endTime());
        }
    }
    return true;
}

#include "plannerimport.moc"
