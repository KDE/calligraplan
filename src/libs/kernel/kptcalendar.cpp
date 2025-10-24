/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003-2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptcalendar.h"

#include "kptappointment.h"
#include "kptmap.h"
#include "kptduration.h"
#include "kptdatetime.h"
#include "kptproject.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"
#include "kptcommand.h"
#include "kptdebug.h"

#include <KoXmlReader.h>

#include <KLocalizedString>

#ifdef HAVE_KHOLIDAYS
#include <KHolidays/HolidayRegion>
#include <KHolidays/Holiday>
#endif

#include <QTimeZone>
#include <QDate>

namespace KPlato
{

#ifdef HAVE_KHOLIDAYS
// start qt-copy (with some changes)
// SPDX-FileCopyrightText: 2013 John Layt <jlayt@kde.org>
struct TzTimeZone {
    QString country;
    QByteArray comment;
};

// Define as a type as Q_GLOBAL_STATIC doesn't like it
typedef QHash<QByteArray, TzTimeZone> TzTimeZoneHash;

// Parse zone.tab table, assume lists all installed zones, if not will need to read directories
static TzTimeZoneHash loadTzTimeZones()
{
    QString path = QStringLiteral("/usr/share/zoneinfo/zone.tab");
    if (!QFile::exists(path))
        path = QStringLiteral("/usr/lib/zoneinfo/zone.tab");

    QFile tzif(path);
    if (!tzif.open(QIODevice::ReadOnly))
        return TzTimeZoneHash();

    TzTimeZoneHash zonesHash;
    // TODO QTextStream inefficient, replace later
    QTextStream ts(&tzif);
    while (!ts.atEnd()) {
        const QString line = ts.readLine();
        // Comment lines are prefixed with a #
        if (!line.isEmpty() && line.at(0) != QLatin1Char('#')) {
            // Data rows are tab-separated columns Region, Coordinates, ID, Optional Comments
            QStringList parts = line.split(QLatin1Char('\t'));
            TzTimeZone zone;
            zone.country = parts.at(0);
            if (parts.size() > 3)
                zone.comment = parts.at(3).toUtf8();
            zonesHash.insert(parts.at(2).toUtf8(), zone);
        }
    }
    return zonesHash;
}
// Hash of available system tz files as loaded by loadTzTimeZones()
Q_GLOBAL_STATIC_WITH_ARGS(const TzTimeZoneHash, tzZones, (loadTzTimeZones()))
// end qt-copy
#endif

QString CalendarDay::stateToString(int st, bool trans)
{
    return
        (st == None) ?
            (trans ? i18n("Undefined") : QStringLiteral("Undefined")) :
        (st == NonWorking) ?
            (trans ? i18n("Non-working") : QStringLiteral("Non-working")) :
        (st == Working) ?
            (trans ?  i18n("Working") : QStringLiteral("Working")) :
        QString();
}

QStringList CalendarDay::stateList(bool trans)
{
    QStringList lst;
    return trans
        ? lst << i18n("Undefined") << i18n("Non-working") << i18n("Working")
        : lst << QStringLiteral("Undefined") << QStringLiteral("Non-working") << QStringLiteral("Working");
}

/////   CalendarDay   ////
CalendarDay::CalendarDay()
    : m_date(),
      m_state(Undefined),
      m_calendar(nullptr)
{

    //debugPlan<<"("<<this<<")";
}

CalendarDay::CalendarDay(int state)
    : m_date(),
      m_state(state),
      m_calendar(nullptr)
{

    //debugPlan<<"("<<this<<")";
}

CalendarDay::CalendarDay(QDate date, int state)
    : m_date(date),
      m_state(state),
      m_calendar(nullptr)
{

    //debugPlan<<"("<<this<<")";
}

CalendarDay::CalendarDay(CalendarDay *day)
{
    //debugPlan<<"("<<this<<") from ("<<day<<")";
    copy(*day);
}

CalendarDay::~CalendarDay() {
    //debugPlan<<"("<<this<<")";
    while (!m_timeIntervals.isEmpty())
        delete m_timeIntervals.takeFirst();
}

const CalendarDay &CalendarDay::copy(const CalendarDay &day) {
    m_calendar = nullptr; // NOTE
    //debugPlan<<"("<<&day<<") date="<<day.date().toString();
    m_date = day.date();
    m_state = day.state();
    m_timeIntervals.clear();
    const auto intervals = day.timeIntervals();
    for (TimeInterval *i : intervals) {
        m_timeIntervals.append(new TimeInterval(*i));
    }
    return *this;
}

void CalendarDay::save(QDomElement &element) const {
    //debugPlan<<m_date.toString();
    if (m_state == None)
        return;
    if (m_date.isValid()) {
        element.setAttribute(QStringLiteral("date"), m_date.toString(Qt::ISODate));
    }
    element.setAttribute(QStringLiteral("state"), QString::number(m_state));
    if (m_timeIntervals.count() == 0)
        return;

    for (TimeInterval *i : std::as_const(m_timeIntervals)) {
        QDomElement me = element.ownerDocument().createElement(QStringLiteral("time-interval"));
        element.appendChild(me);
        me.setAttribute(QStringLiteral("length"), QString::number(i->second));
        me.setAttribute(QStringLiteral("start"), i->first.toString());
    }
}

void CalendarDay::addInterval(TimeInterval *interval) {
    if (!interval) {
        return;
    }

    // TODO: check for overlapping intervals and handle them for what makes sense
    QList <TimeInterval*>::Iterator it;
    QList <TimeInterval*>::Iterator end = m_timeIntervals.end();
    QList <TimeInterval*>::Iterator position = end;
    for (it = m_timeIntervals.begin(); it != end; ++it) {
        // first found that is later?
        if ((*it)->startTime() > interval->startTime()) {
            // insert before
            position = it;
            break;
        }
    }
    m_timeIntervals.insert(position, interval);
}

bool CalendarDay::operator==(const CalendarDay *day) const {
    return operator==(*day);
}
bool CalendarDay::operator==(const CalendarDay &day) const {
    //debugPlan;
    if (m_date.isValid() && day.date().isValid()) {
        if (m_date != day.date()) {
            //debugPlan<<m_date.toString()<<" !="<<day.date().toString();
            return false;
        }
    } else if (m_date.isValid() != day.date().isValid()) {
        //debugPlan<<"one of the dates is not valid";
        return false;
    }
    if (m_state != day.state()) {
        //debugPlan<<m_state<<" !="<<day.state();
        return false;
    }
    if (m_timeIntervals.count() != day.timeIntervals().count()) {
        //debugPlan<<m_timeIntervals.count()<<" !="<<day.timeIntervals().count();
        return false;
    }
    for (TimeInterval *a : std::as_const(m_timeIntervals)) {
        bool res = false;
        const auto intervals = day.timeIntervals();
        for (TimeInterval *b : intervals) {
            if (a == b) {
                res = true;
                break;
            }
        }
        if (res == false) {
            //debugPlan<<"interval mismatch"<<a->first.toString()<<"-"<<a->second.toString();
            return false;
        }
    }
    return true;
}
bool CalendarDay::operator!=(const CalendarDay *day) const {
    return operator!=(*day);
}
bool CalendarDay::operator!=(const CalendarDay &day) const {
    return !operator==(day);
}

Duration CalendarDay::effort(QTime start, int length, const QTimeZone &timeZone, Schedule *sch) {
//     debugPlan<<start<<" -"<<length;
    return effort(m_date, start, length, timeZone, sch);
}

Duration CalendarDay::effort(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch) {
//     debugPlan<<date<<start<<length;
    if (!date.isValid()) {
        return Duration::zeroDuration;
    }
    //debugPlan<<start.toString()<<" -"<<end.toString();
    Duration eff;
    if (m_state != Working) {
        //debugPlan<<"Non working day";
        return eff;
    }
    int l = 0;
    for (TimeInterval *i : std::as_const(m_timeIntervals)) {
        if (! i->endsMidnight() && start >= i->endTime()) {
            //debugPlan<<"Skip:"<<start<<">="<<i->first.addMSecs(i->second);
            continue;
        }
        QTime t1 = start.addMSecs(length);
        if (t1 != QTime(0, 0, 0) && t1 < i->first) {
            //debugPlan<<"Skip:"<<t1<<"<"<<i->first;
            continue;
        }
        t1 = qMax(start, i->first);
        if (i->endsMidnight()) {
            l = t1.msecsTo(QTime(23, 59, 59, 999)) + 1;
        } else {
            l = t1.msecsTo(i->endTime());
        }
        l = qMin(l, length - start.msecsTo(t1));
        if (l <= 0) {
            continue;
        }
        //debugPlan<<"Interval:"<<t1<<"->"<<l;
        DateTime dt1 = ! timeZone.isValid() ? DateTime(date, t1) : DateTime(date, t1, timeZone);
        DateTimeInterval dti(dt1, dt1.addMSecs(l));
        if (sch) {
            dti = sch->available(dti); //FIXME needs an effort method
            //debugPlan<<"Checked sch:"<<dti.first<<" -"<<dti.second;
        }
        eff += dti.second - dti.first;
        //debugPlan<<dti.first.toString()<<" -"<<dti.second.toString()<<", effort now"<<eff.toString();
    }
//     debugPlan<<(m_date.isValid()?m_date.toString(Qt::ISODate):"Weekday")<<":"<<start.toString()<<" -"<<start.addMSecs(length).toString()<<": total="<<eff.toDouble(Duration::Unit_h)<<"h";
    return eff;
}

Duration CalendarDay::workDuration() const
{
    Duration d;
    if (m_state != Working) {
        //debugPlan<<"Non working day";
        return d;
    }
    for (TimeInterval *i : std::as_const(m_timeIntervals)) {
        //debugPlan<<"Interval:"<<i->first<<" -"<<i->second;
        d += Duration((qint64)i->second);
    }
    return d;
}


TimeInterval CalendarDay::interval(QTime start, int length, const QTimeZone &timeZone, Schedule *sch) const {
    //debugPlan;
    return interval(m_date, start, length, timeZone, sch);
}

TimeInterval CalendarDay::interval(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch) const
{
    //debugPlan<<"Inp:"<<date<<start<<"+"<<length<<"="<<QDateTime(date, start).addMSecs(length);
    Q_ASSERT(length > 0);
    Q_ASSERT(QTime(0,0,0).msecsTo(start) + length <= 1000*60*60*24);
    QTime t1;
    int l = 0;
    if (! hasInterval()) {
        return TimeInterval();
    }
    for (TimeInterval *i : std::as_const(m_timeIntervals)) {
        //debugPlan<<"Interval:"<<i->first<<i->second<<i->first.addMSecs(i->second);
        if (! i->endsMidnight() && start >= i->endTime()) {
            //debugPlan<<"Skip:"<<start<<">="<<i->first.addMSecs(i->second);
            continue;
        }
        QTime t1 = start.addMSecs(length);
        if (t1 != QTime(0, 0, 0) && t1 < i->first) {
            //debugPlan<<"Skip:"<<t1<<"<"<<i->first;
            continue;
        }
        t1 = qMax(start, i->first);
        if (i->endsMidnight()) {
            l = t1.msecsTo(QTime(23, 59, 59, 999)) + 1;
        } else {
            l = t1.msecsTo(i->endTime());
        }
        l = qMin(l, length - start.msecsTo(t1));
        if (l <= 0) {
            continue;
        }
        TimeInterval ti(t1, l);
        //debugPlan<<"Day give:"<<date<<","<<t1<<"->"<<l;
        if (sch) {
            // check if booked
            DateTime dt1 = timeZone.isValid() ? DateTime(date, t1, timeZone) : DateTime(date, t1);
            QDate d2 = date;
            QTime t2 = t1.addMSecs(l);
            if (t2 == QTime(0, 0, 0)) {
                d2 = d2.addDays(1);
            }
            DateTime dt2 = timeZone.isValid() ? DateTime(d2, t2, timeZone) : DateTime(d2, t2);
            DateTimeInterval dti(dt1, dt2);
            //debugPlan<<": Booked?"<<date<<","<<t1<<"+"<<l<<"="<<t1.addMSecs(l)<<'\n'<<dti;
            dti = sch->available(dti);
            //debugPlan<<"Checked sch:"<<ti.first<<","<<ti.second<<"="<<dti;
            ti = TimeInterval(dti.first.time(), (dti.second - dti.first).milliseconds());
            //debugPlan<<"CalendarDay::interval:"<<ti;
        }
        if (ti.isValid()) {
            //debugPlan<<"Return:"<<ti.first<<"+"<<ti.second<<"="<<ti.first.addMSecs(ti.second);
            return ti;
        }
    }
    return TimeInterval(t1, l);
}

bool CalendarDay::hasInterval() const
{
    return m_state == Working && m_timeIntervals.count() > 0;
}

bool CalendarDay::hasInterval(QTime start, int length, const QTimeZone &timeZone, Schedule *sch) const {
    //debugPlan<<(m_date.isValid()?m_date.toString(Qt::ISODate):"Weekday")<<""<<start.toString()<<" -"<<end.toString();
    return hasInterval(m_date, start, length, timeZone, sch);
}

bool CalendarDay::hasInterval(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch) const
{
    //debugPlan<<(m_date.isValid()?m_date.toString(Qt::ISODate):"Weekday")<<""<<start<<"->"<<length;
    return interval(date, start, length, timeZone, sch).first.isValid();
}

Duration CalendarDay::duration() const {
    Duration dur;
    for (TimeInterval *i : std::as_const(m_timeIntervals)) {
        dur += Duration((qint64)i->second);
    }
    return dur;
}

void CalendarDay::removeInterval(TimeInterval *ti)
{
    m_timeIntervals.removeOne(ti);
}

int CalendarDay::numIntervals() const 
{
    return m_state == Working ? m_timeIntervals.count() : 0;
}

bool CalendarDay::hasInterval(const TimeInterval* interval) const
{
    return m_timeIntervals.contains(const_cast<TimeInterval*>(interval));
}

DateTime CalendarDay::start() const
{
    if (m_state != Working || m_timeIntervals.isEmpty()) {
        return DateTime();
    }
    QDate date = m_date;
    if (! m_date.isValid()) {
        date = QDate::currentDate();
    }
    if (m_calendar && m_calendar->timeZone().isValid()) {
        return DateTime(date, m_timeIntervals.first()->startTime(), m_calendar->timeZone());
    }
    return DateTime(date, m_timeIntervals.first()->startTime());
}

DateTime CalendarDay::end() const
{
    if (m_state != Working || m_timeIntervals.isEmpty()) {
        return DateTime();
    }
    QDate date;
    if (m_date.isValid()) {
        date = m_timeIntervals.last()->endsMidnight() ? m_date.addDays(1) : m_date;
    } else {
        date = QDate::currentDate();
    }
    if (m_calendar && m_calendar->timeZone().isValid()) {
        return DateTime(date, m_timeIntervals.last()->endTime(), m_calendar->timeZone());
    }
    return DateTime(date, m_timeIntervals.last()->endTime());
}

/////   CalendarWeekdays   ////
CalendarWeekdays::CalendarWeekdays()
    : m_weekdays()
{

    //debugPlan<<"--->";
    for (int i=1; i <= 7; ++i) {
        m_weekdays.insert(i, new CalendarDay());
    }
    //debugPlan<<"<---";
}

CalendarWeekdays::CalendarWeekdays(const CalendarWeekdays *weekdays)
    : m_weekdays() {
    //debugPlan<<"--->";
    copy(*weekdays);
    //debugPlan<<"<---";
}

CalendarWeekdays::~CalendarWeekdays() {
    qDeleteAll(m_weekdays);
    //debugPlan;
}

const CalendarWeekdays &CalendarWeekdays::copy(const CalendarWeekdays &weekdays) {
    //debugPlan;
    qDeleteAll(m_weekdays);
    m_weekdays.clear();
    QMapIterator<int, CalendarDay*> i(weekdays.weekdayMap());
    while (i.hasNext()) {
        i.next();
        m_weekdays.insert(i.key(), new CalendarDay(i.value()));
    }
    return *this;
}
#if 0
bool CalendarWeekdays::load(KoXmlElement &element, XMLLoaderObject &status) {
    //debugPlan;
    bool ok;
    int dayNo = QString(element.attribute(QStringLiteral("day"),QStringLiteral("-1"))).toInt(&ok);
    if (dayNo < 0 || dayNo > 6) {
        errorPlan<<"Illegal weekday: "<<dayNo;
        return true; // we continue anyway
    }
    CalendarDay *day = m_weekdays.value(dayNo + 1);
    if (day == nullptr) {
        errorPlan<<"No weekday: "<<dayNo;
        return false;
    }
    if (!day->load(element, status))
        day->setState(CalendarDay::None);
    return true;
}
#endif
void CalendarWeekdays::save(QDomElement &element) const {
    //debugPlan;
    QMapIterator<int, CalendarDay*> i(m_weekdays);
    while (i.hasNext()) {
        i.next();
        QDomElement me = element.ownerDocument().createElement(QStringLiteral("weekday"));
        element.appendChild(me);
        me.setAttribute(QStringLiteral("day-number"), QString::number(i.key() - 1)); // 0 (monday) .. 6 (sunday)
        i.value()->save(me);
    }
}

const QMap<int, CalendarDay*> &CalendarWeekdays::weekdayMap() const
{
    return m_weekdays;
}

IntMap CalendarWeekdays::stateMap() const
{
    IntMap days;
    QMapIterator<int, CalendarDay*> i(m_weekdays);
    while (i.hasNext()) {
        i.next();
        if (i.value()->state() != CalendarDay::None)
            days.insert(i.key(), i.value()->state());
    }
    return days;
}

int CalendarWeekdays::state(QDate date) const {
    return state(date.dayOfWeek());
}

int CalendarWeekdays::state(int weekday) const {
    CalendarDay *day = m_weekdays.value(weekday);
    return day ? day->state() : CalendarDay::None;
}

void CalendarWeekdays::setState(int weekday, int state) {
    CalendarDay *day = m_weekdays.value(weekday);
    if (day == nullptr)
        return;
    day->setState(state);
}

QList<TimeInterval*> CalendarWeekdays::intervals(int weekday) const { 
    CalendarDay *day = m_weekdays.value(weekday);
    Q_ASSERT(day);
    return day->timeIntervals();
}

void CalendarWeekdays::setIntervals(int weekday, const QList<TimeInterval*> &intervals) {
    CalendarDay *day = m_weekdays.value(weekday);
    if (day) {
        day->setIntervals(intervals);

    }
}

void CalendarWeekdays::clearIntervals(int weekday) {
    CalendarDay *day = m_weekdays.value(weekday);
    if (day) {
        day->clearIntervals();
    }
}

bool CalendarWeekdays::operator==(const CalendarWeekdays *wd) const {
    if (m_weekdays.count() != wd->weekdays().count()) {
        return false;
    }
    QMapIterator<int, CalendarDay*> i(wd->weekdayMap());
    while (i.hasNext()) {
        i.next();
        CalendarDay *day1 = i.value();
        CalendarDay *day2 = m_weekdays.value(i.key());
        if (day1 != day2)
            return false;
    }
    return true;
}
bool CalendarWeekdays::operator!=(const CalendarWeekdays *wd) const {
    return operator==(wd) == false;
}

Duration CalendarWeekdays::effort(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch) {
//     debugPlan<<"Day of week="<<date.dayOfWeek();
    Q_ASSERT(QTime(0,0,0).msecsTo(start) + length <= 1000*60*60*24);
    CalendarDay *day = weekday(date.dayOfWeek());
    if (day && day->state() == CalendarDay::Working) {
        return day->effort(date, start, length, timeZone, sch);
    }
    return Duration::zeroDuration;
}

TimeInterval CalendarWeekdays::interval(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch) const
{
    //debugPlan;
    CalendarDay *day = weekday(date.dayOfWeek());
    if (day && day->state() == CalendarDay::Working) {
        return day->interval(date, start, length, timeZone, sch);
    }
    return TimeInterval();
}

bool CalendarWeekdays::hasInterval(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch) const
{
    //debugPlan<<date<<":"<<start<<"+"<<length;
    CalendarDay *day = weekday(date.dayOfWeek());
    return day && day->hasInterval(date, start, length, timeZone, sch);
}

bool CalendarWeekdays::hasInterval() const
{
    //debugPlan;
    for (CalendarDay *d : std::as_const(m_weekdays)) {
        if (d->hasInterval())
            return true;
    }
    return false;
}

CalendarDay *CalendarWeekdays::weekday(int day) const {
    Q_ASSERT(day >= 1 && day <= 7);
    Q_ASSERT(m_weekdays.contains(day));
    return m_weekdays.value(day);
}

//static
int CalendarWeekdays::dayOfWeek(const QString& name)
{
    QStringList lst;
    lst << QStringLiteral("Monday") << QStringLiteral("Tuesday") << QStringLiteral("Wednesday") << QStringLiteral("Thursday") << QStringLiteral("Friday") << QStringLiteral("Saturday") << QStringLiteral("Sunday");
    int idx = -1;
    if (lst.contains(name)) {
        idx = lst.indexOf(name) + 1;
    }
    return idx;
}

Duration CalendarWeekdays::duration() const {
    Duration dur;
    for (CalendarDay *d : std::as_const(m_weekdays)) {
        dur += d->duration();
    }
    return dur;
}

Duration CalendarWeekdays::duration(int _weekday) const {
    CalendarDay *day = weekday(_weekday);
    if (day)
        return day->duration();
    return Duration();
}

int CalendarWeekdays::indexOf(const CalendarDay *day) const
{
    return m_weekdays.key(const_cast<CalendarDay*>(day));
}

/////   Calendar   ////

Calendar::Calendar()
    : QObject(nullptr), // don't use parent
      m_parent(nullptr),
      m_project(nullptr),
      m_default(false),
      m_shared(false)
{
    init();
}

Calendar::Calendar(const QString& name, Calendar *parent)
    : QObject(nullptr), // don't use parent
      m_name(name),
      m_parent(parent),
      m_project(nullptr),
      m_days(),
      m_default(false),
      m_shared(false)
{
    init();
}

Calendar::~Calendar() {
    //debugPlan<<"deleting"<<m_name;
    removeId();
#ifdef HAVE_KHOLIDAYS
    delete m_region;
#endif
    delete m_weekdays;
    while (!m_days.isEmpty())
        delete m_days.takeFirst();
}
// Not allowed, QObject
// Calendar::Calendar(Calendar *calendar)
//     : m_project(0),
//       m_days() {
//     copy(*calendar);
// }

void Calendar::copy(const Calendar *calendar)
{
    copy(*calendar);
}

const Calendar &Calendar::copy(const Calendar &calendar) {
    m_name = calendar.name();
    m_timeZone = calendar.timeZone();
    // m_parent = calendar.parentCal(); 
    // m_id = calendar.id();
#ifdef HAVE_KHOLIDAYS
    delete m_region;
    m_region = new KHolidays::HolidayRegion(calendar.holidayRegion()->regionCode());
    m_regionCode = calendar.m_regionCode;
#endif
    const auto days = calendar.days();
    for (CalendarDay *d : days) {
        m_days.append(new CalendarDay(d));
    }
    delete m_weekdays;
    m_weekdays = new CalendarWeekdays(calendar.weekdays());
    return *this;
}

void Calendar::init() {
#ifdef HAVE_KHOLIDAYS
    m_region = new KHolidays::HolidayRegion();
#endif
    m_weekdays = new CalendarWeekdays();
    m_timeZone = QTimeZone::systemTimeZone();
    m_cacheversion = 0;
    m_blockversion = false;
}

int Calendar::cacheVersion() const
{
    return m_parent ? m_parent->cacheVersion() : m_cacheversion;
}

void Calendar::incCacheVersion()
{
    if (m_blockversion) {
        return;
    }
    if (m_parent) {
        m_parent->incCacheVersion();
    } else {
        ++m_cacheversion;
    }
}

void Calendar::setCacheVersion(int version)
{
    if (m_blockversion) {
        return;
    }
    if (m_parent) {
        m_parent->setCacheVersion(version);
    } else {
        m_cacheversion = version;
        debugPlan<<m_name<<m_cacheversion;
    }
}

void Calendar::setName(const QString& name)
{
    m_name = name;
    if (m_project) {
        m_project->changed(this);
    }
}

void Calendar::setParentCal(Calendar *parent, int pos)
{
    if (m_parent) {
        m_parent->takeCalendar(this);
    }
    m_parent = parent;
    if (m_parent) {
        m_parent->addCalendar(this, pos);
    }
    
}

bool Calendar::isChildOf(const Calendar *cal) const
{
    Calendar *p = parentCal();
    for (; p != nullptr; p = p->parentCal()) {
        if (cal == p) {
            return true;
        }
    }
    return false;
}

void Calendar::setProject(Project *project) { 
    m_project = project;
}

void Calendar::setTimeZone(const QTimeZone &tz)
{
    if (m_timeZone == tz) {
        return;
    }
    //debugPlan<<tz->name();
    m_timeZone = tz;
#ifdef HAVE_KHOLIDAYS
    if (m_regionCode == QStringLiteral("Default")) {
        setHolidayRegion(QStringLiteral("Default"));
    }
#endif
    if (m_project) {
        m_project->changed(this);
    }
    incCacheVersion();
}

QTimeZone Calendar::projectTimeZone() const
{
    return m_project ? m_project->timeZone() : QTimeZone::systemTimeZone();
}

void Calendar::setDefault(bool on)
{
    m_default = on;
    if (m_project) {
        m_project->changed(this);
    }
    incCacheVersion();
}

// Note: only project should do this
void Calendar::setId(const QString& id) {
    //debugPlan<<id;
    m_id = id;
}

void Calendar::addCalendar(Calendar *calendar, int pos)
{
    if (pos == -1) {
        m_calendars.append(calendar);
    } else {
        m_calendars.insert(pos, calendar);
    }
    calendar->setTimeZone(m_timeZone);
}

void Calendar::takeCalendar(Calendar *calendar)
{
    int i = indexOf(calendar);
    if (i != -1) {
        m_calendars.removeAt(i);
    }
}

int Calendar::indexOf(const Calendar *calendar) const
{
    return m_calendars.indexOf(const_cast<Calendar*>(calendar));
}

bool Calendar::loadCacheVersion(KoXmlElement &element, XMLLoaderObject &status)
{
    Q_UNUSED(status);
    m_cacheversion = element.attribute(QStringLiteral("version")).toInt();
    debugPlan<<m_name<<m_cacheversion;
    return true;
}

void Calendar::saveCacheVersion(QDomElement &element) const
{
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("cache"));
    element.appendChild(me);
    me.setAttribute(QStringLiteral("version"), QString::number(m_cacheversion));
}
#if 0
bool Calendar::load(KoXmlElement &element, XMLLoaderObject &status) {
    //bool ok;
    m_blockversion = true;
    setId(element.attribute(QStringLiteral("id")));
    m_parentId = element.attribute(QStringLiteral("parent"));
    m_name = element.attribute(QStringLiteral("name"),QStringLiteral(""));
    QTimeZone tz(element.attribute(QStringLiteral("timezone")).toLatin1());
    if (tz.isValid()) {
        setTimeZone(tz);
    } else warnPlan<<"No timezone specified, use default (local)";
    m_default = (bool)element.attribute(QStringLiteral("default"),QStringLiteral("0")).toInt();
    if (m_default) {
        status.project().setDefaultCalendar(this);
    }
    if (status.version() < QStringLiteral("0.7.0")) {
        m_shared = element.attribute(QStringLiteral("shared"), QStringLiteral("0")).toInt();
    } else {
        m_shared = element.attribute(QStringLiteral("origin"), QStringLiteral("local")) != QStringLiteral("local");
    }

#ifdef HAVE_KHOLIDAYS
    setHolidayRegion(element.attribute(QStringLiteral("holiday-region")));
#endif

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("weekday")) {
            if (!m_weekdays->load(e, status))
                return false;
        }
        if (e.tagName() == QStringLiteral("day")) {
            CalendarDay *day = new CalendarDay();
            if (day->load(e, status)) {
                if (!day->date().isValid()) {
                    delete day;
                    errorPlan<<m_name<<": Failed to load calendarDay - Invalid date";
                } else {
                    CalendarDay *d = findDay(day->date());
                    if (d) {
                        // already exists, keep the new
                        delete takeDay(d);
                        warnPlan<<m_name<<" Load calendarDay - Date already exists";
                    }
                    addDay(day);
                }
            } else {
                delete day;
                errorPlan<<"Failed to load calendarDay";
                return true; //false; don't throw away the whole calendar
            }
        }
    }
    // this must go last
    KoXmlElement e = element.namedItem(QStringLiteral("cache")).toElement();
    if (! e.isNull()) {
        loadCacheVersion(e, status);
    }
    m_blockversion = false;
    return true;
}
#endif
void Calendar::save(QDomElement &element) const {
    //debugPlan<<m_name;
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("calendar"));
    element.appendChild(me);
    if (m_parent) {
        me.setAttribute(QStringLiteral("parent"), m_parent->id());
    }
    me.setAttribute(QStringLiteral("name"), m_name);
    me.setAttribute(QStringLiteral("id"), m_id);
    if (m_default) {
        me.setAttribute(QStringLiteral("default"), QString::number(m_default));
    }
    me.setAttribute(QStringLiteral("timezone"), m_timeZone.isValid() ? QString::fromLatin1(m_timeZone.id()) : QString());
    m_weekdays->save(me);
    for (CalendarDay *d : std::as_const(m_days)) {
        QDomElement e = me.ownerDocument().createElement(QStringLiteral("day"));
        me.appendChild(e);
        d->save(e);
    }
    me.setAttribute(QStringLiteral("origin"), m_shared ? QStringLiteral("shared") : QStringLiteral("local"));

