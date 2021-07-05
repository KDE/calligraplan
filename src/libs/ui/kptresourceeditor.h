/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTRESOURCEEDITOR_H
#define KPTRESOURCEEDITOR_H

#include "planui_export.h"

#include <kptviewbase.h>


class KoDocument;

class QPoint;


namespace KPlato
{

class Project;
class Resource;
class ResourceGroup;
class ResourceItemModel;

class PLANUI_EXPORT ResourceTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit ResourceTreeView(QWidget *parent);

    ResourceItemModel *model() const;

    Project *project() const;
    void setProject(Project *project);

    QObject *currentObject() const;
    QList<QObject*> selectedObjects() const;
    QList<ResourceGroup*> selectedGroups() const;
    QList<Resource*> selectedResources() const;

protected Q_SLOTS:
    void slotDropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event);
};

class PLANUI_EXPORT ResourceEditor : public ViewBase
{
    Q_OBJECT
public:
    ResourceEditor(KoPart *part, KoDocument *doc, QWidget *parent);
    
    void setupGui();
    Project *project() const override { return m_view->project(); }
    void setProject(Project *project) override;

    ResourceItemModel *model() const { return m_view->model(); }
    
    void updateReadWrite(bool readwrite) override;

    Resource *currentResource() const override;
    Resource *resource(const QModelIndex &idx) const;
    
    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;
    
    KoPrintJob *createPrintJob() override;

Q_SIGNALS:
    void addResource(KPlato::Resource *resource);
    void deleteObjectList(const QObjectList&);
    void resourceSelected(KPlato::Resource *resource);

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;
    void slotEditCopy() override;

protected Q_SLOTS:
    void slotOptions() override;

protected:
    void updateActionsEnabled(bool on = true);
    void createDockers();

private Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotSplitView();
    
    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&);
    void slotEnableActions(bool on);

    void slotAddResource();
    void slotDeleteSelection();
    void slotEditCurrentResource();
    void slotEditResource(Resource *resource);

    void slotEditResourceFinished(int result);

private:
    void deleteResources(const QList<Resource*> &lst);
    void deleteResource(Resource *resource);

private:
    ResourceTreeView *m_view;

    QAction *actionAddResource;
    QAction *actionDeleteSelection;
};

}  //KPlato namespace

#endif
