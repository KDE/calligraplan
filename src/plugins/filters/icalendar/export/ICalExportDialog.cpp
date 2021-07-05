/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ICalExportDialog.h"

#include "kptproject.h"
#include "kptschedule.h"

using namespace KPlato;

QModelIndex findManager(const ScheduleItemModel *model, const QModelIndex &parent = QModelIndex())
{
    for (int i = 0; i < model->rowCount(parent); ++i) {
        QModelIndex idx = model->index(i, 0, parent);
        ScheduleManager *sm = model->manager(idx);
        if (sm) {
            if (sm->isBaselined()) {
                return idx;
            }
        }
        if (model->hasChildren(idx)) {
            QModelIndex cidx = findManager(model, idx);
            if (cidx.isValid()) {
                return cidx;
            }
        }
    }
    if (!parent.isValid() && model->rowCount() > 0) {
        return model->index(0, 0);
    }
    return QModelIndex();
}

ICalExportDialog::ICalExportDialog(Project &project, QWidget *parent)
    : KoDialog(parent)
    , m_project(project)
{
    m_ui.setupUi(this);
    setMainWidget(m_ui.ui_mainWidget);
    setButtons(KoDialog::Ok | KoDialog::Cancel);
    m_ui.ui_scheduleView->setProject(&project);
    m_ui.ui_scheduleView->setColumnsHidden(QList<int>() << 1 << -1);
    m_ui.ui_scheduleView->expandAll();
    m_ui.ui_scheduleView->selectionModel()->select(findManager(m_ui.ui_scheduleView->model()), QItemSelectionModel::ClearAndSelect);
}

bool ICalExportDialog::includeProject() const
{
    return m_ui.ui_project->isChecked();
}

bool ICalExportDialog::includeSummarytasks() const
{
    return m_ui.ui_summarytasks->isChecked();
}

long ICalExportDialog::scheduleId() const
{
    ScheduleManager *m = m_ui.ui_scheduleView->currentManager();
    if (!m) {
        return ANYSCHEDULED;
    }
    return m->scheduleId();
}

