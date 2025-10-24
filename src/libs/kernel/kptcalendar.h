/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTCALENDAR_H
#define KPTCALENDAR_H

#include "kptdatetime.h"
#include "kptduration.h"
#include "kptdebug.h"
#include "plankernel_export.h"

#include <utility>
#include <QList>
#include <QMap>
#include <QTimeZone>

#include <KoXmlReaderForward.h>

#ifdef HAVE_KHOLIDAYS
namespace KHolidays {
    class HolidayRegion;
}
#endif 

class KUndo2Command;

class QDomElement;


/// The main namespace.
namespace KPlato
{

class Calendar;
class Project;
class IntMap;
//class DateTime;
class Project;
class Schedule;
class XMLLoaderObject;
class AppointmentIntervalList;

class PLANKERNEL_EXPORT DateTimeInterval : public std::pair<DateTime, DateTime>
{
public:
    DateTimeInterval()
    : std::pair<DateTime, DateTime>()
    {}
    DateTimeInterval(const DateTime &t1, const DateTime &t2)
    : std::pair<DateTime, DateTime>(t1, t2)
    {}
    DateTimeInterval(const DateTimeInterval &other)
    : std::pair<DateTime, DateTime>(other.first, other.second)
    {}
    DateTimeInterval &operator=(const DateTimeInterval &other) {
        first = other.first; second = other.second;
        return *this;
    }
    bool isValid() const { return first.isValid() && second.isValid(); }
    void limitTo(const DateTime &start, const DateTime &end) {
        if (! first.isValid() || (start.isValid() && start > first) ) {
            first = start;
        }
        if (! second.isValid() || (end.isValid() && end < second) ) {
            second = end;
        }
        if (isValid() && first > second) {
            first = second = DateTime();
        }
    }
    void limitTo(const DateTimeInterval &interval) {
        limitTo(interval.first, interval.second);
    }

    DateTimeInterval limitedTo(const DateTime &start, const DateTime &end) const {
        DateTimeInterval i = *this;
        i.limitTo(start, end);
        return i;
    }
    DateTimeInterval limitedTo(const DateTimeInterval &interval) const {
        return limitedTo(interval.first, interval.second);
    }
    QString toString() const {
        return QStringLiteral("%1 to %2")
                    .arg(first.isValid()?first.toString():QStringLiteral("''"), second.isValid()?second.toString():QStringLiteral("''"));
    }
};

/// TimeInterval is defined as a start time and a length.
/// The end time (start + length) must not exceed midnight
class PLANKERNEL_EXPORT TimeInterval : public std::pair<QTime, int>
{
public:
    TimeInterval()
    : std::pair<QTime, int>(QTime(), -1)
    {}
    explicit TimeInterval(std::pair<QTime, int> value)
    : std::pair<QTime, int>(value)
    {
        init();
    }
    TimeInterval(QTime start, int length)
    : std::pair<QTime, int>(start, length)
    {
        init();
    }
    TimeInterval(const TimeInterval &value)
    : std::pair<QTime, int>(value.first, value.second)
    {
        init();
    }
    /// Return the intervals start time
    QTime startTime() const { return first; }
    /// Return the intervals calculated end time. Note: It may return QTime(0,0,0)
    QTime endTime() const { return first.addMSecs(second); }
    double hours() const { return (double)(second) / (1000. * 60. * 60.); }
    /// Returns true if this interval ends at midnight, and thus endTime() returns QTime(0,0,0)
    bool endsMidnight() const { return endTime() == QTime(0, 0, 0); }

    bool isValid() const { return first.isValid() && second > 0; }
    bool isNull() const { return first.isNull() || second < 0; }

