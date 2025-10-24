/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2011 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptappointment.h"

#include "kptproject.h"
#include "kpttask.h"
#include "kptdatetime.h"
#include "kptcalendar.h"
#include "kpteffortcostmap.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"
#include "kptdebug.h"

#include <KoXmlReader.h>


namespace KPlato
{

class Resource;

AppointmentInterval::AppointmentInterval()
    : d(new AppointmentIntervalData())
{
    //debugPlan<<this;
}

AppointmentInterval::AppointmentInterval(const AppointmentInterval &interval)
    : d(interval.d)
{
}

AppointmentInterval::AppointmentInterval(const DateTime &start, const DateTime &end, double load)
    : d(new AppointmentIntervalData())
{
    if (start.isValid() && end.isValid()) {
        Q_ASSERT(start.timeZone() == end.timeZone());
    }
    setStartTime(start);
    setEndTime(end);
    setLoad(load);
}

AppointmentInterval::AppointmentInterval(QDate date, const TimeInterval& timeInterval, double load)
    : d(new AppointmentIntervalData())
{
    Q_ASSERT(date.isValid() && timeInterval.isValid());
    DateTime s(date, timeInterval.startTime());
    DateTime e(date, timeInterval.endTime());
    if (timeInterval.endsMidnight()) {
        e = e.addDays(1);
    }
    setStartTime(s);
    setEndTime(e);
    setLoad(load);
#ifndef NDEBUG
    if (s.isValid() && e.isValid()) {
//        debugPlan<<*this;
        Q_ASSERT(s.timeZone() == e.timeZone());
    }
#endif
}

AppointmentInterval::~AppointmentInterval() {
    //debugPlan<<this;
}

const DateTime &AppointmentInterval::startTime() const
{
    return d->start;
}
void AppointmentInterval::setStartTime(const DateTime &time)
{
    if (d->start != time) {
        d->start = time;
    }
}

const DateTime &AppointmentInterval::endTime() const
{
    return d->end;
}

void AppointmentInterval::setEndTime(const DateTime &time)
{
    if (d->end != time) {
        d->end = time;
    }
}

double AppointmentInterval::load() const
{
    return d->load;
}

void AppointmentInterval::setLoad(double load)
{
    if (d->load != load) {
        d->load = load;
    }
}

QTimeZone AppointmentInterval::timeZone() const
{
    return d->start.timeZone();
}

AppointmentInterval &AppointmentInterval::toTimeZone(const QTimeZone &tz)
{
    if (d->start.timeZone() != tz) {
        d->start = d->start.toTimeZone(tz);
    }
    if (d->end.timeZone() != tz) {
        d->end = d->end.toTimeZone(tz);
    }
    return *this;
}

Duration AppointmentInterval::effort() const
{
    return (d->end - d->start) * d->load / 100;
}

Duration AppointmentInterval::effort(const DateTime &start, const DateTime &end) const {
    if (start >= d->end || end <= d->start) {
        return Duration::zeroDuration;
    }
    DateTime s = (start > d->start ? start : d->start);
    DateTime e = (end < d->end ? end : d->end);
    return (e - s) * d->load / 100;
}

Duration AppointmentInterval::effort(QDate time, bool upto) const {
    DateTime t(time);
    //debugPlan<<time<<upto<<t<<d->start<<d->end;
    if (upto) {
        if (t <= d->start) {
            return Duration::zeroDuration;
        }
        DateTime e = (t < d->end ? t : d->end);
        Duration eff = (e - d->start) * d->load / 100;
        //debugPlan<<d->toString();
        return eff;
    }
    // from time till end
    if (t >= d->end) {
        return Duration::zeroDuration;
    }
    DateTime s = (t > d->start ? t : d->start);
    return (d->end - s) * d->load / 100;
}

bool AppointmentInterval::loadXML(KoXmlElement &element, XMLLoaderObject &status) {
    //debugPlan;
    bool ok;
    QString s = element.attribute(QStringLiteral("start"));
    if (!s.isEmpty())
        d->start = DateTime::fromString(s, status.projectTimeZone());
    s = element.attribute(QStringLiteral("end"));
    if (!s.isEmpty())
        d->end = DateTime::fromString(s, status.projectTimeZone());
    d->load = element.attribute(QStringLiteral("load"), QStringLiteral("100")).toDouble(&ok);
    if (!ok) d->load = 100;
    if (! isValid()) {
        errorPlan<<"AppointmentInterval::loadXML: Invalid interval:"<<*this<<element.attribute(QStringLiteral("start"))<<element.attribute(QStringLiteral("end"));
    } else {
        Q_ASSERT(d->start.timeZone() == d->end.timeZone());
    }
    return isValid();
}

void AppointmentInterval::saveXML(QDomElement &element) const
{
    Q_ASSERT(isValid());
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("appointment-interval"));
    element.appendChild(me);

