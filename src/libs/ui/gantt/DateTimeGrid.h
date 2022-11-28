/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPLATODATETIMEGRID_H
#define KPLATODATETIMEGRID_H

#include <KGanttDateTimeGrid>

#include "DateTimeTimeLine.h"

#include <kptcalendar.h>

#include <QPointer>

class AbstractRowController;

namespace KPlato {

class Project;

class DateTimeGrid : public KGantt::DateTimeGrid
{
    Q_OBJECT
public:
    DateTimeGrid();

    Calendar *calendar() const;
    void setCalendar(Calendar *calendar);

    DateTimeTimeLine *timeNow() const;

    void paintGrid( QPainter* painter, const QRectF& sceneRect, const QRectF& exposedRect, KGantt::AbstractRowController* rowController = nullptr, QWidget* widget = nullptr ) override;

protected:
    void drawDayBackground(QPainter* painter, const QRectF& rect, const QDate& date) override;

    void drawDayForeground(QPainter* painter, const QRectF& rect, const QDate& date) override;

    void drawTimeLine(QPainter* painter, const QRectF& rect);

    qreal dateTimeToChartX(const QDateTime& dt) const;

    QDateTime chartXtoDateTime( qreal x ) const;
    Qt::PenStyle gridLinePenStyle( QDateTime dt ) const;
    QDateTime adjustDateTimeForHeader( QDateTime dt ) const;
    void paintVerticalLines( QPainter* painter, const QRectF& sceneRect, const QRectF& exposedRect, QWidget* widget);
    void paintVerticalUserDefinedLines(QPainter* painter, const QRectF& sceneRect, const QRectF& exposedRect, QWidget *widget);

private:
    DateTimeTimeLine *m_timeLine;
    QPointer<Calendar> m_calendar;
};

}

#endif
