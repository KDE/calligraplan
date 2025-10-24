/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Thomas Zander zander @kde.org
   SPDX-FileCopyrightText: 2004-2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTDURATION_H
#define KPTDURATION_H

#include "plankernel_export.h"

#include <QtGlobal>

class QString;

/// The main namespace.
namespace KPlato
{

/**
 * The Duration class can be used to store a timespan in a convenient format.
 * The timespan can be in length in many many hours down to milliseconds.
 */
class PLANKERNEL_EXPORT Duration
{
public:
    /**
     * DayTime  = d hh:mm:ss.sss
     * Day      = d.ddd
     * Hour     = hh:mm
     * HourFraction = h.fraction of an hour
     */
    enum Format { Format_DayTime, Format_Year, Format_Month, Format_Week, Format_Day, Format_Hour, Format_HourFraction, Format_i18nDayTime, Format_i18nYear, Format_i18nMonth, Format_i18nWeek, Format_i18nDay, Format_i18nHour, Format_i18nHourFraction };

    //NOTE: These must match units in DurationSpinBox!
    enum Unit { Unit_Y, Unit_M, Unit_w, Unit_d, Unit_h, Unit_m, Unit_s, Unit_ms };

    /// Create a zero duration
    Duration();
    /// Create a duration of @p value, the value is in @p unit (default unit is milliseconds)
    explicit Duration(const qint64 value, Unit unit = Unit_ms);
    /// Create a duration of @p value, the value is in @p unit (default is hours)
    explicit Duration(double value, Unit unit = Unit_h);
    /// Create a duration of @p d days, @p h hours, @p m minutes, @p s seconds and @p ms milliseconds
    Duration(unsigned d, unsigned h, unsigned m, unsigned s=0, unsigned ms=0);

    /// Return duration in milliseconds
    qint64 milliseconds() const { return m_ms; }
    /// Return duration in whole seconds
    qint64 seconds() const { return m_ms / 1000; }
    /// Return duration in whole minutes
    qint64 minutes() const { return seconds() / 60; }
    /// Return duration in whole hours
    unsigned hours() const { return minutes() / 60; }
    /// Return duration in whole days
    unsigned days() const { return hours() / 24; }

    /**
     * Adds @p delta to *this. If @p delta > *this, *this is set to zeroDuration.
     */
    void addMilliseconds(qint64 delta)  { add(delta); }

    /**
     * Adds @p delta to *this. If @p delta > *this, *this is set to zeroDuration.
     */
    void addSeconds(qint64 delta) { addMilliseconds(delta * 1000); }

    /**
     * Adds @p delta to *this. If @p delta > *this, *this is set to zeroDuration.
     */
    void addMinutes(qint64 delta) { addSeconds(delta * 60); }

    /**
     * Adds @p delta to *this. If @p delta > *this, *this is set to zeroDuration.
     */
    void addHours(qint64 delta) { addMinutes(delta * 60); }

    /**
     * Adds @p delta to *this. If @p delta > *this, *this is set to zeroDuration.
     */
    void addDays(qint64 delta) { addHours(delta * 24); }

    bool   operator==(const KPlato::Duration &d) const { return m_ms == d.m_ms; }
    bool   operator==(qint64 d) const { return m_ms == d; }
    bool   operator!=(const KPlato::Duration &d) const { return m_ms != d.m_ms; }
    bool   operator!=(qint64 d) const { return m_ms != d; }
    bool   operator<(const KPlato::Duration &d) const { return m_ms < d.m_ms; }
    bool   operator<(qint64 d) const { return m_ms < d; }
    bool   operator<=(const KPlato::Duration &d) const { return m_ms <= d.m_ms; }
    bool   operator<=(qint64 d) const { return m_ms <= d; }
    bool   operator>(const KPlato::Duration &d) const { return m_ms > d.m_ms; }
    bool   operator>(qint64 d) const { return m_ms > d; }
    bool   operator>=(const KPlato::Duration &d) const { return m_ms >= d.m_ms; }
    bool   operator>=(qint64 d) const { return m_ms >= d; }
    Duration operator*(int value) const; 
    Duration operator*(const double value) const;
    Duration operator*(const Duration &value) const;
    /// Divide duration with the integer @p value
    Duration operator/(int value) const;
    /// Divide duration with the duration @p d
    double operator/(const KPlato::Duration &d) const;
    /// Add duration with duration @p d
    Duration operator+(const KPlato::Duration &d) const
        {Duration dur(*this); dur.add(d); return dur; }
    /// Add duration with duration @p d
    Duration &operator+=(const KPlato::Duration &d) {add(d); return *this; }
    /// Subtract duration with duration @p d
    Duration operator-(const KPlato::Duration &d) const
        {Duration dur(*this); dur.subtract(d); return dur; }
    /// Subtract duration with duration @p d
    Duration &operator-=(const KPlato::Duration &d) {subtract(d); return *this; }

    /// Format duration into a string with @p unit and @p precision.
    QString format(Unit unit = Unit_h, int precision = 1) const;
    /// Convert duration to a string with @p format
    QString toString(Format format = Format_DayTime) const;
    /// Create a duration from string @p s with @p format
    static Duration fromString(const QString &s, Format format = Format_DayTime, bool *ok=nullptr);

    /// Return the duration scaled to hours
    double toHours() const;
    /**
     * Converts Duration into a double and scales it to unit @p u (default unit is hours)
     */
    double toDouble(Unit u = Unit_h) const;

    /// Return the list of units. Translated if @p trans is true.
    static QStringList unitList(bool trans = false);
    /// Return @p unit in human readable form. Translated if @p trans is true.
    static QString unitToString(Duration::Unit unit, bool trans = false);
    /// Convert @p unit name into Unit
    static Unit unitFromString(const QString &unit);
    /// Returns value and unit from a @<value@>@<unit@> coded string in @p rv and @p unit.
    static bool valueFromString(const QString &value, double &rv, Unit &unit);
    /**
     * This is useful for occasions where we need a zero duration.
     */
    static const Duration zeroDuration;

private:
    friend class DateTime;
    /**
     * Duration in milliseconds. Signed to allow for simple calculations which
     * might go negative for intermediate results.
     */
    qint64 m_ms;
    
private:
    void add(qint64 delta);
    void add(const Duration &delta);

    /**
    * Subtracts @param delta from *this. If @param delta > *this, *this is set to zeroDuration.
    */
    void subtract(const Duration &delta);
};

}  //KPlato namespace

#endif