    me.setAttribute(QStringLiteral("start"), d->start.toString(Qt::ISODate));
    me.setAttribute(QStringLiteral("end"), d->end.toString(Qt::ISODate));
    me.setAttribute(QStringLiteral("load"), QString::number(d->load));
}

bool AppointmentInterval::isValid() const {
    return d->start.isValid() && d->end.isValid() && d->start < d->end && d->load >= 0.0;
}

AppointmentInterval AppointmentInterval::firstInterval(const AppointmentInterval &interval, const DateTime &from) const {
    //debugPlan<<interval.startTime().toString()<<" -"<<interval.endTime().toString()<<" from="<<from.toString();
    DateTime f = from;
    DateTime s1 = d->start;
    DateTime e1 = d->end;
    DateTime s2 = interval.startTime();
    DateTime e2 = interval.endTime();
    AppointmentInterval a;
    if (f.isValid() && f >= e1 && f >= e2) {
        return a;
    }
    if (f.isValid()) {
        if (s1 < f && f < e1) {
            s1 = f;
        }
        if (s2 < f && f < e2) {
            s2 = f;
        }
    }

    if (s1 < s2) {
        a.setStartTime(s1);
        if (e1 <= s2) {
            a.setEndTime(e1);
        } else {
            a.setEndTime(s2);
        }
        a.setLoad(d->load);
    } else if (s1 > s2) {
        a.setStartTime(s2);
        if (e2 <= s1) {
            a.setEndTime(e2);
        } else {
            a.setEndTime(s1);
        }
        a.setLoad(interval.load());
    } else {
        a.setStartTime(s1);
        if (e1 <= e2)
            a.setEndTime(e1);
        else
            a.setEndTime(e2);
        a.setLoad(d->load + interval.load());
    }
    //debugPlan<<a.startTime().toString()<<" -"<<a.endTime().toString()<<" load="<<a.load();
    return a;
}

AppointmentInterval &AppointmentInterval::operator=(const AppointmentInterval &interval)
{
    d = interval.d;
    return *this;
}

bool AppointmentInterval::operator==(const AppointmentInterval &interval) const
{
    return d->start == interval.d->start && d->end == interval.d->end && d->load == interval.d->load;
}

bool AppointmentInterval::operator!=(const AppointmentInterval &interval) const
{
    return !operator==(interval);
}

bool AppointmentInterval::operator<(const AppointmentInterval &other) const
{
    if (d->start < other.d->start) {
        //debugPlan<<"this start"<<d->start<<" < "<<other.d->start;
        return true;
    } else if (other.d->start < d->start) {
        //debugPlan<<"other start"<<other.d->start<<" < "<<d->start;
        return false;
    }
    // Start is assumed equal
    //debugPlan<<"this end"<<d->end<<" < "<<other.d->end;
    return d->end < other.d->end;
}

bool AppointmentInterval::intersects(const AppointmentInterval &other) const
{
    return (d->start < other.d->end && d->end > other.d->start);
}

AppointmentInterval AppointmentInterval::interval(const DateTime &start, const DateTime &end) const
{
    // TODO: Find and fix those that call with "wrong" timezone
    const DateTime s = start.toTimeZone(d->start.timeZone());
    const DateTime e = end.toTimeZone(d->end.timeZone());
    if (s <= d->start && e >= d->end) {
        return *this;
    }
    return AppointmentInterval(qMax(s, d->start), qMin(e, d->end), d->load);
}

bool AppointmentInterval::merge(const AppointmentInterval &interval)
{
    if (isConticuousTo(interval)) {
        const DateTime s = interval.d->start.toTimeZone(d->start.timeZone());
        const DateTime e = interval.d->end.toTimeZone(d->end.timeZone());
        if (s < d->start) {
            d->start = s;
        }
        if (e > d->end) {
            d->end = e;
        }
        return true;
    }
    return false;
}

bool AppointmentInterval::isConticuousTo(const AppointmentInterval &other) const
{
    return operator!=(other) && !intersects(other) && (d->end == other.d->start || d->start == other.d->end);
}

QString AppointmentInterval::toString() const
{
    return QStringLiteral("%1 - %2, %3%").arg(d->start.toString(Qt::ISODate)).arg(d->end.toString(Qt::ISODate)).arg(d->load);
}

QDebug operator<<(QDebug dbg, const KPlato::AppointmentInterval &i)
{
    dbg<<"AppointmentInterval["<<i.startTime()<<i.endTime()<<i.load()<<"%"<<']';
    return dbg;
}

//-----------------------
AppointmentIntervalList::AppointmentIntervalList()
{

}

AppointmentIntervalList::AppointmentIntervalList(const AppointmentIntervalList &other)
    : m_map(other.map())
{
}

AppointmentIntervalList::AppointmentIntervalList(const QMultiMap<QDate, AppointmentInterval> &other)
    : m_map(other)
{

}

QTimeZone AppointmentIntervalList::timeZone() const
{
    return m_map.isEmpty() ? QTimeZone::systemTimeZone() : m_map.first().timeZone();
}

