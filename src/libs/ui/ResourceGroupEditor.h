/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTRESOURCEGROUPEDITOR_H
#define KPTRESOURCEGROUPEDITOR_H

#include "planui_export.h"

#include <kptviewbase.h>


class KoDocument;

class QPoint;


namespace KPlato
{

class Project;
class Resource;
class ResourceGroup;
class ResourceGroupItemModel;

class PLANUI_EXPORT ResourceGroupTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit ResourceGroupTreeView(QWidget *parent);

    ResourceGroupItemModel *model() const;

    Project *project() const;
    void setProject(Project *project);

    QObject *currentObject() const;
    QList<QObject*> selectedObjects() const;
    QList<ResourceGroup*> selectedGroups() const;
    QList<Resource*> selectedResources() const;

protected Q_SLOTS:
    void slotDropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event);
};

class PLANUI_EXPORT ResourceGroupEditor : public ViewBase
{
    Q_OBJECT
public:
    ResourceGroupEditor(KoPart *part, KoDocument *dic, QWidget *parent);

    void setupGui();
    Project *project() const override { return m_view->project(); }
    void setProject(Project *project) override;

    ResourceGroupItemModel *model() const { return m_view->model(); }

    void updateReadWrite(bool readwrite) override;

    ResourceGroup *currentResourceGroup() const override;

    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;

    KoPrintJob *createPrintJob() override;

Q_SIGNALS:
    void deleteObjectList(const QObjectList&);

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;
    void slotEditCopy() override;

protected Q_SLOTS:
    void slotOptions() override;

protected:
    void updateActionsEnabled(bool on);
    void createDockers();

private Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotSplitView();

    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&);
    void slotEnableActions(bool on);

    void slotAddGroup();
    void slotAddSubGroup();
    void slotDeleteSelection();

private:
    void deleteResourceGroup(ResourceGroup *group);
    void deleteResourceGroups(const QList<ResourceGroup*> &groups);

private:
    ResourceGroupTreeView *m_view;

    QAction *actionAddGroup;
    QAction *actionAddSubGroup;
    QAction *actionDeleteSelection;

};

}  //KPlato namespace

#endif
