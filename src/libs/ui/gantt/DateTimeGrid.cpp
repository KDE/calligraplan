/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "DateTimeGrid.h"
#include "config.h"

#include <QRectF>
#include <QPainter>
#include <QDate>

using namespace KPlato;

DateTimeGrid::DateTimeGrid()
    : KGantt::DateTimeGrid()
    , m_timeLine(new DateTimeTimeLine())
{}

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

