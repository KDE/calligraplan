/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009, 2012 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptschedulesdocker.h"

#include "kptschedule.h"
#include <kptdebug.h>

#include <KLocalizedString>

#include <QAbstractItemView>
#include <QModelIndex>
#include <QModelIndexList>
#include <QTreeView>


namespace KPlato
{

SchedulesDocker::SchedulesDocker()
{
    setWindowTitle(i18n("Schedule Selector"));
    m_view = new QTreeView(this);
    m_sfModel.setSourceModel(&m_model);
    m_view->setModel(&m_sfModel);
    m_sfModel.setFilterKeyColumn (ScheduleModel::ScheduleScheduled);
    m_sfModel.setFilterRole(Qt::EditRole);
    m_sfModel.setFilterFixedString("true");
    m_sfModel.setDynamicSortFilter (true);

    for(int c = 1; c <  m_model.columnCount(); ++c) {
        m_view->setColumnHidden(c, true);
    }
    m_view->setHeaderHidden(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    setWidget(m_view);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SchedulesDocker::slotSelectionChanged);
}

SchedulesDocker::~SchedulesDocker()
{
}

void SchedulesDocker::slotSelectionChanged()
{
    Q_EMIT selectionChanged(selectedSchedule());
}

void SchedulesDocker::setProject(Project *project)
{
    debugPlan<<project;
    m_model.setProject(project);
}

ScheduleManager *SchedulesDocker::selectedSchedule() const
{
    QModelIndexList lst = m_view->selectionModel()->selectedRows();
    Q_ASSERT(lst.count() <= 1);
    ScheduleManager *sm = nullptr;
    if (! lst.isEmpty()) {
        sm = m_model.manager(m_sfModel.mapToSource(lst.first()));
    }
    return sm;
}

void SchedulesDocker::setSelectedSchedule(ScheduleManager *sm)
{
    qDebug()<<"setSelectedSchedule:"<<sm<<m_model.index(sm);
    QModelIndex idx = m_sfModel.mapFromSource(m_model.index(sm));
    if (sm) {
        Q_ASSERT(idx.isValid());
    }
    m_view->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);
    qDebug()<<"setSelectedSchedule:"<<sm<<idx;
}

//--------------------
SchedulesDockerFactory::SchedulesDockerFactory()
{
}

QString SchedulesDockerFactory::id() const
{
    return QString("KPlatoSchedulesDocker");
}

QDockWidget* SchedulesDockerFactory::createDockWidget()
{
    return new SchedulesDocker();
}

} //namespace KPlato
