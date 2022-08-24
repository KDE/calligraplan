/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_SCHEDULINGVIEW_H
#define PLANPORTFOLIO_SCHEDULINGVIEW_H

#include "planportfolio_export.h"

#include "SchedulingLogModel.h"
#include "ui_SchedulingView.h"

#include <KoView.h>
#include <KoPageLayout.h>

#include <SchedulingContext.h>

class KoDocument;
class KoPrintJob;
class QTreeView;
class QMenu;
class QItemSelection;
class QProgressDialog;

namespace KPlato {
    class ScheduleManager;
    class DateTime;
    class SchedulerPlugin;
}

class PLANPORTFOLIO_EXPORT SchedulingView : public KoView
{
    Q_OBJECT
public:
    explicit SchedulingView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~SchedulingView() override;

    KoPrintJob *createPrintJob() override;

    QString schedulerKey() const;

Q_SIGNALS:
    void projectCalculated(KPlato::Project *project, KPlato::ScheduleManager *sm);

protected Q_SLOTS:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void calculate();

    void updateSchedulingProperties();
    void slotSchedulersComboChanged(int idx);
    void slotGranularitiesChanged(int idx);
    void slotSequentialChanged(bool state);
    void slotTodayToggled(bool state);
    void slotTomorrowToggled(bool state);
    void slotTimeToggled(bool state);

    void updateLogFilter();

    void itemDoubleClicked(const QPersistentModelIndex &idx);
    void slotContextMenuRequested(const QPoint &pos);
    void slotDescription();

    void calculateFromChanged();

    void portfolioChanged();

protected:
    void updateReadWrite(bool readwrite) override;
    void setupGui();
    void updateActionsEnabled();

    KPlato::ScheduleManager* scheduleManager(const KoDocument *doc) const;

    QDateTime calculationTime() const;
    bool calculateSchedule(KPlato::SchedulerPlugin *scheduler);

private:
    bool m_readWrite;
    Ui::SchedulingView ui;
    QTreeView *m_logView;
    SchedulingLogModel m_logModel;
    KPlato::SchedulingContext m_schedulingContext;
    QProgressDialog *m_progress;
};

#endif
