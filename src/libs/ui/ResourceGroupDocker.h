/* This file is part of the KDE project
 * Copyright (C) 2020 Dag Andersen <danders@get2net>
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
