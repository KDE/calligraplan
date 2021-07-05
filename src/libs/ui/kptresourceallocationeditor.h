/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009, 2010 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
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
