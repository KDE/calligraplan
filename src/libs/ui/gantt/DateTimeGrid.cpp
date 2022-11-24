/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "DateTimeGrid.h"
#include "config.h"
#include <kptcalendar.h>
#include <kptproject.h>
#include <KoXmlReader.h>

#include <QRectF>
#include <QPainter>
#include <QDate>
#include <QApplication>
#include <QPalette>
#include <QWidget>

#include <KGanttAbstractRowController>

using namespace KPlato;

DateTimeGrid::DateTimeGrid()
    : KGantt::DateTimeGrid()
    , m_timeLine(new DateTimeTimeLine())
{
    setFreeDays(QSet<Qt::DayOfWeek>());
}

Calendar *DateTimeGrid::calendar() const
{
    return m_calendar;
}

void DateTimeGrid::setCalendar(Calendar *calendar)
{
    m_calendar = calendar;
}

QDateTime DateTimeGrid::chartXtoDateTime(qreal x) const
{
    assert(startDateTime().isValid());
    int days = static_cast<int>(x/dayWidth());
    qreal secs = x*(24.*60.*60.)/dayWidth();
    QDateTime dt = startDateTime();
    QDateTime result = dt.addDays(days)
                       .addSecs(static_cast<int>(secs-(days*24.*60.*60.)))
                       .addMSecs(qRound((secs-static_cast<int>(secs))*1000.));
    return result;
}

Qt::PenStyle DateTimeGrid::gridLinePenStyle(QDateTime dt) const
{
    switch (scale()) {
        case ScaleHour:
            // Midnight
            if (dt.time().hour() == 0)
                return Qt::SolidLine;
            return Qt::DashLine;
        case ScaleDay:
            // First day of the week
            if (dt.date().dayOfWeek() == weekStart())
                return Qt::SolidLine;
            return Qt::DashLine;
        case ScaleWeek:
            // First day of the month
            if (dt.date().day() == 1)
                return Qt::SolidLine;
            // First day of the week
            if (dt.date().dayOfWeek() == weekStart())
                return Qt::DashLine;
            return Qt::NoPen;
        case ScaleMonth:
            // First day of the year
            if (dt.date().dayOfYear() == 1)
                return Qt::SolidLine;
            // First day of the month
            if (dt.date().day() == 1)
                return Qt::DashLine;
            return Qt::NoPen;
        default:
            // Nothing to do here
            break;
   }

    // Default
    return Qt::NoPen;
}

QDateTime DateTimeGrid::adjustDateTimeForHeader(QDateTime dt) const
{
    // In any case, set time to 00:00:00:00
    dt.setTime(QTime(0, 0, 0, 0));

    switch (scale()) {
        case ScaleWeek:
            // Set day to beginning of the week
            while (dt.date().dayOfWeek() != weekStart())
                dt = dt.addDays(-1);
            break;
        case ScaleMonth:
            // Set day to beginning of the month
            dt = dt.addDays(1 - dt.date().day());
            break;
        default:
            // In any other case, we don't need to adjust the date time
            break;
    }

    return dt;
}

void DateTimeGrid::paintVerticalLines(QPainter* painter, const QRectF& sceneRect, const QRectF& exposedRect, QWidget* widget)
{
    if (!m_calendar) {
        return;
    }
    QDateTime dt = chartXtoDateTime(exposedRect.left());
    dt = adjustDateTimeForHeader(dt);

    int offsetSeconds = 0;
    int offsetDays = 0;
    // Determine the time step per grid line
    if (scale() == ScaleHour) {
        offsetSeconds = 60*60;
    } else {
        offsetDays = 1;
    }

    auto freeDaysBrush = this->freeDaysBrush();
    if (freeDaysBrush.style() == Qt::NoBrush) {
        freeDaysBrush = widget ? widget->palette().midlight() : QApplication::palette().midlight();
    }
    for (qreal x = dateTimeToChartX(dt); x < exposedRect.right(); dt = dt.addSecs(offsetSeconds), dt = dt.addDays(offsetDays), x = dateTimeToChartX(dt)) {
        QPen pen = painter->pen();
        pen.setBrush(QApplication::palette().dark());
        pen.setStyle(gridLinePenStyle(dt));
        painter->setPen(pen);
        if (m_calendar->state(dt.date()) != CalendarDay::Working) {
            painter->fillRect(QRectF(x, exposedRect.top(), dayWidth(), exposedRect.height()), freeDaysBrush);
        }
        painter->drawLine(QPointF(x, sceneRect.top()), QPointF(x, sceneRect.bottom()));
    }
}

