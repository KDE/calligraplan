/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef GANTTFILTEROPTIONSWIDGET_H
#define GANTTFILTEROPTIONSWIDGET_H

#include <QWidget>
#include "ui_GanttFilterOptionsWidget.h"

#include <QHash>

class QCheckBox;
class QAction;

namespace KPlato
{
class NodeSortFilterProxyModel;

class GanttFilterOptionsWidget : public QFrame
{
    Q_OBJECT
public:
    explicit GanttFilterOptionsWidget(KPlato::NodeSortFilterProxyModel *model, QWidget *parent = nullptr);

    void addAction(QAction *action);
    int days() const;
    void setDays(int days);

private Q_SLOTS:
    void updatePeriod();

private:
    Ui::GanttFilterOptionsWidget ui;
    NodeSortFilterProxyModel *m_model;
};

}
#endif // GANTTFILTEROPTIONSWIDGET_H
