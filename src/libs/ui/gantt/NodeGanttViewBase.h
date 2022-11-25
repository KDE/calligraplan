/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 * 
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef NodeGanttViewBase_H
#define NodeGanttViewBase_H

#include "GanttViewBase.h"

#include <kptnodeitemmodel.h>

namespace KGantt
{
    class TreeViewRowController;
}

namespace KPlato
{

class NodeGanttViewBase : public GanttViewBase
{
    Q_OBJECT
public:
    explicit NodeGanttViewBase(QWidget *parent);
    ~NodeGanttViewBase() override;

    NodeSortFilterProxyModel *sfModel() const;
    void setItemModel(ItemModelBase *model);
    ItemModelBase *model() const;
    void setProject(Project *project) override;

    GanttItemDelegate *delegate() const { return m_ganttdelegate; }

    bool loadContext(const KoXmlElement &settings) override;
    void saveContext(QDomElement &settings) const override;

public Q_SLOTS:
    void setShowUnscheduledTasks(bool show);

protected:
    GanttItemDelegate *m_ganttdelegate;
    NodeItemModel m_defaultModel;
    KGantt::TreeViewRowController *m_rowController;
};

class MyKGanttView : public NodeGanttViewBase
{
    Q_OBJECT
public:
    explicit MyKGanttView(QWidget *parent);

    GanttItemModel *model() const;
    void setProject(Project *project) override;
    void setScheduleManager(ScheduleManager *sm);

public Q_SLOTS:
    void clearDependencies();
    void createDependencies();
    void addDependency(KPlato::Relation *rel);
    void removeDependency(KPlato::Relation *rel);
    void slotProjectCalculated(KPlato::ScheduleManager *sm);

    void slotNodeInserted(KPlato::Node *node);

protected:
    ScheduleManager *m_manager;
};

}  //KPlato namespace

#endif