void DateTimeGrid::paintVerticalUserDefinedLines(QPainter* painter, const QRectF& sceneRect, const QRectF& exposedRect, QWidget *widget)
{
    Q_UNUSED(sceneRect)

    QPen pen = painter->pen();
    pen.setBrush(QApplication::palette().dark());

    // Do freeDays, we need to iterate over all dates
    QDateTime dtLeft = chartXtoDateTime(exposedRect.left());
    if (m_calendar) {
        auto freeDaysBrush = this->freeDaysBrush();
        if (freeDaysBrush.style() == Qt::NoBrush) {
            freeDaysBrush = widget ? widget->palette().midlight() : QApplication::palette().midlight();
        }
        QDate lastDate = chartXtoDateTime(exposedRect.right()).date();
        for (QDateTime date(dtLeft.date(), QTime()); date.date() <= lastDate; date = date.addDays(1)) {
            if (m_calendar->state(date.date()) != CalendarDay::Working) {
                qreal x = dateTimeToChartX(date);
                painter->fillRect(QRectF(x, exposedRect.top(), dayWidth(), exposedRect.height()), freeDaysBrush);
            }
        }
    }
}

void DateTimeGrid::paintGrid(QPainter *painter, const QRectF &sceneRect, const QRectF &exposedRect, KGantt::AbstractRowController *rowController, QWidget *widget)
{
    switch (scale()) {
    case ScaleHour:
    case ScaleDay:
    case ScaleWeek:
    case ScaleMonth:
        paintVerticalLines(painter, sceneRect, exposedRect, widget);
        break;
    case ScaleAuto:
    case ScaleUserDefined:
        paintVerticalUserDefinedLines(painter, sceneRect, exposedRect, widget);
        break;
    }

    KGantt::DateTimeGrid::paintGrid(painter, sceneRect, exposedRect, rowController, widget);
}

void DateTimeGrid::drawDayBackground(QPainter* painter, const QRectF& rect, const QDate& date)
{
    Q_UNUSED(date);
    if (m_timeLine->options() & DateTimeTimeLine::Background) {
        drawTimeLine(painter, rect);
    }
}

void DateTimeGrid::drawDayForeground(QPainter* painter, const QRectF& rect, const QDate& date)
{
    Q_UNUSED(date);
    if (m_timeLine->options() & DateTimeTimeLine::Foreground) {
        drawTimeLine(painter, rect);
    }
}

DateTimeTimeLine *DateTimeGrid::timeNow() const
{
    return m_timeLine;
}

void DateTimeGrid::drawTimeLine(QPainter* painter, const QRectF& rect)
{
    qreal x = dateTimeToChartX(m_timeLine->dateTime());
    if (rect.contains(x, rect.top())) {
        painter->save();
        painter->setPen(m_timeLine->pen());
        painter->drawLine(x, rect.top(), x, rect.bottom());
        painter->restore();
    }
}

qreal DateTimeGrid::dateTimeToChartX(const QDateTime& dt) const
{
    qreal result = startDateTime().date().daysTo(dt.date())*24.*60.*60.;
    result += startDateTime().time().msecsTo(dt.time())/1000.;
    result *= dayWidth()/(24.*60.*60.);
    
    return result;
}

bool DateTimeGrid::loadContext(const KoXmlElement &settings, Project *project)
{
    if (!project) {
        return false;
    }
    auto e = settings.namedItem("freedays").toElement();
    if (e.isNull()) {
        m_calendar = project->calendars().value(0);
        return false;
    }
    const auto id = e.attribute("calendar-id");
    m_calendar = project->findCalendar(id);

    return true;
}

void DateTimeGrid::saveContext(QDomElement &settings) const
{
    auto e = settings.ownerDocument().createElement(QStringLiteral("freedays"));
    settings.appendChild(e);
    e.setAttribute(QStringLiteral("calendar-id"), m_calendar ? m_calendar->id() : QLatin1String());
}