#ifdef HAVE_KHOLIDAYS
    me.setAttribute(QStringLiteral("holiday-region"), m_regionCode);
#endif

    saveCacheVersion(me);
}

int Calendar::state(QDate date) const
{
    if (!date.isValid()) {
        return CalendarDay::Undefined;
    }
    CalendarDay *day = findDay(date);
    if (day && day->state() != CalendarDay::Undefined) {
        return day->state();
    }
#ifdef HAVE_KHOLIDAYS
    if (isHoliday(date)) {
        return CalendarDay::NonWorking;
    }
#endif
    day = weekday(date.dayOfWeek());
    if (day && day->state() != CalendarDay::Undefined) {
        return day->state();
    }
    return m_parent ? m_parent->state(date) : CalendarDay::Undefined;
}

CalendarDay *Calendar::findDay(QDate date, bool skipUndefined) const {
    //debugPlan<<date.toString();
    for (CalendarDay *d : std::as_const(m_days)) {
        if (d->date() == date) {
            if (skipUndefined  && d->state() == CalendarDay::Undefined) {
                continue; // hmmm, break?
            }
            return d;
        }
    }
    //debugPlan<<date.toString()<<" not found";
    return nullptr;
}

void Calendar::setState(CalendarDay *day, CalendarDay::State state)
{
    day->setState(state);
    Q_EMIT calendarDayChanged(day);
    incCacheVersion();
}

