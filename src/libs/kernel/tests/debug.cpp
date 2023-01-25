/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2009, 2010 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptappointment.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptproject.h"
#include "kptresource.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptschedule.h"

#include <QTest>
#include <QStringList>
#include <QString>


namespace QTest
{
    template<>
    char *toString(const KPlato::DateTime &dt)
    {
        QString s;
        switch (dt.timeSpec()) {
            case Qt::LocalTime: s = QStringLiteral(" LocalTime"); break;
            case Qt::UTC: s = QStringLiteral(" UTC"); break;
            case Qt::OffsetFromUTC: s = QStringLiteral(" OffsetFromUTC"); break;
            case Qt::TimeZone: s = QStringLiteral(" TimeZone (%1)").arg(QLatin1String(dt.timeZone().id())); break;
        }
        return toString(QStringLiteral("%1T%2 %3").arg(dt.date().toString(Qt::ISODate), dt.time().toString(QStringLiteral("hh:mm:ss.zzz")), s));
    }

    template<>
    char *toString(const KPlato::Duration &d)
    {
        return toString(d.toString());
    }
}

namespace KPlato
{

class Debug
{
public:
    Debug() {}
static
void print(Calendar *calendar, const QString &str, bool full = true) {
    Q_UNUSED(full);
    QTimeZone tz = calendar->timeZone();
    QString s = tz.isValid() ? QString::fromLatin1(tz.id()) : QStringLiteral("LocalTime");

    qDebug()<<"Debug info: Calendar"<<calendar->name()<<s<<str;
    if (full) {
        for (int wd = 1; wd <= 7; ++wd) {
            CalendarDay *d = calendar->weekday(wd);
            qDebug()<<QStringLiteral("    ")<<wd<<':'<<d->stateToString(d->state());
            const auto intervals = d->timeIntervals();
            for (TimeInterval *t : intervals) {
                qDebug()<<"      interval:"<<t->first<<t->second<<'('<<Duration(qint64(t->second)).toString()<<')';
            }
        }
        const auto days = calendar->days();
        for (const CalendarDay *d : days) {
            qDebug()<<QStringLiteral("    ")<<d->date()<<':';
            const auto intervals = d->timeIntervals();
            for (TimeInterval *t : intervals) {
                qDebug()<<"      interval:"<<t->first<<t->second;
            }
        }
    }
    for (const auto c : calendar->calendars()) {
        qDebug()<<"  Child calendar:";
        print(c, "", full);
    }
}
static
QString printAvailable(Resource *r, const QString &lead = QString()) {
    QStringList sl;
    sl<<lead<<QStringLiteral("Available:")
        <<(r->availableFrom().isValid()
                ? r->availableFrom().toString()
                : (r->project() ? (QStringLiteral("(%1)").arg(r->project()->constraintStartTime().toString())) : QString()))
        <<(r->availableUntil().isValid()
                ? r->availableUntil().toString()
                : (r->project() ? (QStringLiteral("(%1)").arg(r->project()->constraintEndTime().toString())) : QString()))
        <<(QString::number(r->units())+QStringLiteral("%"))
        <<QStringLiteral("cost: normal")<<QString::number(r->normalRate())<<QStringLiteral(" overtime")<<QString::number(r->overtimeRate());
    return sl.join(QLatin1Char(' '));
}
static
void print(ResourceGroup *group, bool full = true, QString indent = QString()) {
    qDebug()<<indent<<group->name()<<"id:"<<group->id()<<"parent:"<<group->parentGroup();
    auto in = indent;// +  QStringLiteral("  ");
    qDebug()<<in<<"Resources:"<<group->numResources();
    if (full) {
        in = indent +  QStringLiteral("    ");
        const auto resources = group->resources();
        for (Resource *r : resources) {
            qDebug()<<in<<r->name()<<r->id();
        }
    }
    qDebug()<<(indent+QStringLiteral("  "))<<"Resource groups:"<<group->numChildGroups();
    in = indent +  QStringLiteral("    ");
    const auto childGroups = group->childGroups();
    for (ResourceGroup *g : childGroups) {
        print(g, full, in);
    }
}
static
void print(ResourceGroup *group, const QString &str, bool full = true) {
    qDebug()<<"Debug info: Group"<<str;
    print(group, full, QStringLiteral("  "));
}

static
void print(Resource *r, const QString &str, bool full = true) {
    qDebug()<<"Debug info: Resource"<<r->name()<<(r->isShared()?"Shared":"Local")<<"id:"<<r->id()<<(void*)r<<str;
    qDebug()<<"  Parent groups:"<<r->parentGroups().count();
    const auto parentGroups = r->parentGroups();
    for (ResourceGroup *g : parentGroups) {
        qDebug()<<QStringLiteral("    ")<<g->name() + QStringLiteral(" Type: ") + g->type() << "id:"<<g->id();
    }
    qDebug()<<"  Available:"
        <<(r->availableFrom().isValid()
                ? r->availableFrom().toString()
                : (r->project() ? (QLatin1Char('(')+r->project()->constraintStartTime().toString()+QLatin1Char(')')) : QString()))
        <<(r->availableUntil().isValid()
                ? r->availableUntil().toString()
                : (r->project() ? (QLatin1Char('(')+r->project()->constraintEndTime().toString()+QLatin1Char(')')) : QString()))
        <<r->units()<<'%';
    qDebug()<<"  Type:"<<r->typeToString();
    if (r->type() == Resource::Type_Team) {
        qDebug()<<"  Team members:"<<r->teamMembers().count();
        const auto resources = r->teamMembers();
        for (Resource *tm : resources) {
            print(tm, QLatin1String());
//             qDebug()<<"     "<<tm->name()<<"Available:"
//                     <<(r->availableFrom().isValid()
//                             ? r->availableFrom().toString()
//                             : (r->project() ? ('('+r->project()->constraintStartTime().toString()+')') : QString()))
//                     <<(r->availableUntil().isValid()
//                             ? r->availableUntil().toString()
//                             : (r->project() ? ('('+r->project()->constraintEndTime().toString()+')') : QString()))
//                     <<r->units()<<'%';
        }
    } else {
        Calendar *cal = r->calendar(true);
        QString s;
        if (cal) {
            s = cal->name();
        } else {
            cal = r->calendar(false);
            if (cal) {
                s = cal->name() + QStringLiteral(" (Default)");
            } else {
                s = QStringLiteral("  No calendar");
            }
        }
        qDebug()<<"  Calendar:"<<s;
        if (cal) {
            print(cal, QStringLiteral("    Resource calendar"));
        }
    }
    const auto reqs = r->requiredResources();
    if (reqs.isEmpty()) {
        qDebug()<<"  No required resources";
    } else {
        qDebug()<<"  Required resources:"<<reqs.count()<<'('<<r->requiredIds().count()<<')';
        for (Resource *req : reqs) {
            qDebug()<<QStringLiteral("  ")<<req->name()<<req->type()<<req->id()<<(void*)req<<req->requiredIds().contains(req->id());
        }
    }
    if (! full) return;
    qDebug()<<"  External appointments:"<<r->numExternalAppointments();
    const auto appointments = r->externalAppointmentList();
    for (Appointment *a : appointments) {
        qDebug()<<"     appointment:"<<a->startTime().toString()<<a->endTime().toString();
        const auto intervals = a->intervals().map().values();
        for(const AppointmentInterval &i : intervals ) {
            qDebug()<<"        "<<i.startTime().toString()<<i.endTime().toString()<<i.load();
        }
    }
}
static
void print(Project *p, const QString &str, bool all = false) {
    qDebug()<<"Debug info: Project"<<p->name()<<str;
    qDebug()<<"project target start:"<<QTest::toString(QDateTime(p->constraintStartTime()));
    qDebug()<<"project target end  :"<<QTest::toString(QDateTime(p->constraintEndTime()));
    if (p->isScheduled()) {
        qDebug()<<"  project start time:"<<QTest::toString(QDateTime(p->startTime()));
        qDebug()<<"  project end time  :"<<QTest::toString(QDateTime(p->endTime()));
    } else {
        qDebug()<<"  Not scheduled";
    }
    qDebug()<<"  Accounts:"<<p->accounts().accountCount()<<"all:"<<p->accounts().allAccounts().count();
    qDebug()<<"  Calendars:"<<p->calendarCount()<<"all:"<<p->allCalendars().count();
    qDebug()<<"  Default calendar:"<<(p->defaultCalendar()?p->defaultCalendar()->name():QStringLiteral("None"));
    qDebug()<<"  Groups:"<<p->resourceGroups().count()<<"all:"<<p->allResourceGroups().count();
    qDebug()<<"  Resources:"<<p->resourceList().count();
    if (! all) {
        return;
    }
    for (const auto a : p->accounts().accountList()) {
        print(a, QStringLiteral("  "));
    }
    for (const auto c : p->calendars()) {
        print(c, "", true);
    }
    qDebug()<<QStringLiteral("  ")<<"Resource groups:"<<p->resourceGroupCount();
    const auto resourceGroups = p->resourceGroups();
    for (ResourceGroup *g : resourceGroups) {
        print(g, true, QStringLiteral("  "));
    }
    const auto resourceList = p->resourceList();
    if (resourceList.isEmpty()) {
        qDebug()<<"  No resources";
    } else {
        for (Resource *r : resourceList) {
            qDebug();
            print(r, QLatin1String(), true);
        }
    }
    for (int i = 0; i < p->numChildren(); ++i) {
        qDebug();
        print(static_cast<Task*>(p->childNode(i)), true);
    }
}
static
void print(Project *p, Task *t, const QString &str, bool full = true) {
    Q_UNUSED(full);
    print(p, str);
    print(t);
}
static
void print(Project *p, const char *str, bool all = false) {
    print(p, QLatin1String(str), all);
}

static
void print(Completion &c, const QString &pad, bool full = true) {
    if (full) {
        qDebug()<<pad<<"Completion info: Task"<<c.node()->name();
    }
    if (!c.isStarted()) {
        qDebug()<<pad<<"Not started";
        return;
    }
    QString pd = QStringLiteral("%1  ").arg(pad);
    if (c.isFinished()) {
        qDebug()<<pd<<" Started:"<<QTest::toString(QDateTime(c.startTime()));
        qDebug()<<pd<<"Finished:"<<QTest::toString(QDateTime(c.finishTime()));
    } else if (c.isStarted()) {
        qDebug()<<pd<<"Started:"<<QTest::toString(QDateTime(c.startTime()));
        qDebug()<<pd<<"Completed:"<<c.percentFinished()<<'%'<<"Mode:"<<c.entryModeToString();
        switch(c.entrymode()) {
            case Completion::EnterEffortPerTask: {
                const auto entries = c.entries();
                Completion::EntryList::const_iterator it = entries.constBegin();
                for (; it != entries.constEnd(); ++it) {
                    qDebug()<<pd<<it.key()<<QTest::toString(it.value()->percentFinished)<<'%'<<"remaining effort:"<<it.value()->remainingEffort.toDouble(Duration::Unit_h)<<'h';
                }
                break;
            }
            case Completion::EnterEffortPerResource: {
                const QString pd2 = QStringLiteral("%1  ").arg(pd);
                const auto resources = c.resources();
                const auto entries = c.entries();
                Completion::EntryList::const_iterator it = entries.constBegin();
                for (; it != entries.constEnd(); ++it) {
                    qDebug()<<pd<<it.key().toString(Qt::ISODate)<<it.value()->percentFinished<<'%'<<"remaining effort:"<<it.value()->remainingEffort.toDouble(Duration::Unit_h)<<'h';
                    for (const auto r : resources) {
                        qDebug()<<pd2<<r->name()<<':'<<c.getActualEffort(r, it.key()).effort().toDouble(Duration::Unit_h)<<'h';
                    }
                }
                break;
            }
                break;
            default:
                qDebug()<<pd<<"Unused mode:"<<c.entryModeToString();
                break;
        }
    }
}
static
void print(Task *t, const QString &str, bool full = true) {
    qDebug()<<"Debug info: Task"<<t->name()<<str;
    print(t, full);
}
static
void print(Task *t, const char *str, bool full = true) {
    print(t, QLatin1String(str), full);
}
static
void print(Task *t, bool full = true) {
    QString pad;
    if (t->level() > 0) {
        pad = QStringLiteral("%1").arg(QString(), t->level()*2, QLatin1Char(' '));
    }
    qDebug()<<pad<<"Task"<<t->wbsCode()<<t->name()<<t->typeToString()<<"Priority:"<<t->priority()<<t->constraintToString()<<(void*)t;
    if (t->isScheduled()) {
        qDebug()<<pad<<"     earlyStart:"<<QTest::toString(QDateTime(t->earlyStart()));
        qDebug()<<pad<<"      lateStart:"<<QTest::toString(QDateTime(t->lateStart()));
        qDebug()<<pad<<"    earlyFinish:"<<QTest::toString(QDateTime(t->earlyFinish()));
        qDebug()<<pad<<"     lateFinish:"<<QTest::toString(QDateTime(t->lateFinish()));
        qDebug()<<pad<<"      startTime:"<<QTest::toString(QDateTime(t->startTime()));
        qDebug()<<pad<<"        endTime:"<<QTest::toString(QDateTime(t->endTime()));
    } else {
        qDebug()<<pad<<"   Not scheduled";
    }
    qDebug()<<pad;
    switch (t->constraint()) {
        case Node::MustStartOn:
        case Node::StartNotEarlier:
            qDebug()<<pad<<"startConstraint:"<<QTest::toString(QDateTime(t->constraintStartTime()));
            break;
        case Node::FixedInterval:
            qDebug()<<pad<<"startConstraint:"<<QTest::toString(QDateTime(t->constraintStartTime()));
            break;
        case Node::MustFinishOn:
        case Node::FinishNotLater:
            qDebug()<<pad<<"  endConstraint:"<<QTest::toString(QDateTime(t->constraintEndTime()));
            break;
        default: break;
    }
    qDebug()<<pad<<"Estimate   :"<<t->estimate()->expectedEstimate()<<Duration::unitToString(t->estimate()->unit())
            <<t->estimate()->typeToString()
            <<(t->estimate()->type() == Estimate::Type_Duration
                ? (t->estimate()->calendar()?t->estimate()->calendar()->name():QStringLiteral("Fixed"))
                : QStringLiteral("%1 h").arg(t->estimate()->expectedValue().toDouble(Duration::Unit_h)));

    const auto resourceRequests = t->requests().resourceRequests();
    qDebug()<<pad<<"Requests:"<<"resources:"<<resourceRequests.count();
    for (ResourceRequest *rr : resourceRequests) {
        qDebug()<<pad<<printAvailable(rr->resource(), QStringLiteral("   ") + rr->resource()->name())<<"id:"<<rr->resource()->id()<<(void*)rr->resource()<<':'<<(void*)rr;
        const auto requierds = rr->requiredResources();
        if (!requierds.isEmpty()) {
            qDebug()<<pad<<"   Required resources:";
            for (const auto r : requierds) {
                qDebug()<<pad<<printAvailable(r, pad+QStringLiteral("   ") + r->name());
            }
        }
        const auto alts = rr->alternativeRequests();
        if (!alts.isEmpty()) {
            qDebug()<<pad<<"   Alternavives:";
            for (const auto alt : alts) {
                qDebug()<<pad<<printAvailable(alt->resource(), pad+QStringLiteral("   ") + alt->resource()->name());
            }
        }
    }
    if (t->isStartNode()) {
        qDebug()<<pad<<"Start node";
    }
    QStringList rel;
    const auto relations = t->dependChildNodes();
    for (Relation *r : relations) {
        QString type;
        switch(r->type()) {
        case Relation::StartStart: type = QStringLiteral("SS"); break;
        case Relation::FinishFinish: type = QStringLiteral("FF"); break;
        default: type = QStringLiteral("FS"); break;
        }
        rel << QStringLiteral("(%1) -> %2, %3 %4").arg(r->parent()->name()).arg(r->child()->name()).arg(type).arg(r->lag() == 0?QString():r->lag().toString(Duration::Format_HourFraction));
    }
    if (!rel.isEmpty()) {
        qDebug()<<pad<<"Successors:"<<rel.join(QStringLiteral(" : "));
    }
    if (t->isEndNode()) {
        qDebug()<<pad<<"End node";
    }
    rel.clear();
    const auto relations2 = t->dependParentNodes();
    for (Relation *r : relations2) {
        QString type;
        switch(r->type()) {
        case Relation::StartStart: type = QStringLiteral("SS"); break;
        case Relation::FinishFinish: type = QStringLiteral("FF"); break;
        default: type = QStringLiteral("FS"); break;
        }
        rel << QStringLiteral("%1 -> (%2), %3 %4").arg(r->parent()->name()).arg(r->child()->name()).arg(type).arg(r->lag() == 0?QString():r->lag().toString(Duration::Format_HourFraction));
    }
    if (!rel.isEmpty()) {
        qDebug()<<pad<<"Predeccessors:"<<rel.join(QStringLiteral(" : "));
    }
    qDebug()<<pad;
    Schedule *s = t->currentSchedule();
    if (s) {
        qDebug()<<pad<<"Appointments:"<<s->appointments().count();
        const auto appointments = s->appointments();
        for (Appointment *a : appointments) {
            qDebug()<<pad<<"  Resource:"<<a->resource()->resource()->name()<<"booked:"<<QTest::toString(QDateTime(a->startTime()))<<QTest::toString(QDateTime(a->endTime()))<<"effort:"<<a->effort(a->startTime(), a->endTime()).toDouble(Duration::Unit_h)<<'h';
            if (! full) { continue; }
            const auto intervals = a->intervals().map().values();
            for(const AppointmentInterval &i : intervals ) {
                qDebug()<<pad<<QStringLiteral("    ")<<QTest::toString(QDateTime(i.startTime()))<<QTest::toString(QDateTime(i.endTime()))<<i.load()<<"effort:"<<i.effort(i.startTime(), i.endTime()).toDouble(Duration::Unit_h)<<'h';
            }
        }
    }
    if (t->runningAccount()) {
        qDebug()<<pad<<"Running account :"<<t->runningAccount()->name();
    }
    if (t->startupAccount()) {
        qDebug()<<pad<<"Startup account :"<<t->startupAccount()->name()<<" cost:"<<t->startupCost();
    }
    if (t->shutdownAccount()) {
        qDebug()<<pad<<"Shutdown account:"<<t->shutdownAccount()->name()<<" cost:"<<t->shutdownCost();
    }
    print(t->completion(), pad);
    if (full) {
        for (int i = 0; i < t->numChildren(); ++i) {
            qDebug()<<pad;
            print(static_cast<Task*>(t->childNode(i)), full);
        }
    }
}
static
void print(const Completion &c, const QString &name, const QString &s = QString()) {
    qDebug()<<"Completion:"<<name<<s;
    qDebug()<<"  Entry mode:"<<c.entryModeToString();
    qDebug()<<"     Started:"<<c.isStarted()<<c.startTime();
    qDebug()<<"    Finished:"<<c.isFinished()<<c.finishTime();
    qDebug()<<"  Completion:"<<c.percentFinished()<<'%';

    if (! c.usedEffortMap().isEmpty()) {
        qDebug()<<"     Used effort:";
        const auto resources = c.usedEffortMap().keys();
        for (const Resource *r : resources) {  // clazy:exclude=container-anti-pattern
            Completion::UsedEffort *ue = c.usedEffort(r);
            const auto dates = ue->actualEffortMap().keys();
            for (const QDate &d : dates) { // clazy:exclude=container-anti-pattern
                qDebug()<<"         "<<r->name()<<':';
                qDebug()<<"             "<<d.toString(Qt::ISODate)<<':'<<c.actualEffort(d).toString()<<c.actualCost(d);
            }
        }
    }
}
static
void print(const EffortCostMap &m, const QString &s = QString()) {
    qDebug()<<"EffortCostMap"<<s;
    if (m.days().isEmpty()) {
        qDebug()<<"   Empty";
        return;
    }
    qDebug()<<m.startDate().toString(Qt::ISODate)<<m.endDate().toString(Qt::ISODate)
            <<" total="
            <<m.totalEffort().toString()
            <<m.totalCost()
            <<'('
            <<m.bcwpTotalEffort()
            <<m.bcwpTotalCost()
            <<')';

    if (! m.days().isEmpty()) {
        const auto dates = m.days().keys();
        for (const QDate &d : dates ) { // clazy:exclude=container-anti-pattern
            qDebug()<<QStringLiteral("    ")<<d.toString(Qt::ISODate)<<':'<<m.hoursOnDate(d)<<'h'<<m.costOnDate(d)
                                                    <<'('<<m.bcwpEffortOnDate(d)<<'h'<<m.bcwpCostOnDate(d)<<')';
        }
    }
}
static
void print(const EffortCostMap &m, const char *s) {
    print(m, QLatin1String(s));
}
static
void printSchedulingLog(const ScheduleManager &sm, const QString &s = QString())
{
    qDebug()<<"Scheduling log"<<s;
    qDebug()<<"Scheduling:"<<sm.name()<<(sm.recalculate()?QStringLiteral("recalculate from %1").arg(sm.recalculateFrom().toString()):QStringLiteral(""));
    const auto logs = sm.expected()->logMessages();
    for (const QString &s : logs) {
        qDebug()<<s;
    }
}
static
void print(Account *a, const QString &indent = QString(), long scheduleId = -1, const QString &s = QString())
{
    qDebug()<<indent+"Debug info Account:"<<a->name()<<s;
    qDebug()<<indent+"Account:"<<a->name()<<(a->isDefaultAccount() ? "Default" : "");
    EffortCostMap ec = a->plannedCost(scheduleId);
    qDebug()<<indent+"Planned cost:"<<ec.totalCost()<<"effort:"<<ec.totalEffort().toString();
    if (! a->isElement()) {
        const auto accounts = a->accountList();
        for (Account *c : accounts) {
            print(c, indent+"  ", scheduleId);
        }
        return;
    }
    const auto costs = a->costPlaces();
    qDebug()<<indent+"Cost places:"<<costs.count();
    for (Account::CostPlace *cp : costs) {
        qDebug()<<indent+"     Node:"<<(cp->node() ? cp->node()->name() : QString());
        qDebug()<<indent+"  running:"<<cp->running();
        qDebug()<<indent+"  startup:"<<cp->startup();
        qDebug()<<indent+" shutdown:"<<cp->shutdown();
    }
}

static
void print(const AppointmentInterval &i, const QString &indent = QString())
{
    QString s = indent + QStringLiteral("Interval:");
    if (! i.isValid()) {
        qDebug()<<s<<"Not valid";
    } else {
        qDebug()<<s<<i.startTime()<<i.endTime()<<i.load()<<'%';
    }
}

static
void print(const AppointmentIntervalList &lst, const QString &s = QString())
{
    qDebug()<<"Interval list:"<<lst.map().count()<<s;
    const auto intervals = lst.map().values();
    for (const AppointmentInterval &i : intervals) {
        print(i, QStringLiteral("  "));
    }
}

static
void print(const AppointmentIntervalList &lst, const char *s)
{
    print(lst, QLatin1String(s));
}

static
void printRelations(const Task *t)
{
    qDebug()<<"Relations"<<t<<"preds:"<<(t->dependParentNodes().count()+t->parentProxyRelations().count())<<"succs:"<<(t->dependChildNodes().count()+t->childProxyRelations().count());

    for (const auto r : t->dependParentNodes()) {
        qDebug()<<"parent depend:\t"<<r->parent()->name()<<"<-"<<r->child()->name();
    }
    for (const auto r : t->parentProxyRelations()) {
        qDebug()<<"parent proxy:\t"<<r->parent()->name()<<"<-"<<r->child()->name();
    }
    for (const auto r : t->dependChildNodes()) {
        qDebug()<<"child depend:\t"<<r->parent()->name()<<"->"<<r->child()->name();
    }
    for (const auto r : t->childProxyRelations()) {
        qDebug()<<"child proxy:\t"<<r->parent()->name()<<"->"<<r->child()->name();
    }
}

static
void printRelations(const Project *p)
{
    const auto tasks = p->allTasks();
    for (const auto t : tasks) {
        printRelations(t);
    }
}

};

}
