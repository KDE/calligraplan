/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATO_SCROLLABLECHARTFILTERMODEL_H
#define KPLATO_SCROLLABLECHARTFILTERMODEL_H

#include "planui_export.h"

#include <QSortFilterProxyModel>

#define AXISRANGEROLE 234587

namespace KPlato
{

class PLANUI_EXPORT ScrollableChartFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    ScrollableChartFilterModel(QObject *parent = nullptr);

public Q_SLOTS:
    /// Set first row to be shown to @p row
    void setStart(int row);
    /// Set number of rows to be shown to @p numRows
    /// If @p numRows is 0, all rows are shown
    void setNumRows(int numRows);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    int m_start;
    int m_numRows;
};

} // namespace KPlato
#endif
