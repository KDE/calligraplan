/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2007 - 2010 Dag Andersen <dag.andersen@kdemail.net>
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


#include "KPlatoChart.h"

#include <QWidget>
#include <QMouseEvent>
#include <QDebug>

#include <KChartAbstractCoordinatePlane>

using namespace KPlato;

Chart::Chart(QWidget *parent)
    : KChart::Chart(parent)
    , m_inplane(false)
{
    setFocusPolicy(Qt::StrongFocus);
}

void Chart::mousePressEvent(QMouseEvent *event)
{
    Q_FOREACH(KChart::AbstractCoordinatePlane* plane, coordinatePlanes()) {
        if (plane->geometry().contains(event->pos()) && plane->diagrams().size() > 0) {
            KChart::Chart::mousePressEvent(event);
            m_inplane = true;
            return;
        }
    }
    if (event->button() == Qt::RightButton) {
        emit customContextMenuRequested(event->globalPos());
    } else {
        event->ignore();
    }
}

void Chart::mouseMoveEvent(QMouseEvent *event)
{
    if (m_inplane) {
        KChart::Chart::mouseMoveEvent(event);
    } else {
        event->ignore();
    }
}

void Chart::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_inplane) {
        KChart::Chart::mouseReleaseEvent(event);
        m_inplane = false;
    } else {
        event->ignore();
    }
}

void Chart::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Menu) {
        emit customContextMenuRequested(mapToGlobal(QPoint(0,0)));
    }
}