    TimeInterval &operator=(const TimeInterval &ti) { 
        first = ti.first;
        second = ti.second;
        return *this;
    }
    /// Returns true if the intervals overlap in any way
    bool intersects(const TimeInterval &ti) const {
        if (! isValid() || ! ti.isValid()) {
            return false;
        }
        if (endsMidnight() && ti.endsMidnight()) {
            return true;
        }
        if (endsMidnight()) {
            return first < ti.endTime();
        }
        if (ti.endsMidnight()) {
            return ti.first < endTime();
        }
        return (first < ti.endTime() && endTime() > ti.first) || (ti.first < endTime() && ti.endTime() > first);
    }
protected:
    void init()
    {
        int s = QTime(0, 0, 0).msecsTo(first);
        if ((s + second) > 86400000) {
            second = 86400000 - s;
            errorPlan<<"Overflow, limiting length to"<<second;
        }
    }
};


class PLANKERNEL_EXPORT CalendarDay {

public:
    enum State { Undefined = 0,
                 None=Undefined, // depreciated
                 NonWorking=1, Working=2 };
    
    CalendarDay();
    explicit CalendarDay(int state);
    explicit CalendarDay(QDate date, int state=Undefined);
    explicit CalendarDay(CalendarDay *day);
    ~CalendarDay();

    // NOTE: Saving is done here, loading is done using the XmlLoaderObject
    void save(QDomElement &element) const;

    QList<TimeInterval*> timeIntervals() const { return m_timeIntervals; }
    void addInterval(QTime t1, int length) { addInterval(new TimeInterval(t1, length) ); }
    /**
     * Caller needs to ensure that intervals are not overlapping.
     */
    void addInterval(TimeInterval *interval);
    void addInterval(TimeInterval interval) { addInterval(new TimeInterval(interval)); }
    void clearIntervals() { m_timeIntervals.clear(); }
    void setIntervals(const QList<TimeInterval*> &intervals) {
        m_timeIntervals.clear();
        m_timeIntervals = intervals;
    }
    void removeInterval(TimeInterval *interval);
    bool hasInterval(const TimeInterval *interval) const;
    int numIntervals() const;

    DateTime start() const;
    DateTime end() const;
    
    QDate date() const { return m_date; }
    void setDate(QDate date) { m_date = date; }
    int state() const { return m_state; }
    void setState(int state) { m_state = state; }

    bool operator==(const CalendarDay *day) const;
    bool operator==(const CalendarDay &day) const;
    bool operator!=(const CalendarDay *day) const;
    bool operator!=(const CalendarDay &day) const;

    Duration workDuration() const;
    /**
     * Returns the amount of 'worktime' that can be done on
     * this day between the times start and end.
     */
    Duration effort(QTime start, int length, const QTimeZone &timeZone, Schedule *sch=nullptr);
    /**
     * Returns the amount of 'worktime' that can be done on
     * this day between the times start and end.
     */
    Duration effort(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch=nullptr);

    /**
     * Returns the actual 'work interval' for the interval start to end.
     * If no 'work interval' exists, returns the interval start, end.
     * Use @ref hasInterval() to check if a 'work interval' exists.
     */
    TimeInterval interval(QTime start, int length, const QTimeZone &timeZone,  Schedule *sch=nullptr) const;
    
    /**
     * Returns the actual 'work interval' for the interval start to end.
     * If no 'work interval' exists, returns the interval start, end.
     * Use @ref hasInterval() to check if a 'work interval' exists.
     */
    TimeInterval interval(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch=nullptr) const;
    
    bool hasInterval() const;

    /**
     * Returns true if at least a part of a 'work interval' exists 
     * for the interval start to end.
     */
    bool hasInterval(QTime start, int length, const QTimeZone &timeZone, Schedule *sch=nullptr) const;
    
    /**
     * Returns true if at least a part of a 'work interval' exists 
     * for the interval @p start to @p start + @p length.
     * Assumes this day is date. (Used by weekday hasInterval().)
     * If @p sch is not 0, the schedule is checked for availability.
     */
    bool hasInterval(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch=nullptr) const;
    
    Duration duration() const;
    
    const CalendarDay &copy(const CalendarDay &day);

    static QString stateToString(int st, bool trans = false);
    static QStringList stateList(bool trans = false);

private:
    QDate m_date; //NOTE: inValid if used for weekdays
    int m_state;
    Calendar *m_calendar;
    QList<TimeInterval*> m_timeIntervals;

#ifndef NDEBUG
public:
    void printDebug(const QString& indent=QString());
#endif
};

class PLANKERNEL_EXPORT CalendarWeekdays {

public:
    CalendarWeekdays();
    explicit CalendarWeekdays(const CalendarWeekdays *weekdays);
    ~CalendarWeekdays();