AppointmentIntervalList &AppointmentIntervalList::toTimeZone(const QTimeZone &tz)
{
    AppointmentIntervalList m;
    for (const auto &interval : std::as_const(m_map)) {
        const AppointmentInterval i(interval.startTime().toTimeZone(tz), interval.endTime().toTimeZone(tz), interval.load());
        m.add(i);
    }
    m_map = m.m_map;
    return *this;
}

QMultiMap< QDate, AppointmentInterval > AppointmentIntervalList::map()
{
    return m_map;
}

const QMultiMap< QDate, AppointmentInterval >& AppointmentIntervalList::map() const
{
    return m_map;
}

AppointmentIntervalList &AppointmentIntervalList::operator=(const AppointmentIntervalList &lst)
{
    m_map = lst.m_map;
    return *this;
}

AppointmentIntervalList &AppointmentIntervalList::operator-=(const AppointmentIntervalList &lst)
{
    if (lst.m_map.isEmpty()) {
        return *this;
    }
    const auto intervals = lst.map().values();
    for (const AppointmentInterval &ai : intervals) {
        subtract(ai);
    }
    return *this;
}

void AppointmentIntervalList::subtract(const DateTime &st, const DateTime &et, double load)
{
    subtract(AppointmentInterval(st, et, load));
}

void AppointmentIntervalList::subtract(const AppointmentInterval &interval)
{
    //debugPlan<<st<<et<<load;
    if (m_map.isEmpty()) {
        return;
    }
    if (! interval.isValid()) {
        return;
    }
    const DateTime st = interval.startTime();
    const DateTime et = interval.endTime();
    Q_ASSERT(st < et);
    const double load = interval.load();
//     debugPlan<<"subtract:"<<*this<<'\n'<<"minus"<<interval;
    for (QDate date = st.date(); date <= et.date(); date = date.addDays(1)) {
        if (! m_map.contains(date)) {
            continue;
        }
        QList<AppointmentInterval> l;
        const QList<AppointmentInterval> v = m_map.values(date);
        m_map.remove(date);
        for (const AppointmentInterval &vi : v) {
            if (! vi.intersects(interval)) {
                //debugPlan<<"subtract: not intersect:"<<vi<<interval;
                l.insert(0, vi);
                //if (! l.at(0).isValid()) { debugPlan<<vi<<interval<<l.at(0); qFatal("Invalid interval"); }
                continue;
            }
            if (vi < interval) {
                //debugPlan<<"subtract: vi<interval"<<vi<<interval;
                if (vi.startTime() < st) {
                    l.insert(0, AppointmentInterval(vi.startTime(), st, vi.load()));
                    //if (! l.at(0).isValid()) { debugPlan<<vi<<interval<<l.at(0); qFatal("Invalid interval"); }
                }
                if (vi.load() > load) {
                    l.insert(0, AppointmentInterval(st, qMin(vi.endTime(), et), vi.load() - load));
                    //if (! l.at(0).isValid()) { debugPlan<<vi<<interval<<l.at(0); qFatal("Invalid interval"); }
                }
            } else if (interval < vi) {
                //debugPlan<<"subtract: interval<vi"<<vi<<interval;
                if (vi.load() > load) {
                    //debugPlan<<"subtract: interval<vi vi.load > load"<<vi.load()<<load;
                    l.insert(0, AppointmentInterval(vi.startTime(), qMin(vi.endTime(), et), vi.load() - load));
                    //if (! l.at(0).isValid()) { debugPlan<<vi<<interval<<l.at(0); qFatal("Invalid interval"); }
                }
                if (et < vi.endTime()) {
                    //debugPlan<<"subtract: interval<vi et < vi.endTime"<<et<<vi.endTime();
                    l.insert(0, AppointmentInterval(et, vi.endTime(), vi.load()));
                    //if (! l.at(0).isValid()) { debugPlan<<vi<<interval<<l.at(0); qFatal("Invalid interval"); }
                }
            } else if (vi.load() > load) {
                //debugPlan<<"subtract: vi==interval"<<vi<<interval;
                l.insert(0, AppointmentInterval(st, et, vi.load() - load));
                //if (! l.at(0).isValid()) { debugPlan<<vi<<interval<<l.at(0); qFatal("Invalid interval"); }
            }
        }
        for (const AppointmentInterval &i : std::as_const(l)) {
            //if (! i.isValid()) { debugPlan<<interval<<i; qFatal("Invalid interval"); }
            m_map.insert(date, i);
        }
    }
    //debugPlan<<"subtract:"<<interval<<" result="<<'\n'<<*this;
}

AppointmentIntervalList &AppointmentIntervalList::operator+=(const AppointmentIntervalList &lst)
{
    if (lst.isEmpty()) {
        return *this;
    }
    const auto intervals = lst.map().values();
    for (const AppointmentInterval &ai : intervals) {
        add(ai);
    }
    return *this;
}

