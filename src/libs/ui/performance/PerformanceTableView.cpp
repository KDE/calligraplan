/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2011 Dag Andersen <dag.andersen@kdemail.net>
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

// clazy:excludeall=qstring-arg
#include "PerformanceTableView.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QDebug>

using namespace KPlato;


PerformanceTableView::PerformanceTableView(QWidget *parent)
    : QTableView(parent)
{
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, this, &PerformanceTableView::customContextMenuRequested);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(verticalHeader(), &QHeaderView::customContextMenuRequested, this, &PerformanceTableView::customContextMenuRequested);
}

QSize PerformanceTableView::sizeHint() const
{
    QSize s = QTableView::sizeHint();
    int h = horizontalHeader()->height();
    for (int r = 0; r < verticalHeader()->count(); ++r) {
        if (! verticalHeader()->isSectionHidden(r)) {
            h += verticalHeader()->sectionSize(r);
        }
    }
    s.setHeight(h + frameWidth() * 2);
    return s;
}

QSize PerformanceTableView::minimumSizeHint() const
{
    return sizeHint();
}