void Calendar::addWorkInterval(CalendarDay *day, TimeInterval *ti)
{
    Q_EMIT workIntervalToBeAdded(day, ti, day->numIntervals());
    day->addInterval(ti);
    Q_EMIT workIntervalAdded(day, ti);
    incCacheVersion();
}

void Calendar::takeWorkInterval(CalendarDay *day, TimeInterval *ti)
{
    if (!day->hasInterval(ti)) {
        return;
    }
    Q_EMIT workIntervalToBeRemoved(day, ti);
    day->removeInterval(ti);
    Q_EMIT workIntervalRemoved(day, ti);
    incCacheVersion();
    return;
}

void Calendar::setWorkInterval(TimeInterval *ti, const TimeInterval &value)
{
    *ti = value;
    Q_EMIT timeIntervalChanged(ti);
    incCacheVersion();
}

void Calendar::setDate(CalendarDay *day, QDate date)
{
    day->setDate(date);
    Q_EMIT calendarDayChanged(day);
    incCacheVersion();
}

CalendarDay *Calendar::day(QDate date) const
{
    for (CalendarDay *d : std::as_const(m_days)) {
        if (d->date() == date) {
            return d;
        }
    }
    return nullptr;
}

IntMap Calendar::weekdayStateMap() const
{
    return m_weekdays->stateMap();
}

