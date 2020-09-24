/* This file is part of the KDE project
   Copyright (C) 2007 Dag Andersen <dag.andersen@kdemail.net>
   Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

// clazy:excludeall=qstring-arg
#include "kptworkpackagesendpanel.h"
#include "kpttaskeditor.h"

#include "kptproject.h"
#include "kpttask.h"
#include "kptschedule.h"

#include <KoIcon.h>

#include <KLocalizedString>

#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QList>
#include <QStandardItem>
#include <QStandardItemModel>

using namespace KPlato;


WorkPackageSendPanel::WorkPackageSendPanel(const QList<Node*> &tasks,  ScheduleManager *sm, QWidget *p)
    : QWidget(p)
{
    setupUi(this);
    QStandardItemModel *m = new QStandardItemModel(0, 1, ui_treeView);
    m->setHeaderData(0, Qt::Horizontal, xi18nc("@title", "Resource"));
    ui_treeView->setModel(m);

    long id = sm ? sm->scheduleId() : NOTSCHEDULED;
    foreach (Node *n, tasks) {
        Task *t = qobject_cast<Task*>(n);
        if (t == nullptr) {
            continue;
        }
        foreach (Resource *r, t->workPackage().fetchResources(id)) {
            m_resMap[r->name()] = r;
            m_nodeMap[ r ] << n;
        }
    }
    QMap<QString, Resource*>::const_iterator it;
    for (it = m_resMap.constBegin(); it != m_resMap.constEnd(); ++it) {
        QStandardItem *item = new QStandardItem(it.key());
        m->appendRow(item);
    }
    connect(ui_publish, &QAbstractButton::clicked, this, &WorkPackageSendPanel::slotSendClicked);
    connect(ui_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WorkPackageSendPanel::slotSelectionChanged);
}

void WorkPackageSendPanel::slotSendClicked()
{
//     Resource *r = m_pbMap[ qobject_cast<QPushButton*>(sender()) ];
    QModelIndex idx = ui_treeView->selectionModel()->selectedIndexes().value(0);
    if (idx.isValid()) {
        Resource *r = m_resMap.value(idx.data().toString());
        Q_ASSERT(r);
        emit sendWorkpackages(m_nodeMap[ r ], r, true /*by mail for now*/); // FIXME
        ui_treeView->model()->setData(idx, Qt::Checked, Qt::CheckStateRole);
    }
}

void WorkPackageSendPanel::slotSelectionChanged()
{
    ui_publish->setEnabled(ui_treeView->selectionModel()->selectedIndexes().value(0).isValid());
}
