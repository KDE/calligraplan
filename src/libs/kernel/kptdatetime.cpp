/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptdatetime.h"
#include "kptdebug.h"

#include <QTimeZone>

namespace KPlato
{

DateTime::DateTime()
    : QDateTime()
{
}

DateTime::DateTime(QDate date)
    : QDateTime(date, QTime(0,0,0), QTimeZone::systemTimeZone())
{
}

DateTime::DateTime(QDate date, QTime time)
    : QDateTime(date, time, QTimeZone::systemTimeZone())
{
    if (!isValid() && this->date().isValid() && this->time().isValid()) {
        QTime t = this->time();
        t = t.addSecs(timeZone().daylightTimeOffset(*this));
        warnPlan<<"Invalid DateTime, try to compensate for DST"<<this->date()<<t;
        setTime(t);
        Q_ASSERT(isValid());
    }
}

DateTime::DateTime(QDate date, QTime time, const QTimeZone &timeZone)
    : QDateTime(timeZone.isValid() ? QDateTime(date, time, timeZone) : QDateTime(date, time, QTimeZone::systemTimeZone()))
{
    // If we ended inside DST, DateTime is not valid, but date(), time() and timeSpec() should be valid
    if (!isValid() && this->date().isValid() && this->time().isValid()) {
        QTime t = this->time();
        t = t.addSecs(timeZone.daylightTimeOffset(*this));
        warnPlan<<"Invalid DateTime, try to compensate for DST"<<this->date()<<t;
        setTime(t);
        Q_ASSERT(isValid());
    }
}

DateTime::DateTime(const QDateTime& other)
    : QDateTime(other)
{
}

void DateTime::add(const Duration &duration) {
    if (isValid() && duration.m_ms) {
        if (timeSpec() == Qt::TimeZone) {
            QTimeZone tz = timeZone();
            toUTC();
            QDateTime x = addMSecs(duration.m_ms);
            x.toUTC();
            setDate(x.date());
            setTime(x.time());
            toTimeZone(tz);
        } else {
            *this = addMSecs(duration.m_ms);
        }
        //debugPlan<<toString();
    }
}

void DateTime::subtract(const Duration &duration) {
    if (isValid() && duration.m_ms) {
        if (timeSpec() == Qt::TimeZone) {
            QTimeZone tz = timeZone();
            QDateTime x = addMSecs(-duration.m_ms);
            x.toUTC();
            setDate(x.date());
            setTime(x.time());
            toTimeZone(tz);
        } else {
            *this = addMSecs(-duration.m_ms);
        }
        //debugPlan<<toString();
    }
}

Duration DateTime::duration(const DateTime &dt) const {
    Duration dur;
    if (isValid() && dt.isValid()) {
        qint64 x = msecsTo(dt); //NOTE: this does conversion to UTC (expensive)
        dur.m_ms = x < 0 ? -x : x;
    }
    //debugPlan<<dur.milliseconds();
    return dur;
}

DateTime DateTime::operator+(const Duration &duration) const {
    DateTime d(*this);
    d.add(duration);
    return d;
}

DateTime& DateTime::operator+=(const Duration &duration) {
    add(duration);
    return *this;
}

DateTime DateTime::operator-(const Duration &duration) const {
    DateTime d(*this);
    d.subtract(duration);
    return d;
}

DateTime& DateTime::operator-=(const Duration &duration) {
    subtract(duration);
    return *this;
}

DateTime DateTime::fromString(const QString &dts, const QTimeZone &timeZone)
{
    if (dts.isEmpty()) {
        return DateTime();
    }
    QDateTime dt = QDateTime::fromString(dts, Qt::ISODate);
    return DateTime(dt.date(), dt.time(), timeZone);
}


}  //KPlato namespace

QDebug operator<<(QDebug dbg, const KPlato::DateTime &dt)
{
    dbg.nospace()<<" DateTime[" << dt.toString(Qt::ISODate);
    switch (dt.timeSpec()) {
        case Qt::LocalTime: dbg << "LocalTime"; break;
        case Qt::UTC: dbg << "UTC"; break;
        case Qt::OffsetFromUTC: dbg << "OffsetFromUTC"; break;
        case Qt::TimeZone: dbg << dt.timeZone(); break;
    }
    dbg.nospace() << "] ";
    return dbg.space();
}
