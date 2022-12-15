/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "GanttFilterOptionsWidget.h"
#include "NodeGanttViewBase.h"
#include "kptnodeitemmodel.h"

#include <QDateTime>

using namespace KPlato;

GanttFilterOptionsWidget::GanttFilterOptionsWidget(NodeSortFilterProxyModel *model, QWidget *parent)
    : QFrame{parent}
    , m_model(model)
{
    ui.setupUi(this);
    ui.start_date->setDate(model->periodStart());
    connect(ui.start_date, &QDateEdit::dateChanged, model, &NodeSortFilterProxyModel::setPeriodStart);
    ui.end_date->setDate(model->periodEnd());
    connect(ui.end_date, &QDateEdit::dateChanged, model, &NodeSortFilterProxyModel::setPeriodEnd);

    connect(ui.enable_period, &QRadioButton::toggled, this, &GanttFilterOptionsWidget::updatePeriod);
    connect(ui.period, qOverload<int>(&QSpinBox::valueChanged), this, &GanttFilterOptionsWidget::updatePeriod);

    connect(ui.tasks_and_milestones_group, &QGroupBox::toggled, this, [this](bool on) {
        if (on) {
            ui.period_group->setChecked(!on);
        }
    });
    connect(ui.period_group, &QGroupBox::toggled, this, [this](bool on) {
        if (on) {
            ui.tasks_and_milestones_group->setChecked(!on);
        }
    });

    connect(model->itemModel(), &KPlato::ItemModelBase::managerChanged, this, [this](ScheduleManager *sm) {
        if (sm && sm->expected()) {
            const auto start = sm->expected()->start().date();
            const auto end = sm->expected()->end().date();
            ui.start_date->setDateRange(start, end);
            ui.end_date->setDateRange(start, end);
            ui.period->setMaximum(start.daysTo(end) / 2);
        }
    });
    connect(ui.closeButton, &QPushButton::clicked, this, &QFrame::hide);
}

int GanttFilterOptionsWidget::days() const
{
    return ui.period->value();
}

void GanttFilterOptionsWidget::setDays(int days)
{
    ui.period->setValue(days);
}

void GanttFilterOptionsWidget::addAction(QAction *action)
{
    if (auto w = ui.scrollAreaWidgetContents->findChild<QCheckBox*>(action->objectName())) {
        w->setChecked(action->isChecked());
        w->setToolTip(action->toolTip());
        connect(w, &QCheckBox::toggled, action, &QAction::setChecked);
        connect(action, &QAction::toggled, w, &QCheckBox::setChecked);
    } else if (auto w = ui.scrollAreaWidgetContents->findChild<QGroupBox*>(action->objectName())) {
        w->setChecked(action->isChecked());
        w->setToolTip(action->toolTip());
        connect(w, &QGroupBox::toggled, action, &QAction::setChecked);
        connect(action, &QAction::toggled, w, &QGroupBox::setChecked);
    } else if (auto w = ui.scrollAreaWidgetContents->findChild<QRadioButton*>(action->objectName())) {
        w->setChecked(action->isChecked());
        w->setToolTip(action->toolTip());
        connect(w, &QRadioButton::toggled, action, &QAction::setChecked);
        connect(action, &QAction::toggled, w, &QRadioButton::setChecked);
    }
}

void GanttFilterOptionsWidget::updatePeriod()
{
    if (ui.enable_period->isChecked()) {
        auto date = QDate::currentDate();
        const auto lower = ui.start_date->minimumDate();
        const auto upper = ui.end_date->maximumDate();
        int days = std::min(ui.period->value(), (int)(lower.daysTo(upper) / 2));
        if (date > ui.end_date->maximumDate().addDays(-days)) {
            date = ui.end_date->maximumDate().addDays(-days);
        }
        if (date < ui.start_date->minimumDate().addDays(days)) {
            date = ui.start_date->minimumDate().addDays(days);
        }
        ui.start_date->setDate(date.addDays(-days));
        ui.end_date->setDate(date.addDays(days));
    }
}