AppointmentIntervalList AppointmentIntervalList::extractIntervals(const DateTime &start, const DateTime &end) const
{
    if (isEmpty()) {
        return AppointmentIntervalList();
    }
    QList<AppointmentInterval> ilst;
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = m_map.lowerBound(start.date());
    for (; it != m_map.constEnd() && it.key() <= end.date(); ++it) {
        AppointmentInterval i = it.value().interval(start, end);
        if (i.isValid()) {
            ilst.append(i);
        }
    }
    QMultiMap<QDate, AppointmentInterval> lst;
    while (!ilst.isEmpty()) {
        auto i = ilst.takeLast();
        lst.insert(i.startTime().date(), i);
    }
    return AppointmentIntervalList(lst);
}

void AppointmentIntervalList::add(const DateTime &st, const DateTime &et, double load)
{
    add(AppointmentInterval(st, et, load));
}

void AppointmentIntervalList::add(const AppointmentInterval &ai)
{
    if (! ai.isValid()) {
        debugPlan<<ai;
        Q_ASSERT(ai.isValid());
        return;
    }
    auto interval = ai;
    if (!isEmpty()) {
        interval.toTimeZone(m_map.first().timeZone());
    }
    QDate date = interval.startTime().date();
    QDate ed =  interval.endTime().date();
    int load = interval.load();

    QList<AppointmentInterval> lst;
    if (date == ed) {
        lst << interval;
    } else {
        // split intervals into separate dates
        const auto tz = interval.timeZone();
        QTime t1 = interval.startTime().time();
        while (date < ed) {
            lst << AppointmentInterval(DateTime(date, t1, tz), DateTime(date.addDays(1), QTime(0, 0), tz), load);
            //debugPlan<<"split:"<<date<<lst.last();
            Q_ASSERT_X(lst.last().isValid(), "Split", "Invalid interval");
            date = date.addDays(1);
            t1 = QTime();
        }
        if (interval.endTime().time() != QTime(0, 0, 0)) {
            lst << AppointmentInterval(DateTime(ed, QTime(0, 0), tz), interval.endTime(), load);
            Q_ASSERT_X(lst.last().isValid(), "Split", "Invalid interval");
        }
    }
    for (AppointmentInterval li : std::as_const(lst)) {
        Q_ASSERT_X(lst.last().isValid(), "Add", "Invalid interval");
        date = li.startTime().date();
        if (! m_map.contains(date)) {
            m_map.insert(date, li);
            continue;
        }
        const QList<AppointmentInterval> v = m_map.values(date);
        m_map.remove(date);
        QList<AppointmentInterval> l;
        for (const AppointmentInterval &vi : v) {
            if (! li.isValid()) {
                l.insert(0, vi);
                Q_ASSERT_X(l.at(0).isValid(), "Original", "Invalid interval");
                continue;
            }
            if (li.isConticuousTo(vi)) {
                li.merge(vi);
                continue;
            }
            if (!li.intersects(vi)) {
                //debugPlan<<"not intersects:"<<li<<vi;
                if (li < vi) {
                    if (! l.contains(li)) {
                        //debugPlan<<"li < vi:"<<"insert li"<<li;
                        l.insert(0, li);
                        Q_ASSERT_X(l.at(0).isValid(), "No intersects", "Add Invalid interval");
                        li = AppointmentInterval();
                    }
                    //debugPlan<<"li < vi:"<<"insert vi"<<vi;
                    l.insert(0, vi);
                    Q_ASSERT_X(l.at(0).isValid(), "No intersects", "Add Invalid interval");
                } else if (vi < li) {
                    //debugPlan<<"vi < li:"<<"insert vi"<<vi;
                    l.insert(0, vi);
                    Q_ASSERT_X(l.at(0).isValid(), "No intersects", "Add Invalid interval");
                } else { Q_ASSERT(false); }
                continue;
            }
            //debugPlan<<"intersects, merge"<<li<<vi;
            if (li < vi) {
                //debugPlan<<"li < vi:";
                if (li.startTime() < vi.startTime()) {
                    l.insert(0, AppointmentInterval(li.startTime(), vi.startTime(), li.load()));
                    Q_ASSERT_X(l.at(0).isValid(), "Intersects, start", "Add Invalid interval");
                }
                l.insert(0, AppointmentInterval(vi.startTime(), qMin(vi.endTime(), li.endTime()), vi.load() + li.load()));
                Q_ASSERT_X(l.at(0).isValid(), "Intersects, middle", "Add Invalid interval");
                li.setStartTime(l.at(0).endTime()); // if more of li, it may overlap with next vi
                if (l.at(0).endTime() < vi.endTime()) {
                    l.insert(0, AppointmentInterval(l.at(0).endTime(), vi.endTime(), vi.load()));
                    //debugPlan<<"li < vi: vi rest:"<<l.at(0);
                    Q_ASSERT_X(l.at(0).isValid(), "Intersects, end", "Add Invalid interval");
                }
            } else if (vi < li) {
                //debugPlan<<"vi < li:";
                if (vi.startTime() < li.startTime()) {
                    l.insert(0, AppointmentInterval(vi.startTime(), li.startTime(), vi.load()));
                    Q_ASSERT_X(l.at(0).isValid(), "Intersects, start", "Add Invalid interval");
                }
                l.insert(0, AppointmentInterval(li.startTime(), qMin(vi.endTime(), li.endTime()), vi.load() + li.load()));
                Q_ASSERT_X(l.at(0).isValid(), "Intersects, middle", "Add Invalid interval");
                li.setStartTime(l.at(0).endTime()); // if more of li, it may overlap with next vi
                if (l.at(0).endTime() < vi.endTime()) {
                    l.insert(0, AppointmentInterval(l.at(0).endTime(), vi.endTime(), vi.load()));
                    //debugPlan<<"vi < li: vi rest:"<<l.at(0);
                    Q_ASSERT_X(l.at(0).isValid(), "Intersects, end", "Add Invalid interval");
                }
            } else {
                //debugPlan<<"vi == li:";
                li.setLoad(vi.load() + li.load());
                l.insert(0, li);
                Q_ASSERT_X(l.at(0).isValid(), "Equal", "Add Invalid interval");
                li = AppointmentInterval();
            }
        }
        // If there is a rest of li, it must be inserted
        if (li.isValid()) {
            //debugPlan<<"rest:"<<li;
            l.insert(0, li);
        }
        for(const AppointmentInterval &i : std::as_const(l)) {
            Q_ASSERT(i.isValid());
            m_map.insert(i.startTime().date(), i);
        }
    }
}