void Calendar::setWeekday(int dayno, const CalendarDay &day)
{
    if (dayno < 1 || dayno > 7) {
        return;
    }
    CalendarDay *wd = weekday(dayno);
    while (! wd->timeIntervals().isEmpty()) {
        TimeInterval *ti = wd->timeIntervals().constLast();
        Q_EMIT workIntervalToBeRemoved(wd, ti);
        wd->removeInterval(ti);
        Q_EMIT workIntervalRemoved(wd, ti);
    }
    wd->setState(day.state());
    Q_EMIT calendarDayChanged(wd);
    const auto intervals = day.timeIntervals();
    for (TimeInterval *ti : intervals) {
        TimeInterval *t = new TimeInterval(*ti);
        Q_EMIT workIntervalToBeAdded(wd, t, wd->numIntervals()); // hmmmm
        wd->addInterval(t);
        Q_EMIT workIntervalAdded(wd, t);
    }
    incCacheVersion();
}

bool Calendar::hasParent(Calendar *cal) {
    //debugPlan;
    if (!m_parent)
        return false;
    if (m_parent == cal)
        return true;
    return m_parent->hasParent(cal);
}

AppointmentIntervalList Calendar::workIntervals(const QDateTime &start, const QDateTime &end, double load) const
{
    const auto tz = start.timeZone();
    const auto limit = end.toTimeZone(start.timeZone());
    debugPlan<<tz<<start<<end<<load;
    AppointmentIntervalList lst;
    TimeInterval res;
    QTime startTime = start.time();
    int length = 0;
    if (start.date() == limit.date()) {
        // Handle single day
        length = startTime.msecsTo(limit.time());
        if (length <= 0) {
            warnPlan<<"Invalid length"<<length;
            return lst;
        }
        //debugPlan<<"Check single day:"<<s.date()<<s.time()<<length;
        res = firstInterval(start.date(), startTime, length, nullptr);
        while (res.isValid()) {
            const DateTime dt1 = DateTime(start.date(), res.startTime(), m_timeZone).toTimeZone(tz);
            const DateTime dt2 = dt1.addMSecs(res.second).toTimeZone(tz);
            lst.add(AppointmentInterval(dt1, dt2, load));
            length -= res.second;
            if (length <= 0 || res.endsMidnight()) {
                break;
            }
            res = firstInterval(start.date(), res.endTime(), length, nullptr);
        }
        //debugPlan<<lst;
        return lst;
    }
    //debugPlan<<"tospec:"<<s.toString()<<" -"<<e.toString();
    // Multiple days
    for (QDate date = start.date(); date <= end.date(); date = date.addDays(1)) {
        if (date > start.date()) {
            startTime = QTime(0, 0, 0);
        }
        if (date < end.date()) {
            length = startTime.msecsTo(QTime(23, 59, 59, 999)) + 1;
        } else {
            length = startTime.msecsTo(end.time());
        }
        if (length <= 0) {
            break;
        }
        res = firstInterval(date, startTime, length);
        while (res.isValid()) {
            //debugPlan<<"interval:"<<date<<startTime<<'='<<res.first<<res.second;
            DateTime dt1(date, res.startTime(), m_timeZone);
            DateTime dt2(date, res.endTime(), m_timeZone);
            if (res.endsMidnight()) {
                dt2 = dt2.addDays(1);
            }
            AppointmentInterval i(dt1.toTimeZone(tz), dt2.toTimeZone(tz), load);
            lst.add(i);
            debugPlan<<res<<dt1<<dt2<<lst;
            length -= startTime.msecsTo(res.endTime());
            if (length <= 0 || res.endsMidnight()) {
                break;
            }
            startTime = res.endTime();
            res = firstInterval(date, startTime, length, nullptr);
        }
    }
    //debugPlan<<lst;
    return lst;
}

