/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
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

#ifndef CONFIGTASKMODULESPANEL_H
#define CONFIGTASKMODULESPANEL_H

#include "plan_export.h"

#include "ui_ConfigTaskModulesPanel.h"

#include <QWidget>
#include <QStringListModel>

namespace KPlato
{


class PLAN_EXPORT ConfigTaskModulesPanel : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool hasChanged READ hasChanged)

public:
    explicit ConfigTaskModulesPanel( QWidget *parent=nullptr );

    QStringListModel model;
    Ui::ConfigTaskModulesPanel ui;

    bool hasChanged() const;

public Q_SLOTS:
    void updateSettings();
    void updateWidgets();

Q_SIGNALS:
    void settingsChanged();

private Q_SLOTS:
    void slotInsertClicked();
    void slotRemoveClicked();
};

} //KPlato namespace

#endif // CONFIGTASKMODULESPANEL_H
