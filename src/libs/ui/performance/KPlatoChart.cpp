/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007-2010 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    const auto lst = coordinatePlanes();
    for (KChart::AbstractCoordinatePlane* plane : lst) {
        if (plane->geometry().contains(event->pos()) && plane->diagrams().size() > 0) {
            KChart::Chart::mousePressEvent(event);
            m_inplane = true;
            return;
        }
    }
    if (event->button() == Qt::RightButton) {
        Q_EMIT customContextMenuRequested(event->globalPosition().toPoint());
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
        Q_EMIT customContextMenuRequested(mapToGlobal(QPoint(0,0)));
    }
}
