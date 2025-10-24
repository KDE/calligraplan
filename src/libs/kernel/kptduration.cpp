/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Thomas Zander zander @kde.org
   SPDX-FileCopyrightText: 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptduration.h"
#include "kptdatetime.h"
#include "kptdebug.h"

#include <KFormat>
#include <KLocalizedString>

#include <QLocale>
#include <QRegularExpression>
#include <QStringList>


namespace KPlato
{

// Set the value of Duration::zeroDuration to zero.
const Duration Duration::zeroDuration(0, 0, 0);

Duration::Duration() {
    m_ms = 0;
}

Duration::Duration(double value, Duration::Unit unit) {
    if (unit == Unit_ms) m_ms = (qint64)value;
    else if (unit == Unit_s) m_ms = (qint64)(value * 1000);
    else if (unit == Unit_m) m_ms = (qint64)(value * (1000 * 60));
    else if (unit == Unit_h) m_ms = (qint64)(value * (1000 * 60 * 60));
    else if (unit == Unit_d) m_ms = (qint64)(value * (1000 * 60 * 60 * 24));
    else if (unit == Unit_w) m_ms = (qint64)(value * (1000 * 60 * 60 * 24 * 7));
    else if (unit == Unit_M) m_ms = (qint64)(value * (qint64)(1000 * 60 * 60) * (24 * 30));
    else if (unit == Unit_Y) m_ms = (qint64)(value * (qint64)(1000 * 60 * 60) * (24 * 365));
}

Duration::Duration(unsigned d, unsigned h, unsigned m, unsigned s, unsigned ms) {
    m_ms = ms;
    m_ms += static_cast<qint64>(s) * 1000; // cast to avoid potential overflow problem
    m_ms += static_cast<qint64>(m) * 60 * 1000;
    m_ms += static_cast<qint64>(h) * 60 * 60 * 1000;
    m_ms += static_cast<qint64>(d) * 24 * 60 * 60 * 1000;
}

Duration::Duration(const qint64 value, Duration::Unit unit) {
    if (unit == Unit_ms) m_ms = value;
    else if (unit == Unit_s) m_ms = (qint64)(value * 1000);
    else if (unit == Unit_m) m_ms = (qint64)(value * (1000 * 60));
    else if (unit == Unit_h) m_ms = (qint64)(value * (1000 * 60 * 60));
    else if (unit == Unit_d) m_ms = (qint64)(value * (1000 * 60 * 60 * 24));
    else if (unit == Unit_w) m_ms = (qint64)(value * (1000 * 60 * 60 * 24 * 7));
    else if (unit == Unit_M) m_ms = (qint64)(value * (qint64)(1000 * 60 * 60) * (24 * 30));
    else if (unit == Unit_Y) m_ms = (qint64)(value * (qint64)(1000 * 60 * 60) * (24 * 365));
    else errorPlan<<"Unknown unit: "<<unit;
}

void Duration::add(const Duration &delta) {
    m_ms += delta.m_ms;
}

void Duration::add(qint64 delta) {
    qint64 tmp = m_ms + delta;
    if (tmp < 0) {
        debugPlan<<"Underflow"<<(long int)delta<<" from"<<this->toString();
        m_ms = 0;
        return;
    }
    m_ms = tmp;
}

void Duration::subtract(const Duration &delta) {
    if (m_ms < delta.m_ms) {
        debugPlan<<"Underflow"<<delta.toString()<<" from"<<this->toString();
        m_ms = 0;
        return;
    }
    m_ms -= delta.m_ms;
}

Duration Duration::operator*(int value) const {
    Duration dur(*this);
    if (value < 0) {
        debugPlan<<"Underflow"<<value<<" from"<<this->toString();
    }
    else {
        dur.m_ms = m_ms * value; //FIXME
    }
    return dur;
}

Duration Duration::operator/(int value) const {
    Duration dur(*this);
    if (value <= 0) {
        debugPlan<<"Underflow"<<value<<" from"<<this->toString();
    }
    else {
        dur.m_ms = m_ms / value; //FIXME
    }
    return dur;
}

Duration Duration::operator*(const double value) const {
    Duration dur(*this);
    dur.m_ms = qAbs(m_ms * (qint64)value);
    return dur;
}

Duration Duration::operator*(const Duration &value) const {
    Duration dur(*this);
    dur.m_ms = m_ms * value.m_ms;
    return dur;
}

double Duration::operator/(const Duration &d) const {
    if (d == zeroDuration) {
        debugPlan<<"Divide by zero:"<<this->toString();
        return 0.0;
    }
    return (double)(m_ms) / (double)(d.m_ms);
}

QString Duration::format(Unit unit, int pres) const
{
    /* FIXME if necessary
    return i18nc("<duration><unit>", "%1%2", QLocale().toString(toDouble(unit), 'f', pres), unitToString(unit));*/
    return QLocale().toString(toDouble(unit), 'f', pres) + unitToString(unit);
}

QString Duration::toString(Format format) const {
    qint64 ms;
    double days;
    unsigned hours;
    unsigned minutes;
    unsigned seconds;
    QString result;
    QLocale locale;
    switch (format) {
        case Format_Hour:
            ms = m_ms;
            hours = ms / (1000 * 60 * 60);
            ms -= (qint64)hours * (1000 * 60 * 60);
            minutes = ms / (1000 * 60);
            result = QStringLiteral("%1h%2m").arg(hours).arg(minutes);
            break;
        case Format_Day:
            days = m_ms / (1000 * 60 * 60 * 24.0);
            result = QStringLiteral("%1d").arg(QString::number(days, 'f', 4));
            break;
        case Format_DayTime: {
            ms = m_ms;
            days = m_ms / (1000 * 60 * 60 * 24);
            ms -= (qint64)days * (1000 * 60 * 60 * 24);
            hours = ms / (1000 * 60 * 60);
            ms -= (qint64)hours * (1000 * 60 * 60);
            minutes = ms / (1000 * 60);
            ms -= minutes * (1000 * 60);
            seconds = ms / (1000);
            ms -= seconds * (1000);
            const QString dayString = QString::number((unsigned)days);
            const QString hourString = QString::number(hours);
            const QString minString = QString::number(minutes);
            const QString secString = QString::number(seconds);
            const QString msString = QString::number(ms);
            result = QStringLiteral("%1 %2:%3:%4.%5").arg(dayString, hourString, minString, secString, msString);
            break;
        }
        case Format_HourFraction:
            result = QLocale().toString(toDouble(Unit_h), 'f', 2);
            break;
        // i18n
        case Format_i18nHour:
            ms = m_ms;
            hours = ms / (1000 * 60 * 60);
            ms -= (qint64)hours * (1000 * 60 * 60);
            minutes = ms / (1000 * 60);
            if (minutes > 0) {
                result = i18nc("<hours>h:<minutes>m", "%1h:%2m", locale.toString(hours), locale.toString(minutes));
            } else {
                result = i18nc("<hours>h:<minutes>m", "%1h", locale.toString(hours));
            }
            break;
        case Format_i18nDay:
            result = KFormat().formatSpelloutDuration(m_ms);
            break;
        case Format_i18nWeek:
            result = this->format(Unit_w, 2);
            break;
        case Format_i18nMonth:
            result = this->format(Unit_M, 2);
            break;
        case Format_i18nYear:
            result = this->format(Unit_Y, 2);
            break;
        case Format_i18nDayTime:
            ms = m_ms;
            days = m_ms / (1000 * 60 * 60 * 24);
            ms -= (qint64)days * (1000 * 60 * 60 * 24);
            hours = ms / (1000 * 60 * 60);
            ms -= (qint64)hours * (1000 * 60 * 60);
            minutes = ms / (1000 * 60);
            ms -= minutes * (1000 * 60);
            seconds = ms / (1000);
            ms -= seconds * (1000);
            if (days > 0) {
                result = i18nc("<days>d <hours>h:<minutes>m", "%1d %2h:%3m", locale.toString(days), locale.toString(hours), locale.toString(minutes));
            } else if (hours > 0) {
                result = toString(Format_i18nHour);
            } else if (minutes > 0) {
                if (ms > 0 ) {
                    result = i18nc("<minutes>m:<seconds>s.<milliseconds>", "%1m:%2s.%3", locale.toString(minutes), locale.toString(seconds), locale.toString(ms));
                } else if (seconds > 0) {
                    result = i18nc("<minutes>m:<seconds>s", "%1m:%2s", locale.toString(minutes), locale.toString(seconds));
                } else {
                    result = i18nc("<minutes>m", "%1m", locale.toString(minutes));
                }
            } else if (seconds > 0) {
                if (ms == 0 ) {
                    result = i18nc("<seconds>s", "%1s", locale.toString(seconds));
                } else {
                    result = i18nc("<seconds>s.<milliseconds>", "%1s.%2", locale.toString(seconds), locale.toString(ms));
                }
            } else if (ms > 0) {
                result = i18nc("<milliseconds>ms", "%1ms", locale.toString(ms));
            }
            break;
        case Format_i18nHourFraction:
            result = locale.toString(toDouble(Unit_h), 'f', 2);
            break;
        default:
            qFatal("Unknown format");
            break;
    }
    return result;
}

Duration Duration::fromString(const QString &s, Format format, bool *ok) {
    if (ok) *ok = false;
    Duration tmp;
    switch (format) {
        case Format_Hour: {
            QRegularExpression matcher(QStringLiteral("^(\\d*)h(\\d*)m$"));
            QRegularExpressionMatch match = matcher.match(s);
            if (match.hasMatch()) {
                tmp.addHours(match.captured(1).toUInt());
                tmp.addMinutes(match.captured(2).toUInt());
                if (ok) *ok = true;
            }
            break;
        }
        case Format_DayTime: {
            QRegularExpression matcher(QStringLiteral("^(\\d*) (\\d*):(\\d*):(\\d*)\\.(\\d*)$"));
            QRegularExpressionMatch match = matcher.match(s);
            if (match.hasMatch()) {
                tmp.addDays(match.captured(1).toUInt());
                tmp.addHours(match.captured(2).toUInt());
                tmp.addMinutes(match.captured(3).toUInt());
                tmp.addSeconds(match.captured(4).toUInt());
                tmp.addMilliseconds(match.captured(5).toUInt());
                if (ok) *ok = true;
            }
            break;
        }
        case Format_HourFraction: {
            // should be in double format
            bool res;
            double f = QLocale().toDouble(s, &res);
            if (ok) *ok = res;
            if (res) {
                return Duration((qint64)(f)*3600*1000);
            }
            break;
        }
        default:
            qFatal("Unknown format");
            break;
    }
    return tmp;
}

QStringList Duration::unitList(bool trans)
{
    QStringList lst;
    lst << (trans ? i18nc("Year. Note: Letter(s) only!", "Y") : QStringLiteral("Y"))
        << (trans ? i18nc("Month. Note: Letter(s) only!", "M") : QStringLiteral("M"))
        << (trans ? i18nc("Week. Note: Letter(s) only!", "w") : QStringLiteral("w"))
        << (trans ? i18nc("Day. Note: Letter(s) only!", "d") : QStringLiteral("d"))
        << (trans ? i18nc("Hour. Note: Letter(s) only!", "h") : QStringLiteral("h"))
        << (trans ? i18nc("Minute. Note: Letter(s) only!", "m") : QStringLiteral("m"))
        << (trans ? i18nc("Second. Note: Letter(s) only!", "s") : QStringLiteral("s"))
        << (trans ? i18nc("Millisecond. Note: Letter(s) only!", "ms") : QStringLiteral("ms"));
    return lst;
}

QString Duration::unitToString(Duration::Unit unit, bool trans)
{
    return unitList(trans).at(unit);
}

Duration::Unit Duration::unitFromString(const QString &u)
{
    int i = unitList().indexOf(u);
    if (i < 0) {
        errorPlan<<"Illegal unit: "<<u;
        return Unit_ms;
    }
    return (Duration::Unit)(i); 
}

bool Duration::valueFromString(const QString &value, double &rv, Unit &unit) {
    const QStringList lst = Duration::unitList();
    for (const QString &s : lst) {
        int pos = value.lastIndexOf(s);
        if (pos != -1) {
            unit = Duration::unitFromString(s);
            QString v = value;
            v.remove(s);
            bool ok;
            rv = v.toDouble(&ok);
            errorPlan<<value<<" -> "<<v<<", "<<s<<" = "<<ok<<'\n';
            return ok;
        }
    }
    errorPlan<<"Illegal format, no unit: "<<value<<'\n';
    return false;
}

double Duration::toHours() const
{
    return toDouble(Unit_h);
}

double Duration::toDouble(Unit u) const
{
    if (u == Unit_ms) return (double)m_ms;
    else if (u == Unit_s) return (double)m_ms/1000.0;
    else if (u == Unit_m) return (double)m_ms/(1000.0*60.0);
    else if (u == Unit_h) return (double)m_ms/(1000.0*60.0*60.0);
    else if (u == Unit_d) return (double)m_ms/(1000.0*60.0*60.0*24.0);
    else if (u == Unit_w) return (double)m_ms/(1000.0*60.0*60.0*24.0*7.0);
    else if (u == Unit_M) return (double)m_ms/(1000.0*60.0*60.0*24.0*30); //Month
    else if (u == Unit_Y) return (double)m_ms/(1000.0*60.0*60.0*24.0*365); // Year
    return (double)m_ms;
}

}  //KPlato namespace
