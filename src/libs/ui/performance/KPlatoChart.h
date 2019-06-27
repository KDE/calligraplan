/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2007 - 2010 Dag Andersen <danders@get2net.dk>
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

#ifndef KPLATOCHART_H
#define KPLATOCHART_H

#include <KChartChart>

#include <QWidget>
#include <QMouseEvent>
#include <QDebug>

namespace KPlato
{

/*
 * class KPlato::Chart
 * 
 * Wrapper for KChart::Chart to prevent rubberband selection
 * with no keyboard modifers.
 * 
 * By default KChart::Chart eats the mouse events which makes
 * implementation of drag and context menu difficult.
 */
class Chart : public KChart::Chart
{
public:
    Chart(QWidget *parent) : KChart::Chart(parent) {}

protected:
    void mousePressEvent(QMouseEvent *event) {
        if (event->modifiers() != Qt::NoModifier) {
            KChart::Chart::mousePressEvent(event);
        } else {
            event->ignore();
        }
    }
    void mouseMoveEvent(QMouseEvent *event) {
        if (event->modifiers() != Qt::NoModifier) {
            KChart::Chart::mouseMoveEvent(event);
        } else {
            event->ignore();
        }
    }
};

}

#endif
