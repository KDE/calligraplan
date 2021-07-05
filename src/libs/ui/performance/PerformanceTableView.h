/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPLATOPERFORMANCETABLEVIEW_H
#define KPLATOPERFORMANCETABLEVIEW_H

#include <QTableWidget>

namespace KPlato
{

class PerformanceTableView : public QTableView
{
    Q_OBJECT
public:
    explicit PerformanceTableView(QWidget *parent);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
};

} // namespace KPlato

#endif