AppointmentIntervalList Calendar::workIntervals(const DateTime &start, const DateTime &end, double load) const
{
//    debugPlan<<start<<end<<load;
    AppointmentIntervalList lst;
    if (!start.isValid()) {
        warnPlan<<"Invalid start time";
        return lst;
    }
    if (!end.isValid()) {
        warnPlan<<"Invalid end time";
        return lst;
    }
    if (start >= end) {
        warnPlan<<"Invalid interval";
        return lst;
    }
    Q_ASSERT(m_timeZone.isValid());
    QDateTime zonedStart = start.toTimeZone(m_timeZone);
    QDateTime zonedEnd = end.toTimeZone(m_timeZone);
    Q_ASSERT(zonedStart.isValid() && zonedEnd.isValid());
    // convert to starts timezone

    return workIntervals(zonedStart, zonedEnd, load);
}

Duration Calendar::effort(QDate date, QTime start, int length, Schedule *sch) const {
//     debugPlan<<m_name<<":"<<date<<""<<start<<"->"<<length;
    if (length <= 0) {
        return Duration::zeroDuration;
    }
    // first, check my own day
    CalendarDay *day = findDay(date, true);
    if (day) {
        if (day->state() == CalendarDay::Working) {
            return day->effort(start, length, m_timeZone, sch);
        } else if (day->state() == CalendarDay::NonWorking) {
            return Duration::zeroDuration;
        } else {
            errorPlan<<"Invalid state: "<<day->state();
            return Duration::zeroDuration;
        }
    }
#ifdef HAVE_KHOLIDAYS
    if (isHoliday(date)) {
        return Duration::zeroDuration;
    }
#endif
    // check my own weekdays
    if (m_weekdays) {
        if (m_weekdays->state(date) == CalendarDay::Working) {
            return m_weekdays->effort(date, start, length, m_timeZone, sch);
        }
        if (m_weekdays->state(date) == CalendarDay::NonWorking) {
            return Duration::zeroDuration;
        }
    }
    if (m_parent) {
        return m_parent->effort(date, start, length, sch);
    }
    return Duration::zeroDuration;
}