    //bool load(KoXmlElement &element, XMLLoaderObject &status);
    void save(QDomElement &element) const;

    const QList<CalendarDay*> weekdays() const 
        { QList<CalendarDay*> lst = m_weekdays.values(); return lst; }
    /**
     * Returns the pointer to CalendarDay for day.
     * @param day The weekday number, must be between 1 (monday) and 7 (sunday)
     */
    CalendarDay *weekday(int day) const;
    CalendarDay *weekday(QDate date) const { return weekday(date.dayOfWeek()); }

    static int dayOfWeek(const QString &name);

    const QMap<int, CalendarDay*> &weekdayMap() const;
    
    IntMap stateMap() const;
    
//    void setWeekday(IntMap::iterator it, int state) { m_weekdays.at(it.key())->setState(state); }

    int state(QDate date) const;
    int state(int weekday) const;
    void setState(int weekday, int state);
    
    QList<TimeInterval*> intervals(int weekday) const;
    void setIntervals(int weekday, const QList<TimeInterval*> &intervals);
    void clearIntervals(int weekday);
    
    bool operator==(const CalendarWeekdays *weekdays) const;
    bool operator!=(const CalendarWeekdays *weekdays) const;

    Duration effort(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch=nullptr);
    
    /**
     * Returns the actual 'work interval' on the weekday defined by date
     * for the interval @p start to @p start + @p length.
     * If no 'work interval' exists, returns the interval start, end.
     * Use @ref hasInterval() to check if a 'work interval' exists.
     * If @p sch is not 0, the schedule is checked for availability.
     */
    TimeInterval interval(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch) const;
    /**
     * Returns true if at least a part of a 'work interval' exists 
     * on the weekday defined by date for the interval start to end.
     */
    bool hasInterval(QDate date, QTime start, int length, const QTimeZone &timeZone, Schedule *sch) const;
    bool hasInterval() const;

    Duration duration() const;
    Duration duration(int weekday) const;

    const CalendarWeekdays &copy(const CalendarWeekdays &weekdays);

    int indexOf(const CalendarDay *day) const;
    
private:
    Calendar *m_calendar;
    QMap<int, CalendarDay*> m_weekdays;

#ifndef NDEBUG
public:
    void printDebug(const QString& indent=QString());
#endif
};

/**
 * Calendar defines the working and nonworking days and hours.
 * A day can have the three states Undefined, NonWorking, or Working.
 * A calendar can have a parent calendar that defines the days that are 
 * undefined in this calendar. 
 * If a calendar have no parent, an undefined day defaults to Nonworking.
 * A Working day has one or more work intervals to define the work hours.
 *
 * The definition can consist of two parts: Weekdays and Day.
 * Day has highest priority.
 *
 * A typical calendar hierarchy could include calendars on 4 levels:
 *  1. Definition of normal weekdays and national holidays/vacation days.
 *  2. Definition of the company's special workdays/-time and vacation days.
 *  3. Definitions for groups of resources.
 *  4. Definitions for individual resources.
 *
 * A calendar can define a timezone different from the projects.
 * This enables planning with resources that does not reside in the same place.
 *
 */
class PLANKERNEL_EXPORT Calendar : public QObject
{
    Q_OBJECT
public:
    Calendar();
    explicit Calendar(const QString& name, Calendar *parent=nullptr);
    //Calendar(const Calendar &c); QObject doesn't allow a copy constructor
    ~Calendar() override;

    const Calendar &operator=(const Calendar &calendar) { return copy(calendar); }
    const Calendar &copy(const Calendar &calendar);
    void copy(const Calendar *calendar);

    QString name() const { return m_name; }
    void setName(const QString& name);

    Calendar *parentCal() const { return m_parent; }
    /**
     * Set parent calendar to @p parent.
     * Removes myself from current parent and
     * inserts myself as child to new parent.
     */
    void setParentCal(Calendar *parent, int pos = -1);
    
    bool isChildOf(const Calendar *cal) const;
    
