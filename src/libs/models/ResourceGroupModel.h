/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef RESOURCEGROUPMODEL_H
#define RESOURCEGROUPMODEL_H

#include "planmodels_export.h"

#include <QSortFilterProxyModel>
#include <QMetaEnum>

class QByteArray;

namespace KIO {
    class Job;
}
class KJob;

namespace KPlato
{

class Project;
class ResourceGroup;

class PLANMODELS_EXPORT ResourceGroupModel : public QObject
{
    Q_OBJECT
public:
    explicit ResourceGroupModel(QObject *parent = nullptr);
    ~ResourceGroupModel() override;

    enum Properties {
        Name = 0,
        Origin,
        Type,
        Units,
        Coordinator
    };
    Q_ENUM(Properties)

    const QMetaEnum columnMap() const;
    void setProject(Project *project);
    int propertyCount() const;
    QVariant data(const ResourceGroup *group, int property, int role = Qt::DisplayRole) const;
    static QVariant headerData(int section, int role = Qt::DisplayRole);

    QVariant name(const ResourceGroup *group, int role) const;
    QVariant origin(const ResourceGroup *group, int role) const;
    QVariant type(const ResourceGroup *group, int role) const;
    QVariant units(const ResourceGroup *group, int role) const;
    QVariant coordinator(const ResourceGroup *group, int role) const;

private:
    Project *m_project;
};

}  //KPlato namespace

#endif