// Returns the total effort
Duration AppointmentIntervalList::effort() const
{
    Duration d;
    const auto intervals = m_map.values();
    for (const AppointmentInterval &i : intervals) {
        d += i.effort();
    }
    return d;
}

// Returns the effort from start to end
Duration AppointmentIntervalList::effort(const DateTime &start, const DateTime &end) const
{
    Duration d;
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = m_map.lowerBound(start.date());
    for (; it != m_map.constEnd() && it.key() <= end.date(); ++it) {
        d += it.value().effort(start, end);
    }
    return d;
}

void AppointmentIntervalList::saveXML(QDomElement &element) const
{
    const auto intervals = m_map.values();
    for (const AppointmentInterval &i : intervals) {
        i.saveXML(element);
#ifndef NDEBUG
        if (!i.isValid()) {
            // NOTE: This should not happen, so hunt down cause if it does
            warnPlan<<"Invalid interval:"<<i;
        }
#endif
    }
}

bool AppointmentIntervalList::loadXML(KoXmlElement &element, XMLLoaderObject &status)
{
    KoXmlElement e;
    forEachElement(e, element) {
        if (e.tagName() == QStringLiteral("appointment-interval") || (status.version() < QStringLiteral("0.7.0") && e.tagName() == QStringLiteral("interval"))) {
            AppointmentInterval a;
            if (a.loadXML(e, status)) {
                add(a);
            } else {
                errorPlan<<"AppointmentIntervalList::loadXML:"<<"Could not load interval"<<a;
            }
        }
    }
    return true;
}

QDebug operator<<(QDebug dbg, const KPlato::AppointmentIntervalList &i)
{
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = i.map().constBegin();
    for (; it != i.map().constEnd(); ++it) {
        dbg<<'\n'<<it.key()<<":"<<it.value().startTime()<<it.value().endTime()<<it.value().load()<<"%";
    }
    return dbg;
}

////
Appointment::Appointment()
    : m_extraRepeats(), m_skipRepeats() {
    //debugPlan<<"("<<this<<")";
    m_resource=nullptr;
    m_node=nullptr;
    m_calculationMode = Schedule::Scheduling;
    m_repeatInterval=Duration();
    m_repeatCount=0;
}

Appointment::Appointment(Schedule *resource, Schedule *node, const DateTime &start, const DateTime &end, double load)
    : m_extraRepeats(),
      m_skipRepeats() {
    //debugPlan<<"("<<this<<")";
    m_node = node;
    m_resource = resource;
    m_calculationMode = Schedule::Scheduling;
    m_repeatInterval = Duration();
    m_repeatCount = 0;

    addInterval(start, end, load);
}

Appointment::Appointment(Schedule *resource, Schedule *node, const DateTime &start, Duration duration, double load)
    : m_extraRepeats(),
      m_skipRepeats() {
    //debugPlan<<"("<<this<<")";
    m_node = node;
    m_resource = resource;
    m_calculationMode = Schedule::Scheduling;
    m_repeatInterval = Duration();
    m_repeatCount = 0;

    addInterval(start, duration, load);

}

Appointment::Appointment(const Appointment &app)
{
    copy(app);
}


Appointment::~Appointment() {
    //debugPlan<<"("<<this<<")";
    detach();
}

