/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTRESOURCEGROUPDOCKER_H
#define KPTRESOURCEGROUPDOCKER_H

#include "planui_export.h"

#include "kptviewbase.h"

#include <KCheckableProxyModel>

class QItemSelectionModel;

namespace KPlato
{

class Project;
class ResourceGroup;

class CheckableProxyModel : public KCheckableProxyModel
{
    Q_OBJECT
public:
    CheckableProxyModel(QObject *parent = nullptr);

    int columnCount(const QModelIndex &idx) const override;

    void clear(const QModelIndex &parent = QModelIndex());
};

class PLANUI_EXPORT ResourceGroupDocker : public DockWidget
{
    Q_OBJECT
public:
    explicit ResourceGroupDocker(QItemSelectionModel *groupSelection, ViewBase *parent, const QString &identity, const QString &title);

public Q_SLOTS:
    void setProject(KPlato::Project *project);
    void setGroup(KPlato::ResourceGroup *group);

private Q_SLOTS:
    void slotSelectionChanged();
    void slotDataChanged(const QModelIndex &idx1, const QModelIndex &idx2);
    void updateCheckableModel();

private:
    QModelIndex selectedGroupIndex() const;
    ResourceGroup *selectedGroup() const;

private:
    CheckableProxyModel m_checkable;
    QItemSelectionModel *m_groupSelection;
    ResourceGroup *m_group;
};


}  //KPlato namespace

#endif
