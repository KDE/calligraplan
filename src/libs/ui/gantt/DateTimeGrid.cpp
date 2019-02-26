/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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

qreal DateTimeGrid::dateTimeToChartX( const QDateTime& dt ) const
{
    qreal result = startDateTime().date().daysTo(dt.date())*24.*60.*60.;
    result += startDateTime().time().msecsTo(dt.time())/1000.;
    result *= dayWidth()/( 24.*60.*60. );
    
    return result;
}

