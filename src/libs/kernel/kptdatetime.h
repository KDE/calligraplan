/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KPTDATETIME_H
#define KPTDATETIME_H

#include "plankernel_export.h"
#include "kptduration.h"

#include <QDateTime>
#include <QTimeZone>
#include <QDebug>

/// The main namespace.
namespace KPlato
{

class Duration;

/**
 * DateTime is a QDateTime which knows about Duration
 * Note that in Plan all datetimes shall be in the time zone specified
 * in the project.
 * Exception to this is the calendar related dates and times which has
 * their own time zone specification.
 */
class PLANKERNEL_EXPORT DateTime : public QDateTime {

public:
    /// Create a DateTime.
    DateTime();
    /// Constructs a datetime with the given date, a valid time(00:00:00.000), and sets the timezone to QTimZone::systemTimeZone().
    explicit DateTime(QDate  );
    ///Constructs a datetime with the given date and time, and sets the timezone to QTimZone::systemTimeZone().
    /// If date is valid and time is not, the time will be set to midnight.
    DateTime(QDate , QTime);
    ///Constructs a datetime with the given date and time in the given timezone.
    /// If @p timeZone is not valid, the QTimZone::systemTimeZone() is used.
    /// If @p date is valid and @p time is not, the time will be set to midnight.
    DateTime(QDate , QTime , const QTimeZone &timeZone);
    /// Constructs a copy of the @p other QDateTime
    DateTime(const QDateTime &other);

    /**
     * Adds the duration @p duration to the datetime
     */
    DateTime operator+(const Duration &duration) const;
    /**
     * Subtracts the duration @p duration from the datetime
     */
    DateTime operator-(const Duration &duration) const ;
    /**
     * Returns the absolute duration between the two datetimes
     */
    Duration operator-(const DateTime &dt) const { return duration(dt); }
    /**
     * Returns the absolute duration between the two datetimes
     */
    Duration operator-(const DateTime &dt) { return duration(dt); }
    /// Add @p duration to this datetime.
    DateTime &operator+=(const Duration &duration);
    /// Subtract the @p duration from this datetime.
    DateTime &operator-=(const Duration &duration);

    /**
     * Parse a datetime string @p dts and return the DateTime in the given @p timeZone.
     * The string @p dts should be in Qt::ISODate format and contain no time zone information.
     */
    static DateTime fromString(const QString &dts, const QTimeZone &timeZone = QTimeZone::systemTimeZone());

private:

    Duration duration(const DateTime &dt) const;
    void add(const Duration &duration);
    void subtract(const Duration &duration);

};


}  //KPlato namespace

PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::DateTime &dt);

#endif
