/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    connect(this, &QTableView::customContextMenuRequested, this, [this](const QPoint &pos) {
        Q_EMIT contextMenuRequested(viewport()->mapToParent(pos));
     });
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, this, [this](const QPoint &pos) {
        Q_EMIT contextMenuRequested(horizontalHeader()->mapToParent(pos));
     });
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(verticalHeader(), &QHeaderView::customContextMenuRequested, this, [this](const QPoint &pos) {
        Q_EMIT contextMenuRequested(verticalHeader()->mapToParent(pos));
     });
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
