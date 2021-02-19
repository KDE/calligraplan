/* This file is part of the KDE project
 * Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
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

#ifndef PLANPORTFOLIO_SCHEDULINGVIEW_H
#define PLANPORTFOLIO_SCHEDULINGVIEW_H

#include "planportfolio_export.h"

#include <KoView.h>
#include <KoPageLayout.h>

class KoDocument;
class KoPrintJob;
class QTreeView;
class QMenu;
class QItemSelection;

namespace KPlato {
    class ScheduleManager;
    class ScheduleLogTreeView;
    class DateTime;
    class SchedulerPlugin;
}

class PLANPORTFOLIO_EXPORT SchedulingView : public KoView
{
    Q_OBJECT
public:
    explicit SchedulingView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~SchedulingView() override;

    QPrintDialog* createPrintDialog(KoPrintJob*, QWidget*) override;

protected Q_SLOTS:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void loadProjects();
    void slotLoadCompleted();
    void slotLoadCanceled();

    void calculate();
    void calculateTJ(KPlato::SchedulerPlugin *scheduler, const QList<KoDocument*> docs);
    void calculatePert();

    void slotDoubleClicked(const QModelIndex &idx);
    void slotCustomContextMenuRequested(const QPoint &pos);
    void openProject();

Q_SIGNALS:
    void openDocument(KoDocument *doc);

protected:
    void updateReadWrite(bool readwrite) override;
    void setupGui();
    void updateActionsEnabled();

    QList<QUrl> projectUrls(const QList<QUrl> &dirs);
    void loadProject(const QUrl &url);

    KPlato::ScheduleManager* scheduleManager(const KoDocument *doc) const;
    void schedule(KoDocument *doc, QList<KoDocument*> include);

    KPlato::DateTime targetStart() const;

private:
    bool m_readWrite;
    QTreeView *m_view;
    KPlato::ScheduleLogTreeView *m_logView;
};

#endif