Duration Calendar::effort(const QDateTime &start, const QDateTime &end, Schedule *sch) const {
//     debugPlan<<m_name<<":"<<start<<"to"<<end;
    Duration eff;
    QDate date = start.date();
    QTime startTime = start.time();
    QTime endTime = end.time();
    int length = 0;
    if (date == end.date()) {
        // single day
        length = startTime.msecsTo(endTime);
        return effort(date, startTime, length, sch);
    }
    length = startTime.msecsTo(QTime(23, 59, 59, 999)) + 1;
    QTime t0(0, 0, 0);
    int aday = t0.msecsTo(QTime(23, 59, 59, 999)) + 1;
    eff = effort(date, startTime, length, sch); // first day
    // Now get all the rest of the days
    for (date = date.addDays(1); date <= end.date(); date = date.addDays(1)) {
        if (date < end.date()) {
            eff += effort(date, t0, aday, sch); // whole days
        } else if (endTime > t0) {
            eff += effort(date, t0, t0.msecsTo(endTime), sch); // last day
        }
        //debugPlan<<": eff now="<<eff.toString(Duration::Format_Day);
    }
    //debugPlan<<start<<"-"<<end<<": total="<<eff.toString();
    return eff;
}

Duration Calendar::effort(const DateTime &start, const DateTime &end, Schedule *sch) const {
//     debugPlan<<m_name<<":"<<start<<start.timeSpec()<<"to"<<end<<end.timeSpec();
    Duration eff;
    if (!start.isValid() || !end.isValid() || end < start) {
        if (sch && sch->resource()) debugPlan<<sch->resource()->name()<<sch->name()<<"Available:"<<sch->resource()->availableFrom()<<sch->resource()->availableUntil();
        errorPlan<<"Illegal datetime: "<<start<<", "<<end;
        return eff;
    }
    if (start == end) {
        //debugPlan<<"start == end";
        return eff;
    }
    Q_ASSERT(m_timeZone.isValid());
    QDateTime zonedStart = start.toTimeZone(m_timeZone);
    QDateTime zonedEnd = end.toTimeZone(m_timeZone);
    Q_ASSERT(zonedStart.isValid() && zonedEnd.isValid());
    return effort(zonedStart, zonedEnd, sch);
}


TimeInterval Calendar::firstInterval(QDate date, QTime startTime, int length, Schedule *sch) const {
    //debugPlan;
    CalendarDay *day = findDay(date, true);
    if (day) {
        return day->interval(startTime, length, m_timeZone, sch);
    }
#ifdef HAVE_KHOLIDAYS
    if (isHoliday(date)) {
        return TimeInterval();
    }
#endif
    if (m_weekdays) {
        if (m_weekdays->state(date) == CalendarDay::Working) {
            //debugPlan<<"Check weekday";
            TimeInterval i = m_weekdays->interval(date, startTime, length, m_timeZone, sch);
            //debugPlan<<"Checked weekday, got"<<i;
            return i;
        }
        if (m_weekdays->state(date) == CalendarDay::NonWorking) {
            return TimeInterval();
        }
    }
    if (m_parent) {
        //debugPlan<<"Check parent";
        return m_parent->firstInterval(date, startTime, length, sch);
    }
    return TimeInterval();
}

