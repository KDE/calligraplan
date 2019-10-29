/* This file is part of the KDE project
  Copyright (C) 2009, 2010 Dag Andersen <danders@get2net.dk>

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

#ifndef KPTRESOURCEALLOCATIONEDITOR_H
#define KPTRESOURCEALLOCATIONEDITOR_H

#include "planui_export.h"

#include "kptviewbase.h"
#include "kptresourceallocationmodel.h"
#include "kpttask.h"

#include <QHash>

class KoDocument;

class QPoint;


namespace KPlato
{

class Project;
class Resource;
class ResourceGroup;


class PLANUI_EXPORT ResourceAllocationTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit ResourceAllocationTreeView(QWidget *parent);

    ResourceAllocationItemModel *model() const { return static_cast<ResourceAllocationItemModel*>(DoubleTreeViewBase::model()); }

    Project *project() const { return model()->project(); }
    void setProject(Project *project) { model()->setProject(project); }

    Task *task() const { return model()->task(); }
    void setTask(Task *task) { model()->setTask(task); }

    QObject *currentObject() const;

    const QHash<const Resource*, ResourceRequest*> &resourceCache() const { return model()->resourceCache(); }
    const QHash<const ResourceGroup*, ResourceGroupRequest*> &groupCache() const { return model()->groupCache(); }

Q_SIGNALS:
    void dataChanged();

};

class PLANUI_EXPORT ResourceAllocationEditor : public ViewBase
{
    Q_OBJECT
public:
    ResourceAllocationEditor(KoPart *part, KoDocument *doc, QWidget *parent);
    
    void setupGui();
    Project *project() const override { return m_view->project(); }
    void setProject(Project *project) override { m_view->setProject(project); }

    ResourceAllocationItemModel *model() const { return m_view->model(); }
    
    void updateReadWrite(bool readwrite) override;

    Resource *currentResource() const override;
    ResourceGroup *currentResourceGroup() const override;
    
    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;
    
    KoPrintJob *createPrintJob() override;

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

protected Q_SLOTS:
    void slotOptions() override;

protected:
    void updateActionsEnabled(bool on = true);

private Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotSplitView();

    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&);
    void slotEnableActions(bool on);

private:
    ResourceAllocationTreeView *m_view;

};

}  //KPlato namespace

#endif