Appointment &Appointment::toTimeZone(const QTimeZone &tz)
{
    m_intervals.toTimeZone(tz);
    return *this;
}

void Appointment::clear()
{
    m_intervals.clear();
}

AppointmentIntervalList Appointment::intervals(const DateTime &start, const DateTime &end) const
{
    //debugPlan<<start<<end;
    AppointmentIntervalList lst;
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = m_intervals.map().lowerBound(start.date());
    for (; it != m_intervals.map().constEnd() && it.key() <= end.date(); ++it) {
        AppointmentInterval ai = it.value().interval(start, end);
        if (ai.isValid()) {
            lst.add(ai);
            //debugPlan<<ai.startTime().toString()<<ai.endTime().toString();
        }
    }
    return lst;
}

void Appointment::setIntervals(const AppointmentIntervalList &lst) {
    m_intervals.clear();
    const auto intervals = lst.map().values();
    for (const AppointmentInterval &i : intervals) {
        m_intervals.add(i);
    }
}

void Appointment::addInterval(const AppointmentInterval &a) {
    Q_ASSERT(a.isValid());
    Q_ASSERT(a.startTime().timeZone() == a.endTime().timeZone());
    m_intervals.add(a);
//     if (m_resource && m_resource->resource() && m_node && m_node->node()) debugPlan<<"Mode="<<m_calculationMode<<":"<<m_resource->resource()->name()<<" to"<<m_node->node()->name()<<""<<a.startTime()<<a.endTime();
}
void Appointment::addInterval(const DateTime &start, const DateTime &end, double load) {
    Q_ASSERT(start < end);
    addInterval(AppointmentInterval(start, end, load));
}

void Appointment::addInterval(const DateTime &start, KPlato::Duration duration, double load) {
    DateTime e = start+duration;
    addInterval(start, e, load);
}

double Appointment::maxLoad() const {
    double v = 0.0;
    const auto intervals = m_intervals.map().values();
    for (const AppointmentInterval &i : intervals) {
        if (v < i.load())
            v = i.load();
    }
    return v;
}

DateTime Appointment::startTime() const {
    if (isEmpty()) {
        //debugPlan<<"empty list";
        return DateTime();
    }
    return m_intervals.map().values().first().startTime();
}

DateTime Appointment::endTime() const {
    if (isEmpty()) {
        //debugPlan<<"empty list";
        return DateTime();
    }
    return m_intervals.map().values().last().endTime();
}

bool Appointment::isBusy(const DateTime &/*start*/, const DateTime &/*end*/) {
    return false;
}

void Appointment::saveXML(QDomElement &element) const {
    if (isEmpty()) {
        errorPlan<<"Incomplete appointment data: No intervals";
    }
    if (m_resource == nullptr || m_resource->resource() == nullptr) {
        errorPlan<<"Incomplete appointment data: No resource";
        return;
    }
    if (m_node == nullptr || m_node->node() == nullptr) {
        errorPlan<<"Incomplete appointment data: No node";
        return; // shouldn't happen
    }
    //debugPlan;
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("appointment"));
    element.appendChild(me);

    me.setAttribute(QStringLiteral("resource-id"), m_resource->resource()->id());
    me.setAttribute(QStringLiteral("task-id"), m_node->node()->id());
    //debugPlan<<m_resource->resource()->name()<<m_node->node()->name();
    m_intervals.saveXML(me);
}

// Returns the total planned effort for this appointment
Duration Appointment::plannedEffort(const Resource *resource, EffortCostCalculationType type) const {
    if (m_resource->resource() != resource) {
        return Duration::zeroDuration;
    }
    return plannedEffort(type);
}

// Returns the total planned effort for this appointment
Duration Appointment::plannedEffort(EffortCostCalculationType type) const {
    Duration d;
    if (type == ECCT_All || m_resource == nullptr || m_resource->resource()->type() == Resource::Type_Work) {
        const auto intervals = m_intervals.map().values();
        for (const AppointmentInterval &i : intervals) {
            d += i.effort();
        }
    }
    return d;
}

// Returns the planned effort on the date
Duration Appointment::plannedEffort(QDate date, EffortCostCalculationType type) const {
    Duration d;
    if (type == ECCT_All || m_resource == nullptr || m_resource->resource()->type() == Resource::Type_Work) {
        QMultiMap<QDate, AppointmentInterval>::const_iterator it = m_intervals.map().constFind(date);
        for (; it != m_intervals.map().constEnd() && it.key() == date; ++it) {
            d += it.value().effort();
        }
    }
    return d;
}

// Returns the planned effort on the date
Duration Appointment::plannedEffort(const Resource *resource, QDate date, EffortCostCalculationType type) const {
    if (resource != m_resource->resource()) {
        return Duration::zeroDuration;
    }
    return plannedEffort(date, type);
}