DateTimeInterval Calendar::firstInterval(const QDateTime &start, const QDateTime &end, Schedule *sch) const
{
    TimeInterval res;
    QTime startTime = start.time();
    int length = 0;
    if (start.date() == end.date()) {
        // Handle single day
        length = startTime.msecsTo(end.time());
        if (length <= 0) {
            warnPlan<<"Invalid length"<<length;
            return DateTimeInterval();
        }
        //debugPlan<<"Check single day:"<<s.date()<<s.time()<<length;
        res = firstInterval(start.date(), startTime, length, sch);
        if (! res.isValid()) {
            return DateTimeInterval();
        }
        DateTime dt1(start.date(), res.first, m_timeZone);
        DateTimeInterval dti(dt1, DateTime(dt1.addMSecs(res.second)).toTimeZone(m_timeZone));
//         debugPlan<<"Result firstInterval:"<<dti;
        return dti;
    }
    //debugPlan<<"tospec:"<<s.toString()<<" -"<<e.toString();
    // Multiple days
    for (QDate date = start.date(); date <= end.date(); date = date.addDays(1)) {
        if (date > start.date()) {
            startTime = QTime(0, 0, 0);
        }
        if (date < end.date()) {
            length = startTime.msecsTo(QTime(23, 59, 59, 999)) + 1;
        } else {
            length = startTime.msecsTo(end.time());
        }
        if (length <= 0) {
            break;
        }
        //debugPlan<<"Check:"<<date<<startTime<<"+"<<length<<"="<<startTime.addMSecs(length);
        res = firstInterval(date, startTime, length, sch);
        if (res.isValid()) {
            //debugPlan<<"inp:"<<start<<"-"<<end;
            //debugPlan<<"Found an interval ("<<date<<","<<res.first<<","<<res.second<<")";
            // return result in callers timezone
            DateTime dt1(date, res.first, m_timeZone);
            DateTimeInterval dti(DateTime(dt1), dt1.addMSecs(res.second).toTimeZone(m_timeZone));
//             debugPlan<<"Result firstInterval:"<<dti;
            return dti;
        }
    }
    //warnPlan<<"Didn't find an interval ("<<start<<", "<<end<<")";
    return DateTimeInterval();
}

DateTimeInterval Calendar::firstInterval(const DateTime &start, const DateTime &end, Schedule *sch) const
{
    //debugPlan<<"inp:"<<start.toString()<<" -"<<end.toString();
    if (!start.isValid()) {
        warnPlan<<"Invalid start time";
        return DateTimeInterval(DateTime(), DateTime());
    }
    if (!end.isValid()) {
        warnPlan<<"Invalid end time";
        return DateTimeInterval(DateTime(), DateTime());
    }
    if (start >= end) {
        warnPlan<<"Invalid interval"<<start<<end<<":"<<start<<end;
        return DateTimeInterval();
    }
    Q_ASSERT(m_timeZone.isValid());
    QDateTime zonedStart = start.toTimeZone(m_timeZone);
    QDateTime zonedEnd = end.toTimeZone(m_timeZone);
    Q_ASSERT(zonedStart.isValid() && zonedEnd.isValid());
    return firstInterval(zonedStart, zonedEnd, sch);
}


bool Calendar::hasInterval(QDate date, QTime startTime, int length, Schedule *sch) const
{
    //debugPlan;
    return ! firstInterval(date, startTime, length, sch).first.isNull();
}

bool Calendar::hasInterval(const DateTime &start, const DateTime &end, Schedule *sch) const {
    //debugPlan;
    return ! firstInterval(start, end, sch).first.isNull();
}

DateTime Calendar::firstAvailableAfter(const DateTime &time, const DateTime &limit, Schedule *sch) {
    //debugPlan<<m_name<<": check from"<<time<<" limit="<<limit;
    if (!time.isValid() || !limit.isValid() || time > limit) {
        errorPlan<<"Invalid input: "<<(time.isValid()?"":"(time invalid) ")<<(limit.isValid()?"":"(limit invalid) ")<<(time>limit?"":"(time>limit)");
        return DateTime();
    }
    if (time == limit) {
        return DateTime();
    }
    Q_ASSERT(m_timeZone.isValid());
    QDateTime zonedTime = time.toTimeZone(m_timeZone);
    QDateTime zonedLimit = limit.toTimeZone(m_timeZone);
    Q_ASSERT(zonedTime.isValid() && zonedLimit.isValid());
    return firstInterval(zonedTime, zonedLimit, sch).first;
}

DateTime Calendar::firstAvailableBefore(const QDateTime &time, const QDateTime &limit, Schedule *sch) {
    //debugPlan<<m_name<<"check from"<<time<<"limit="<<limit;
    if (!time.isValid() || !limit.isValid()) {
        warnPlan<<"Calendar::firstAvailableBefore:"<<"Invalid datetimes";
        return DateTime();
    }
    Q_ASSERT(time.timeZone() == m_timeZone);
    QDateTime lmt = time;
    QDateTime t = QDateTime(time.date(), QTime(0, 0, 0), m_timeZone); // start of first day
    if (t == lmt) {
        t = t.addDays(-1); // in case time == start of day
    }
    if (t < limit) {
        t = limit;  // always stop at limit (lower boundary)
    }
    //debugPlan<<m_name<<":"<<time<<limit<<t<<lmt;
    QDateTime res;
    //debugPlan<<m_name<<": t="<<t<<","<<lmt<<" limit="<<limit;
    while (!res.isValid() && t >= limit) {
        // check intervals for 1 day
        QDateTime r = firstInterval(t, lmt, sch).second.toTimeZone(m_timeZone);
        res = r;
        // Find the last interval
        while(r.isValid() && r < lmt) {
            r = firstInterval(r, lmt, sch).second.toTimeZone(m_timeZone);
            if (r.isValid()) {
                res = r;
            }
            //debugPlan<<m_name<<": r="<<r<<","<<lmt<<" res="<<res;
        }
        if (!res.isValid()) {
            if (t == limit) {
                break;
            }
            lmt = t;
            t = t.addDays(-1);
            if (t < limit) {
                t = limit;
            }
            if (t == lmt)
                break;
        }
    }
    DateTime result(res.toTimeZone(projectTimeZone()));
    //debugPlan<<m_name<<res<<res.dateTime().timeSpec()<<result<<result.timeSpec();
    return result; // return in local timezone
}

DateTime Calendar::firstAvailableBefore(const DateTime &time, const DateTime &limit, Schedule *sch) {
    //debugPlan<<m_name<<"check from"<<time<<time.timeSpec()<<" limit="<<limit<<limit.timeSpec();
    if (!time.isValid() || !limit.isValid() || time < limit) {
        errorPlan<<"Invalid input: "<<(time.isValid()?"":"(time invalid) ")<<(limit.isValid()?"":"(limit invalid) ")<<(time<limit?"":"(time<limit)");
        return DateTime();
    }
    if (time == limit) {
        return DateTime();
    }
    Q_ASSERT(m_timeZone.isValid());
    return firstAvailableBefore(time.toTimeZone(m_timeZone), limit.toTimeZone(m_timeZone), sch);
}

Calendar *Calendar::findCalendar(const QString &id) const { 
    return (m_project ? m_project->findCalendar(id) : nullptr); 
}

bool Calendar::removeId(const QString &id) { 
    return (m_project ? m_project->removeCalendarId(id) : false); 
}

void Calendar::insertId(const QString &id){ 
    if (m_project)
        m_project->insertCalendarId(id, this); 
}

void Calendar::addDay(CalendarDay *day)
{
    Q_EMIT dayToBeAdded(day, 0);
    m_days.insert(0, day);
    Q_EMIT dayAdded(day);
    incCacheVersion();
}

CalendarDay *Calendar::takeDay(CalendarDay *day)
{
    int i = m_days.indexOf(day);
    if (i == -1) {
        return nullptr;
    }
    Q_EMIT dayToBeRemoved(day);
    m_days.removeAt(i);
    Q_EMIT dayRemoved(day);
    incCacheVersion();
    return day;
}

