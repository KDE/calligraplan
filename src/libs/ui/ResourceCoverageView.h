/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    void setProject(KPlato::Project *project) override;
    void setScheduleManager(KPlato::ScheduleManager *sm) override;

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