// Returns the planned effort upto and including the date
Duration Appointment::plannedEffortTo(QDate date, EffortCostCalculationType type) const {
    Duration d;
    QDate e(date.addDays(1));
    if (type == ECCT_All || m_resource == nullptr || m_resource->resource()->type() == Resource::Type_Work) {
        const auto intervals = m_intervals.map().values();
        for (const AppointmentInterval &i : intervals) {
            d += i.effort(e, true); // upto e, not including
        }
    }
    //debugPlan<<date<<d.toString();
    return d;
}

// Returns the planned effort upto and including the date
Duration Appointment::plannedEffortTo(const Resource *resource, QDate date, EffortCostCalculationType type) const {
    if (resource != m_resource->resource()) {
        return Duration::zeroDuration;
    }
    return plannedEffortTo(date, type);
}

// Returns the planned effort upto time
Duration Appointment::plannedEffortTo(const QDateTime &time, EffortCostCalculationType type) const {
    Duration d;
    if (type == ECCT_All || m_resource == nullptr || m_resource->resource()->type() == Resource::Type_Work) {
        const auto intervals = this->intervals(startTime(), time).map().values();
        for (const AppointmentInterval &i : intervals) {
            d += i.effort(i.startTime(), time); // upto e, not including
        }
    }
    //debugPlan<<date<<d.toString();
    return d;
}

EffortCostMap Appointment::plannedPrDay(QDate pstart, QDate pend, EffortCostCalculationType type) const {
    //debugPlan<<m_node->id()<<","<<m_resource->id();
    EffortCostMap ec;
    QDate start = pstart.isValid() ? pstart : startTime().date();
    QDate end = pend.isValid() ? pend : endTime().date();
    double rate = m_resource && m_resource->resource() ? m_resource->normalRatePrHour() : 0.0;
    Resource::Type rt = m_resource && m_resource->resource() ? m_resource->resource()->type() : Resource::Type_Work;
    Duration zero;
    //debugPlan<<rate<<m_intervals.count();
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = m_intervals.map().lowerBound(start);
    for (; it != m_intervals.map().constEnd() && it.key() <= end; ++it) {
        //debugPlan<<start<<end<<dt;
        Duration eff;
        switch (type) {
            case ECCT_All:
                eff = it.value().effort();
                ec.add(it.key(), eff, eff.toDouble(Duration::Unit_h) * rate);
                break;
            case ECCT_EffortWork:
                eff = it.value().effort();
                ec.add(it.key(), (rt == Resource::Type_Work ? eff : zero), eff.toDouble(Duration::Unit_h) * rate);
                break;
            case ECCT_Work:
                if (rt == Resource::Type_Work) {
                    eff = it.value().effort();
                    ec.add(it.key(), eff, eff.toDouble(Duration::Unit_h) * rate);
                }
                break;
        }
    }
    return ec;
}

EffortCost Appointment::plannedCost(EffortCostCalculationType type) const {
    EffortCost ec;
    ec.setEffort(plannedEffort(type));
    if (m_resource && m_resource->resource()) {
        switch (type) {
            case ECCT_Work:
                if (m_resource->resource()->type() != Resource::Type_Work) {
                    break;
                }
                // fall through
            default:
                ec.setCost(ec.hours() * m_resource->resource()->normalRate()); //FIXME overtime
                break;
        }
    }
    return ec;
}

//Calculates the planned cost on date
double Appointment::plannedCost(QDate date, EffortCostCalculationType type) {
    if (m_resource && m_resource->resource()) {
        switch (type) {
            case ECCT_Work:
                if (m_resource->resource()->type() != Resource::Type_Work) {
                    break;
                }
                // fall through
            default:
                return plannedEffort(date).toDouble(Duration::Unit_h) * m_resource->resource()->normalRate(); //FIXME overtime
        }
    }
    return 0.0;
}

//Calculates the planned cost upto and including date
double Appointment::plannedCostTo(QDate date, EffortCostCalculationType type) {
    if (m_resource && m_resource->resource()) {
        switch (type) {
            case ECCT_Work:
                if (m_resource->resource()->type() != Resource::Type_Work) {
                    break;
                }
                // fall through
            default:
                return plannedEffortTo(date).toDouble(Duration::Unit_h) * m_resource->resource()->normalRate(); //FIXME overtime
        }
    }
    return 0.0;
}

bool Appointment::attach() {
    //debugPlan<<"("<<this<<")";
    if (m_resource && m_node) {
        m_resource->attach(this);
        m_node->attach(this);
        return true;
    }
    warnPlan<<"Failed: "<<(m_resource ? "" : "resource=0 ")
                                       <<(m_node ? "" : "node=0");
    return false;
}

void Appointment::detach() {
    //debugPlan<<"("<<this<<")"<<m_calculationMode<<":"<<m_resource<<","<<m_node;
    if (m_resource) {
        m_resource->takeAppointment(this, m_calculationMode); // takes from node also
    }
    if (m_node) {
        m_node->takeAppointment(this, m_calculationMode); // to make it robust
    }
}

