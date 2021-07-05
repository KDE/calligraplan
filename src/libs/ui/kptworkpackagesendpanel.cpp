/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
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
    for (Node *n : tasks) {
        Task *t = qobject_cast<Task*>(n);
        if (t == nullptr) {
            continue;
        }
        const QList<Resource*> resources = t->workPackage().fetchResources(id);
        for (Resource *r : resources) {
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
        Q_EMIT sendWorkpackages(m_nodeMap[ r ], r, true /*by mail for now*/); // FIXME
        ui_treeView->model()->setData(idx, Qt::Checked, Qt::CheckStateRole);
    }
}

void WorkPackageSendPanel::slotSelectionChanged()
{
    ui_publish->setEnabled(ui_treeView->selectionModel()->selectedIndexes().value(0).isValid());
}
