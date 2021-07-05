/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTWORKPACKAGESENDPANEL_H
#define KPTWORKPACKAGESENDPANEL_H

#include "planui_export.h"

#include "ui_kptworkpackagesendpanel.h"

#include <QWidget>
#include <QMap>

class QPushButton;

namespace KPlato
{

class Resource;
class Node;
class ScheduleManager;

class PLANUI_EXPORT WorkPackageSendPanel : public QWidget, public Ui_WorkPackageSendPanel
{
    Q_OBJECT
public:
    explicit WorkPackageSendPanel(const QList<Node*> &tasks,  ScheduleManager *sm, QWidget *parent=nullptr);

Q_SIGNALS:
    void sendWorkpackages(const QList<KPlato::Node*>&, KPlato::Resource*, bool);

protected Q_SLOTS:
    void slotSendClicked();
    void slotSelectionChanged();

protected:
    QMap<QString, Resource*> m_resMap;
    QMap<Resource*, QList<Node*> > m_nodeMap;
};

} //KPlato namespace

#endif // KPTWORKPACKAGESENDPANEL_H