    Project *project() const { return m_project; }
    void setProject(Project *project);

    QString id() const { return m_id; }
    void setId(const QString& id);
    
    const QList<Calendar*> &calendars() const { return m_calendars; }
    void addCalendar(Calendar *calendar, int pos = -1);
    void takeCalendar(Calendar *calendar);
    int indexOf(const Calendar *calendar) const;
    /// Return number of children
    int childCount() const { return m_calendars.count(); }
    /// Return child calendar at @p index, 0 if index out of bounds
    Calendar *childAt(int index) const { return m_calendars.value(index); }
    
    //bool load(KoXmlElement &element, XMLLoaderObject &status);
    void save(QDomElement &element) const;

    int state(QDate date) const;
    void setState(CalendarDay *day, CalendarDay::State state);
    void addWorkInterval(CalendarDay *day, TimeInterval *ti);
    void takeWorkInterval(CalendarDay *day, TimeInterval *ti);
    void setWorkInterval(TimeInterval *ti, const TimeInterval &value);

    /**
     * Find the definition for the day @p date.
     * If @p skipUndefined = true the day is NOT returned if it has state Undefined.
     */
    CalendarDay *findDay(QDate date, bool skipUndefined=false) const;
    void addDay(CalendarDay *day);
    CalendarDay *takeDay(CalendarDay *day);
    const QList<CalendarDay*> &days() const { return m_days; }
    QList<std::pair<CalendarDay*, CalendarDay*> > consecutiveVacationDays() const;
    QList<CalendarDay*> workingDays() const;
    int indexOf(const CalendarDay *day) const { return m_days.indexOf(const_cast<CalendarDay*>(day) ); }
    CalendarDay *dayAt(int index) { return m_days.value(index); }
    int numDays() const { return m_days.count(); }
    void setDate(CalendarDay *day, QDate date);
    CalendarDay *day(QDate date) const;
    
    IntMap weekdayStateMap() const;
    
    CalendarWeekdays *weekdays() const { return m_weekdays; }
    CalendarDay *weekday(int day) const { return m_weekdays->weekday(day); }
    int indexOfWeekday(const CalendarDay *day) const { return m_weekdays->indexOf(day); }
    const QList<CalendarDay*> weekdayList() const { return m_weekdays->weekdays(); }
    int numWeekdays() const { return weekdayList().count(); }
    
    /// Sets the @p weekday data to the data in @p day
    void setWeekday(int weekday, const CalendarDay &day);
    
    QString parentId() const { return m_parentId; }
    void setParentId(const QString& id) { m_parentId = id; }
    bool hasParent(Calendar *cal);

    /**
     * Returns the work intervals in the interval from @p start to @p end
     * Sets the load of each interval to @p load
     */
    AppointmentIntervalList workIntervals(const DateTime &start, const DateTime &end, double load) const;

    /**
     * Returns the amount of 'worktime' that can be done in the
     * interval from @p start to @p end
     * If @p sch is not 0, the schedule is checked for availability.
     */
    Duration effort(const DateTime &start, const DateTime &end, Schedule *sch=nullptr) const;

    /**
     * Returns the first 'work interval' for the interval 
     * starting at @p start and ending at @p end.
     * If no 'work interval' exists, returns an interval with invalid DateTime.
     * You can also use @ref hasInterval() to check if a 'work interval' exists.
     * If @p sch is not 0, the schedule is checked for availability.
     */
    DateTimeInterval firstInterval(const DateTime &start, const DateTime &end, Schedule *sch=nullptr) const;
    
    /**
     * Returns true if at least a part of a 'work interval' exists 
     * for the interval starting at @p start and ending at @p end.
     * If @p sch is not 0, the schedule is checked for availability.
     */
    bool hasInterval(const DateTime &start, const DateTime &end, Schedule *sch=nullptr) const;
        
    /** 
     * Find the first available time after @p time before @p limit.
     * Return invalid datetime if not available.
     * If @p sch is not 0, the schedule is checked for availability.
     */
    DateTime firstAvailableAfter(const DateTime &time, const DateTime &limit, Schedule *sch = nullptr);
    /** 
     * Find the first available time backwards from @p time. Search until @p limit.
     * Return invalid datetime if not available.
     * If @p sch is not 0, the schedule is checked for availability.
     */
    DateTime firstAvailableBefore(const DateTime &time, const DateTime &limit, Schedule *sch = nullptr);