// Returns the effort from start to end
Duration Appointment::effort(const DateTime &start, const DateTime &end, EffortCostCalculationType type) const {
    Duration e;
    if (type == ECCT_All || m_resource == nullptr || m_resource->resource()->type() == Resource::Type_Work) {
        e = m_intervals.effort(start, end);
    }
    return e;
}
// Returns the effort from start for the duration
Duration Appointment::effort(const DateTime &start, KPlato::Duration duration, EffortCostCalculationType type) const {
    Duration d;
    if (type == ECCT_All || m_resource == nullptr || m_resource->resource()->type() == Resource::Type_Work) {
        const auto intervals = m_intervals.map().values();
        for (const AppointmentInterval &i : intervals) {
            d += i.effort(start, start+duration);
        }
    }
    return d;
}

Appointment &Appointment::operator=(const Appointment &app) {
    copy(app);
    return *this;
}

Appointment &Appointment::operator+=(const Appointment &app) {
    merge(app);
    return *this;
}

Appointment Appointment::operator+(const Appointment &app) {
    Appointment a(*this);
    a.merge(app);
    return a;
}

Appointment &Appointment::operator-=(const Appointment &app) {
    m_intervals -= app.m_intervals;
    return *this;
}

void Appointment::copy(const Appointment &app) {
    m_resource = nullptr; //app.resource(); // NOTE: Don't copy, the new appointment
    m_node = nullptr; //app.node();         // NOTE: doesn't belong to anyone yet.
    //TODO: incomplete but this is all we use atm
    m_calculationMode = app.calculationMode();
    
    //m_repeatInterval = app.repeatInterval();
    //m_repeatCount = app.repeatCount();

    m_intervals.clear();
    const auto intervals = app.intervals().map().values();
    for (const AppointmentInterval &i : intervals) {
        addInterval(i);
    }
}

void Appointment::merge(const Appointment &app) {
    //debugPlan<<this<<(m_node ? m_node->node()->name() : "no node")<<(app.node() ? app.node()->node()->name() : "no node");
    if (app.isEmpty()) {
        return;
    }
    if (isEmpty()) {
        setIntervals(app.intervals());
        return;
    }
    QList<AppointmentInterval> result;
    QList<AppointmentInterval> lst1 = m_intervals.map().values();
    AppointmentInterval i1;
    QList<AppointmentInterval> lst2 = app.intervals().map().values();
    //debugPlan<<"add"<<lst1.count()<<" intervals to"<<lst2.count()<<" intervals";
    AppointmentInterval i2;
    int index1 = 0, index2 = 0;
    DateTime from;
    while (index1 < lst1.size() || index2 < lst2.size()) {
        if (index1 >= lst1.size()) {
            i2 = lst2[index2];
            if (!from.isValid() || from < i2.startTime())
                from = i2.startTime();
            result.append(AppointmentInterval(from, i2.endTime(), i2.load()));
            //debugPlan<<"Interval+ (i2):"<<from<<" -"<<i2.endTime();
            from = i2.endTime();
            ++index2;
            continue;
        }
        if (index2 >= lst2.size()) {
            i1 = lst1[index1];
            if (!from.isValid() || from < i1.startTime())
                from = i1.startTime();
            result.append(AppointmentInterval(from, i1.endTime(), i1.load()));
            //debugPlan<<"Interval+ (i1):"<<from<<" -"<<i1.endTime();
            from = i1.endTime();
            ++index1;
            continue;
        }
        i1 = lst1[index1];
        i2 = lst2[index2];
        AppointmentInterval i =  i1.firstInterval(i2, from);
        if (!i.isValid()) {
            break;
        }
        result.append(AppointmentInterval(i)); 
        from = i.endTime();
        //debugPlan<<"Interval+ (i):"<<i.startTime()<<" -"<<i.endTime()<<" load="<<i.load();
        if (i.endTime() >= i1.endTime()) {
            ++index1;
        }
        if (i.endTime() >= i2.endTime()) {
            ++index2;
        }
    }
    m_intervals.clear();
    for (const AppointmentInterval &i : std::as_const(result)) {
        m_intervals.add(i);
    }
    //debugPlan<<this<<":"<<m_intervals.count();
    return;
}

Appointment Appointment::extractIntervals(const DateTimeInterval& interval) const
{
    Appointment a;
    if (interval.isValid()) {
        a.setIntervals(m_intervals.extractIntervals(interval.first, interval.second));
    }
    return a;
}


}  //KPlato namespace

QDebug operator<<(QDebug dbg, const KPlato::Appointment *a)
{
    dbg << "Appointment("<<(void*)a<<')';
    if (a) {
        dbg<<'['<<a->node()<<a->resource()<<a->startTime()<<'-'<<a->endTime();
        const auto lst = a->intervals().map();
        for (const auto &i : lst) { // clazy:exclude=range-loop-reference
            dbg<<'\n'<<'\t'<<i;
        }
        dbg<<']';
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, const KPlato::Appointment &a)
{
    dbg << &a;
    return dbg;
}
