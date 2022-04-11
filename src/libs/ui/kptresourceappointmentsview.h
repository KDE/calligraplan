 /* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTRESOURCEAPPOINTMENTSVIEW_H
#define KPTRESOURCEAPPOINTMENTSVIEW_H

#include "planui_export.h"

#include "kptviewbase.h"
#include "kptresourceappointmentsmodel.h"

#include <kpagedialog.h>

#include <QDomDocument>

class KoPageLayoutWidget;
class KoDocument;

class QPoint;

namespace KPlato
{

class View;
class Project;
class Resource;
class ResourceGroup;
class ScheduleManager;
class ResourceAppointmentsItemModel;

class PLANUI_EXPORT ResourceAppointmentsTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit ResourceAppointmentsTreeView(QWidget *parent);

    ResourceAppointmentsItemModel *model() const { return static_cast<ResourceAppointmentsItemModel*>(DoubleTreeViewBase::model()); }

    Project *project() const { return model()->project(); }
    void setProject(Project *project) { model()->setProject(project); }
    void setScheduleManager(ScheduleManager *sm) { model()->setScheduleManager(sm); }

    QModelIndex currentIndex() const;
    
    /// Load context info into this view.
    virtual bool loadContext(const KoXmlElement &context);
    using DoubleTreeViewBase::loadContext;
    /// Save context info from this view.
    virtual void saveContext(QDomElement &context) const;
    using DoubleTreeViewBase::saveContext;

protected Q_SLOTS:
    void slotRefreshed();

private:
    ViewBase *m_view;
};

class PLANUI_EXPORT ResourceAppointmentsView : public ViewBase
{
    Q_OBJECT
public:
    ResourceAppointmentsView(KoPart *part, KoDocument *doc, QWidget *parent);
    
    void setupGui();
    void setProject(Project *project) override;
    Project *project() const override { return m_view->project(); }
    void draw(Project &project) override;
    void draw() override;

    ResourceAppointmentsItemModel *model() const { return m_view->model(); }

    Node *currentNode() const override;
    Resource *currentResource() const override;
    ResourceGroup *currentResourceGroup() const override;
    
    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;
    
    KoPrintJob *createPrintJob() override;
    
Q_SIGNALS:
    void addResource(KPlato::ResourceGroup*);
    void deleteObjectList(const QObjectList&);
    
public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;
    
    void setScheduleManager(KPlato::ScheduleManager *sm) override;

protected Q_SLOTS:
    void slotOptions() override;

protected:
    void updateActionsEnabled(bool on = true);

private Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    
    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&);
    void slotEnableActions(bool on);

    void slotAddResource();
    void slotAddGroup();
    void slotDeleteSelection();

    void slotTaskProgress();
    void slotTaskDescription();
    void slotOpenTaskDescription(bool);
    void slotDocuments();

    void slotTaskProgressFinished(int result);
    void slotTaskDescriptionFinished(int result);
    void slotDocumentsFinished(int result);

private:
    ResourceAppointmentsTreeView *m_view;

    QAction *actionAddResource;
    QAction *actionAddGroup;
    QAction *actionDeleteSelection;
    QDomDocument m_domdoc;

};

}  //KPlato namespace

#endif // KPTRESOURCEAPPOINTMENTSVIEW_H
