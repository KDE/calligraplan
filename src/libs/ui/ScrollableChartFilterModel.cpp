/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ScrollableChartFilterModel.h"

#include <kptdebug.h>

using namespace KPlato;

ScrollableChartFilterModel::ScrollableChartFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_start(0)
    , m_numRows(0)
{
}

void ScrollableChartFilterModel::setStart(int row)
{
    debugPlanChart<<row;
    m_start = row;
    invalidateFilter();
}

void ScrollableChartFilterModel::setNumRows(int numRows)
{
    debugPlanChart<<numRows;
    m_numRows = numRows;
    invalidateFilter();
}

bool ScrollableChartFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (source_parent.isValid()) {
        // we only handle tables
        return false;
    }
    if (m_numRows == 0) {
        return true;
    }
    const int end = m_start + m_numRows - 1;
    const bool accept = source_row >= m_start && source_row <= end;
    return accept;
}
