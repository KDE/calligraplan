/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPLATODATETIMEGRID_H
#define KPLATODATETIMEGRID_H

#include <KGanttDateTimeGrid>

#include "DateTimeTimeLine.h"

namespace KPlato {
    
class DateTimeGrid : public KGantt::DateTimeGrid
{
    Q_OBJECT
public:
    DateTimeGrid();
    
    DateTimeTimeLine *timeNow() const;

protected:
    void drawDayBackground(QPainter* painter, const QRectF& rect, const QDate& date) override;

    void drawDayForeground(QPainter* painter, const QRectF& rect, const QDate& date) override;

    void drawTimeLine(QPainter* painter, const QRectF& rect);

    qreal dateTimeToChartX(const QDateTime& dt) const;

private:
    DateTimeTimeLine *m_timeLine;
};

}

#endif
