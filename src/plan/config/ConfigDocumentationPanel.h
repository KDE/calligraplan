/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    explicit ConfigDocumentationPanel(QWidget *parent=nullptr);

private Q_SLOTS:
    void slotPathChanged();
};

} //KPlato namespace

#endif // CONFIGDOCUMENTATIONPANEL_H