    Calendar *findCalendar() const { return findCalendar(m_id); }
    Calendar *findCalendar(const QString &id) const;
    bool removeId() { return removeId(m_id); }
    bool removeId(const QString &id);
    void insertId(const QString &id);

    QTimeZone timeZone() const { return m_timeZone; }
    void setTimeZone(const QTimeZone &tz);
    /// Return the project timezone, or local timezone if no project
    QTimeZone projectTimeZone() const;

    void setDefault(bool on);
    bool isDefault() const { return m_default; }
    
    int cacheVersion() const;
    void incCacheVersion();
    void setCacheVersion(int version);
    bool loadCacheVersion(KoXmlElement &element, XMLLoaderObject &status);
    void saveCacheVersion(QDomElement &element) const;

    /// A calendar can be local to this project, or
    /// defined externally and shared with other projects
    bool isShared() const;
    /// Set calendar to be local if on = false, or shared if on = true
    void setShared(bool on);

#ifdef HAVE_KHOLIDAYS
    bool isHoliday(QDate date) const;
    KHolidays::HolidayRegion *holidayRegion() const;
    void setHolidayRegion(const QString &code);
    QString holidayRegionCode() const;
    QStringList holidayRegionCodes() const;
#endif

    void setBlockVersion(bool block) { m_blockversion = block; }

Q_SIGNALS:
    void calendarChanged(KPlato::Calendar*);
    void calendarDayChanged(KPlato::CalendarDay*);
    void timeIntervalChanged(KPlato::TimeInterval*);
    
    void weekdayToBeAdded(KPlato::CalendarDay *day, int index);
    void weekdayAdded(KPlato::CalendarDay *day);
    void weekdayToBeRemoved(KPlato::CalendarDay *day);
    void weekdayRemoved(KPlato::CalendarDay *day);
    
    void dayToBeAdded(KPlato::CalendarDay *day, int index);
    void dayAdded(KPlato::CalendarDay *day);
    void dayToBeRemoved(KPlato::CalendarDay *day);
    void dayRemoved(KPlato::CalendarDay *day);
    
    void workIntervalToBeAdded(KPlato::CalendarDay*, KPlato::TimeInterval*, int index);
    void workIntervalAdded(KPlato::CalendarDay*, KPlato::TimeInterval*);
    void workIntervalToBeRemoved(KPlato::CalendarDay*, KPlato::TimeInterval*);
    void workIntervalRemoved(KPlato::CalendarDay*, KPlato::TimeInterval*);

protected:
    void init();
    
    /**
     * Returns the amount of 'worktime' that can be done on
     * the @p date between the times @p start and @p start + @p length.
     * The date and times are in timespecification @p spec.
     * If @p sch is not 0, the schedule is checked for availability.
     */
    Duration effort(QDate date, QTime start, int length, Schedule *sch=nullptr) const;
    /**
     * Returns the amount of 'worktime' that can be done in the
     * interval from @p start to @p end
     * If @p sch is not 0, the schedule is checked for availability.
     */
    Duration effort(const QDateTime &start, const QDateTime &end, Schedule *sch=nullptr) const;
    /**
     * Returns the first 'work interval' on date for the interval 
     * starting at @p start and ending at @p start + @p length.
     * If no 'work interval' exists, returns a null interval.
     * You can also use @ref hasInterval() to check if a 'work interval' exists.
     * The date and times are in timespecification spec.
     * If @p sch is not 0, the schedule is checked for availability.
     */
    TimeInterval firstInterval(QDate date, QTime start, int length, Schedule *sch=nullptr) const;
    /**
     * Returns the first 'work interval' for the interval
     * starting at @p start and ending at @p end.
     * If no 'work interval' exists, returns an interval with invalid DateTime.
     */
    DateTimeInterval firstInterval(const QDateTime &start, const QDateTime &end, Schedule *sch=nullptr) const;

