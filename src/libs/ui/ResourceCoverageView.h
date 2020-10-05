/* This file is part of the KDE project
 * Copyright (C) 2020 Dag Andersen <dag.andersen@kdemail.net>
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


#ifndef RESOURCECOVERAGEVIEW_H
#define RESOURCECOVERAGEVIEW_H

#include "planui_export.h"
#include "kptviewbase.h"

#include <QList>

class KoDocument;
class KoPart;

namespace KChart
{
class Chart;
}

namespace KPlato
{

class Resource;
class Project;
class ScheduleManager;
class TreeViewBase;
class ResourceAppointmentsItemModel;
class SwapModel;
class AvailableModel;

class PLANUI_EXPORT ResourceCoverageView : public ViewBase
{
    Q_OBJECT
public:
    ResourceCoverageView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~ResourceCoverageView();

    QList<Resource*> selectedResources() const;

public Q_SLOTS:
    void setProject(Project *project) override;
    void setScheduleManager(ScheduleManager *sm) override;

    void selectionChanged();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    TreeViewBase *m_treeView;
    KChart::Chart *m_chart;
    ResourceAppointmentsItemModel *m_baseModel;
    SwapModel *m_usageModel;
    AvailableModel *m_availModel;
};


} // namespace KPlato

#endif