QList<std::pair<CalendarDay*, CalendarDay*> > Calendar::consecutiveVacationDays() const
{
    QList<std::pair<CalendarDay*, CalendarDay*> > lst;
    std::pair<CalendarDay*, CalendarDay*> interval(nullptr, nullptr);
    for (CalendarDay* day : std::as_const(m_days)) {
        if (day->state() == CalendarDay::NonWorking) {
            if (interval.first == nullptr) {
                interval.first = day;
            }
            interval.second = day;
        } else {
            if (interval.first != nullptr) {
                lst << std::pair<CalendarDay*, CalendarDay*>(interval);
            }
            interval.first = interval.second = nullptr;
        }
    }
    return lst;
}

QList<CalendarDay*> Calendar::workingDays() const
{
    QList<CalendarDay*> lst;
    for (CalendarDay* day : std::as_const(m_days)) {
        if (day->state() == CalendarDay::Working) {
            lst << day;
        }
    }
    return lst;
}

bool Calendar::isShared() const
{
    return m_shared;
}

void Calendar::setShared(bool on)
{
    m_shared = on;
}

#ifdef HAVE_KHOLIDAYS
bool Calendar::isHoliday(QDate date) const
{
    if (m_region->isValid()) {
        KHolidays::Holiday::List lst = m_region->rawHolidaysWithAstroSeasons(date);
        if (!lst.isEmpty() && lst.first().dayType() != KHolidays::Holiday::Workday) {
            return true;
        }
    }
    return false;
}

KHolidays::HolidayRegion *Calendar::holidayRegion() const
{
    return m_region;
}

void Calendar::setHolidayRegion(const QString &code)
{
    delete m_region;
    m_regionCode = code;
    if (code == QStringLiteral("Default")) {
        QString country;
        if (m_timeZone.isValid()) {
            // TODO be more accurate when country has multiple timezones/regions
            country = tzZones->value(m_timeZone.id()).country;
        }
        m_region = new KHolidays::HolidayRegion(KHolidays::HolidayRegion::defaultRegionCode(country));
    } else {
        m_region = new KHolidays::HolidayRegion(code);
    }
    debugPlan<<code<<"->"<<m_regionCode<<m_region->isValid();
    Q_EMIT calendarDayChanged(static_cast<CalendarDay*>(nullptr));
    if (m_project) {
        m_project->changed(this);
    }
}

QString Calendar::holidayRegionCode() const
{
    return m_regionCode;
}

QStringList Calendar::holidayRegionCodes() const
{
    QStringList lst = KHolidays::HolidayRegion::regionCodes();
    lst.removeDuplicates();
    return lst;
}
#endif

/////////////
StandardWorktime::StandardWorktime(Project *project)
    : m_project(project)
 {
    init();
}

StandardWorktime::StandardWorktime(StandardWorktime *worktime) {
    if (worktime) {
        m_year = worktime->durationYear();
        m_month = worktime->durationMonth();
        m_week = worktime->durationWeek();
        m_day = worktime->durationDay();
    } else {
        init();
    }
}

StandardWorktime::~StandardWorktime()
{
    //debugPlan<<"("<<this<<")";
}

void StandardWorktime::init() {
    // Some sane default values
    m_year = Duration(0, 1760, 0);
    m_month = Duration(0, 176, 0);
    m_week = Duration(0, 40, 0);
    m_day = Duration(0, 8, 0);
}

void StandardWorktime::changed()
{
    if (m_project) {
        m_project->changed(this);
    }
}

QList<qint64> StandardWorktime::scales() const
{
    return QList<qint64>() << m_year.milliseconds() << m_month.milliseconds() << m_week.milliseconds() << m_day.milliseconds() << 60*60*1000 << 60*1000 << 1000 << 1;
}
#if 0
bool StandardWorktime::load(KoXmlElement &element, XMLLoaderObject &status) {
    //debugPlan;
    m_year = Duration::fromString(element.attribute(QStringLiteral("year")), Duration::Format_Hour); 
    m_month = Duration::fromString(element.attribute(QStringLiteral("month")), Duration::Format_Hour); 
    m_week = Duration::fromString(element.attribute(QStringLiteral("week")), Duration::Format_Hour); 
    m_day = Duration::fromString(element.attribute(QStringLiteral("day")), Duration::Format_Hour); 
    
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("calendar")) {
            // pre 0.6 version stored base calendar in standard worktime
            if (status.version() >= QStringLiteral("0.6")) {
                warnPlan<<"Old format, calendar in standard worktime";
                warnPlan<<"Tries to load anyway";
            }
            // try to load anyway
            Calendar *calendar = new Calendar;
            if (calendar->load(e, status)) {
                status.project().addCalendar(calendar);
                calendar->setDefault(true);
                status.project().setDefaultCalendar(calendar); // hmmm
                status.setBaseCalendar(calendar);
            } else {
                delete calendar;
                errorPlan<<"Failed to load calendar";
            }
        }
    }
    return true;
}
#endif
void StandardWorktime::save(QDomElement &element) const {
    //debugPlan;
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("standard-worktime"));
    element.appendChild(me);
    me.setAttribute(QStringLiteral("year"), m_year.toString(Duration::Format_Hour));
    me.setAttribute(QStringLiteral("month"), m_month.toString(Duration::Format_Hour));
    me.setAttribute(QStringLiteral("week"), m_week.toString(Duration::Format_Hour));
    me.setAttribute(QStringLiteral("day"), m_day.toString(Duration::Format_Hour));
    
}


}  //KPlato namespace

QString stateToString(int state)
{
    const QStringList lst = QStringList() << QStringLiteral("U") << QStringLiteral("N") << QStringLiteral("W");
    return lst.value(state);
}
QDebug operator<<(QDebug dbg, KPlato::Calendar *c)
{
    if (!c) {
        return dbg << "Calendar[0x0]";
    }
    dbg << "Calendar[" << c->name();
    for (int i = 1; i <= 7; ++i) {
        if (c->weekday(i)) {
            dbg << '\n' << '\t' << i << ':' << c->weekday(i);
        }
    }
    const auto days = c->days();
    for (const KPlato::CalendarDay *day : days) {
        dbg << '\n' << '\t'<< day;
    }
    dbg << '\n' << ']';
    return dbg;
}

QDebug operator<<(QDebug dbg, KPlato::CalendarWeekdays *w)
{
    if (!w) {
        return dbg << "Weekdays[0x0]";
    }
    dbg << "Weekdays:" << w->weekdayMap();
    return dbg;
}

QDebug operator<<(QDebug dbg, KPlato::CalendarDay *day)
{
    if (!day) {
        return dbg << "Day[0x0]";
    }
    dbg << "Day[" << stateToString(day->state());
    if (day->date().isValid()) {
        dbg << day->date();
    }
    for(int i = 0; i < day->numIntervals(); ++i) {
        dbg << *(day->timeIntervals().at(i));
    }
    dbg <<  "]";
    return dbg;
}

QDebug operator<<(QDebug dbg, KPlato::StandardWorktime *wt)
{
    return operator<<(dbg, *wt);
}

QDebug operator<<(QDebug dbg, KPlato::StandardWorktime &wt)
{
    dbg.noquote().nospace() << "Standard["
    << "y:" <<wt.year()
    << " m:" << wt.month()
    << " w:" << wt.week()
    << " d:" << wt.day()
    << ']';
    return dbg;
}