    /**
     * Returns true if at least a part of a 'work interval' exists 
     * for the interval on date, starting at @p start and ending at @p start + @p length.
     * If @p sch is not 0, the schedule is checked for availability.
     */
    bool hasInterval(QDate date, QTime start, int length, Schedule *sch=nullptr) const;

    /**
     * Returns the work intervals in the interval from @p start to @p end
     * Sets the load of each interval to @p load
     */
    AppointmentIntervalList workIntervals(const QDateTime &start, const QDateTime &end, double load) const;

    /**
     * Find the first available time backwards from @p time. Search until @p limit.
     * Return invalid datetime if not available.
     * If @p sch is not 0, the schedule is checked for availability.
     */
    DateTime firstAvailableBefore(const QDateTime &time, const QDateTime &limit, Schedule *sch = nullptr);

private:
    QString m_name;
    Calendar *m_parent;
    Project *m_project;
    bool m_deleted;
    QString m_id;
    QString m_parentId;

    QList<CalendarDay*> m_days;
    CalendarWeekdays *m_weekdays;

    QList<Calendar*> m_calendars;

    QTimeZone m_timeZone;
    bool m_default; // this is the default calendar, only used for save/load
    bool m_shared;

#ifdef HAVE_KHOLIDAYS
    KHolidays::HolidayRegion *m_region;
    QString m_regionCode;
#endif

    int m_cacheversion; // incremented every time a calendar is changed
    friend class Project;
    int m_blockversion; // don't update if true
#ifndef NDEBUG
public:
    void printDebug(const QString& indent=QString());
#endif
};

class PLANKERNEL_EXPORT StandardWorktime
{
public:
    explicit StandardWorktime(Project *project = nullptr);
    explicit StandardWorktime(StandardWorktime* worktime);
    ~StandardWorktime();

    /// Set Project
    void setProject(Project *project) { m_project = project; }
    /// The work time of a normal year.
    Duration durationYear() const { return m_year; }
    /// The work time of a normal year.
    double year() const { return m_year.toDouble(Duration::Unit_h); }
    /// Set the work time of a normal year.
    void setYear(const Duration year) { m_year = year; }
    /// Set the work time of a normal year.
    void setYear(double hours) { m_year = Duration((qint64)(hours*60.0*60.0*1000.0)); }
    
    /// The work time of a normal month
    Duration durationMonth() const { return m_month; }
    /// The work time of a normal month
    double month() const  { return m_month.toDouble(Duration::Unit_h); }
    /// Set the work time of a normal month
    void setMonth(const Duration month) { m_month = month; }
    /// Set the work time of a normal month
    void setMonth(double hours) { m_month = Duration((qint64)(hours*60.0*60.0*1000.0)); }
    
    /// The work time of a normal week
    Duration durationWeek() const { return m_week; }
    /// The work time of a normal week
    double week() const { return m_week.toDouble(Duration::Unit_h); }
    /// Set the work time of a normal week
    void setWeek(const Duration week) { m_week = week; }
    /// Set the work time of a normal week
    void setWeek(double hours) { m_week = Duration((qint64)(hours*60.0*60.0*1000.0)); }
    
    /// The work time of a normal day
    Duration durationDay() const { return m_day; }
    /// The work time of a normal day
    double day() const { return m_day.toDouble(Duration::Unit_h); }
    /// Set the work time of a normal day
    void setDay(const Duration day) { m_day = day; changed(); }
    /// Set the work time of a normal day
    void setDay(double hours) { m_day = Duration(hours, Duration::Unit_h); changed(); }
    
    QList<qint64> scales() const;

    //bool load(KoXmlElement &element, XMLLoaderObject &status);
    void save(QDomElement &element) const;

    void changed();

protected:
    void init();
    
private:
    Project *m_project;
    Duration m_year;
    Duration m_month;
    Duration m_week;
    Duration m_day;
};

}  //KPlato namespace

PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, KPlato::Calendar *c);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, KPlato::CalendarWeekdays *w);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, KPlato::CalendarDay *day);

PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, KPlato::StandardWorktime &wt);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, KPlato::StandardWorktime *wt);

#endif
