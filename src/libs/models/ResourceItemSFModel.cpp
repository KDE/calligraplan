/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2006-2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ResourceItemSFModel.h"

#include "ResourceItemModel.h"
#include <kptproject.h>
#include <kptdebug.h>

#include <QApplication>
#include <QComboBox>
#include <QKeyEvent>
#include <QModelIndex>
#include <QItemSelection>
#include <QStyleOptionViewItem>
#include <QTimeEdit>
#include <QPainter>
#include <QToolTip>
#include <QTreeView>
#include <QStylePainter>
#include <QMimeData>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTableFormat>
#include <QVector>
#include <QTextLength>
#include <QTextTable>

#include <QBuffer>
#include <KoStore.h>
#include <KoXmlWriter.h>
#include <KoOdfWriteStore.h>

#include <KComboBox>
#include <KLineEdit>


using namespace KPlato;


ResourceItemSFModel::ResourceItemSFModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_project(nullptr)
{
    setDynamicSortFilter(true);
    ResourceItemModel *m = new ResourceItemModel(this);
    m->setIsCheckable(false);
    setSourceModel(m);
}

void ResourceItemSFModel::setProject(Project *project)
{
    static_cast<ResourceItemModel*>(sourceModel())->setProject(project);
    m_project = project;
}

Resource *ResourceItemSFModel::resource(const QModelIndex &idx) const
{
    return static_cast<ResourceItemModel*>(sourceModel())->resource(mapToSource(idx));
}

QModelIndex ResourceItemSFModel::index(Resource *r) const
{
    return mapFromSource(static_cast<ResourceItemModel*>(sourceModel())->index(r));
}

Qt::ItemFlags ResourceItemSFModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = QSortFilterProxyModel::flags(index);
    return f;
}

void ResourceItemSFModel::setFilteredResources(const QList<Resource*> &lst)
{
    m_filteredResources = lst;
}

void ResourceItemSFModel::addFilteredResource(Resource *r)
{
    if (!m_filteredResources.contains(r)) {
        m_filteredResources << r;
    }
}

bool ResourceItemSFModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
    if (m_project && m_filteredResources.contains(m_project->resourceAt(source_row))) {
        return false;
    }
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
