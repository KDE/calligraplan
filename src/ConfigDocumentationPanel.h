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

#ifndef CONFIGDOCUMENTATIONPANEL_H
#define CONFIGDOCUMENTATIONPANEL_H

#include "plan_export.h"

#include "ui_ConfigDocumentationPanel.h"

#include <QWidget>

namespace KPlato
{


class ConfigDocumentationPanelImpl : public QWidget, public Ui::ConfigDocumentationPanel
{
    Q_OBJECT
public:
    explicit ConfigDocumentationPanelImpl(QWidget *parent);
};

class PLAN_EXPORT ConfigDocumentationPanel : public ConfigDocumentationPanelImpl
{
    Q_OBJECT
public:
    explicit ConfigDocumentationPanel( QWidget *parent=0 );

private Q_SLOTS:
    void slotPathChanged();
};

} //KPlato namespace

#endif // CONFIGDOCUMENTATIONPANEL_H
